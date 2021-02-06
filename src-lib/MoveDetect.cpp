// MoveDetect - C++ library to detect movement.
// Copyright 2021 Stephane Charette <stephanecharette@gmail.com>
// MIT license applies.  See "license.txt" for details.

#include "MoveDetect.hpp"
#include <algorithm>


double MoveDetect::psnr(const cv::Mat & src, const cv::Mat & dst)
{
	if (src.empty() || dst.empty())
	{
		throw std::invalid_argument("cannot calculate psnr using empty image");
	}

	if (src.type() != dst.type() ||
		src.cols != dst.cols ||
		src.rows != dst.rows)
	{
		throw std::invalid_argument("src and dst images cannot be compared");
	}

	cv::Mat s1;
	cv::absdiff(src, dst, s1);	// |src - dst|
	s1.convertTo(s1, CV_32F);	// cannot make a square on 8 bits
	s1 = s1.mul(s1);			// |src - dst|^2
	cv::Scalar s = sum(s1);		// sum elements per channel
	const double sse = s.val[0] + s.val[1] + s.val[2]; // sum channels

	if (sse <= 1e-10)			// for small values return zero
	{
		return 0.0;
	}

	const double mse	= sse / static_cast<double>(src.channels() * src.total());
	const double psnr	= 10.0 * std::log10((255 * 255) / mse);

	return psnr;
}


cv::Mat MoveDetect::simple_colour_balance(const cv::Mat & src)
{
	if (src.empty() || src.channels() != 3)
	{
		throw std::invalid_argument("cannot colour balance the given image");
	}

	const float full_percent = 1.0;
	const float half_percent = full_percent / 200.0f;

	std::vector<cv::Mat> tmpsplit;
	cv::split(src, tmpsplit);

	for(size_t idx = 0; idx < 3; idx ++)
	{
		// find the low and high precentile values (based on the input percentile)
		cv::Mat flat;
		tmpsplit[idx].reshape(1,1).copyTo(flat);
		cv::sort(flat, flat, cv::SORT_EVERY_ROW + cv::SORT_ASCENDING);
		const int lo = flat.at<uchar>(cvFloor(((float)flat.cols) * (0.0 + half_percent)));
		const int hi = flat.at<uchar>( cvCeil(((float)flat.cols) * (1.0 - half_percent)));

		// saturate below the low percentile and above the high percentile
		tmpsplit[idx].setTo(lo, tmpsplit[idx] < lo);
		tmpsplit[idx].setTo(hi, tmpsplit[idx] > hi);

		// scale the channel
		cv::normalize(tmpsplit[idx], tmpsplit[idx], 0, 255, cv::NORM_MINMAX);
	}

	cv::Mat output;
	cv::merge(tmpsplit, output);

	return output;
}


MoveDetect::Handler::Handler()
{
	clear();

	return;
}


MoveDetect::Handler::~Handler()
{
	return;
}


bool MoveDetect::Handler::empty() const
{
	return control.empty();
}


MoveDetect::Handler & MoveDetect::Handler::clear()
{
	control.clear();
	movement_detected			= false;
	transition_detected			= false;
	next_frame_index			= 0;
	next_key_frame				= 0;
	key_frame_frequency			= 10;
	number_of_control_frames	= 4;
	psnr_threshold				= 32.0;
	most_recent_psnr_score		= 0.0;
	thumbnail_ratio				= 0.05;
	thumbnail_size				= cv::Size(0, 0);
	frame_index_with_movement	= 0;
	movement_last_detected		= std::chrono::high_resolution_clock::time_point();
	mask_enabled				= false;
	mask						= cv::Mat();
	line_type					= cv::LINE_4;
	contours_enabled			= false;
	contours_size				= 1;
	bbox_enabled				= false;
	bbox_size					= 1;
	output						= cv::Mat();

	return *this;
}


bool MoveDetect::Handler::detect(cv::Mat & next_image)
{
	return detect(next_frame_index, next_image);
}


bool MoveDetect::Handler::detect(const size_t frame_index, cv::Mat & image)
{
	if (image.empty())
	{
		throw std::invalid_argument("cannot detect using an empty image");
	}

	if (thumbnail_size.area() <= 1)
	{
		// we need to figure out a decent width and height to use for the thumbnails
		thumbnail_ratio			= std::clamp(thumbnail_ratio, 0.01, 1.0);
		thumbnail_size.width	= image.cols * thumbnail_ratio;
		thumbnail_size.height	= image.rows * thumbnail_ratio;
	}

	if (contours_enabled or bbox_enabled)
	{
		// bounding box requires the mask to be created
		mask_enabled = true;
	}

	cv::Mat scb = image; //simple_colour_balance(image);
	cv::Mat thumbnail;
	cv::resize(scb, thumbnail, thumbnail_size, 0, 0, cv::INTER_AREA);

	// Now compare this image against all the other control images we've kept.
	//
	// Do the comparison in reverse order, starting with the most recent thumbnail
	// since if there was movement, that would be the first place we'd detect it.
	const bool previous_movement_detected = movement_detected;
	movement_detected = false;
	for (auto iter = control.rbegin(); iter != control.rend(); iter ++)
	{
//		const auto & key = iter->first;		// the index value
		const auto & val = iter->second;	// the stored thumbnail for the given index

		most_recent_psnr_score = psnr(val, thumbnail);
		if (most_recent_psnr_score < psnr_threshold)
		{
			movement_detected = true;
			movement_last_detected = std::chrono::high_resolution_clock::now();
			frame_index_with_movement = frame_index;

			if (mask_enabled)
			{
				cv::Mat differences;
				cv::absdiff(val, thumbnail, differences);

				// This gives us a very tiny 3-channel image.
				// Now resize it to match the original image size.

				cv::Mat differences_resized;
				cv::resize(differences, differences_resized, image.size(), 0, 0, cv::INTER_CUBIC);

				// We'd like to generate a binary threshold, but that requires us to convert
				// the image to greyscale first.
				cv::Mat greyscale;
				cv::cvtColor(differences_resized, greyscale, cv::COLOR_BGR2GRAY);
				cv::Mat threshold;
				cv::threshold(greyscale, threshold, 0.0, 255.0, cv::THRESH_BINARY | cv::THRESH_OTSU);

				// And finally we dilate + erode the results to combine regions.
				cv::Mat dilated;
				cv::dilate(threshold, dilated, cv::Mat(), cv::Point(-1, -1), 10);
				cv::Mat eroded;
				cv::erode(dilated, eroded, cv::Mat(), cv::Point(-1, -1), 10);

				mask = eroded;
			}

			break;
		}
	}

	transition_detected = (previous_movement_detected != movement_detected);

	if (mask_enabled)
	{
		// if the caller is expecting a mask, but we have no movement then reset the mask to a "blank" image
		if (mask.empty() or (transition_detected and movement_detected == false))
		{
			mask = cv::Mat(image.size(), CV_8UC1, {0, 0, 0});
		}

		if (contours_enabled or bbox_enabled)
		{
			output = image.clone();

			if (contours_enabled)
			{
				typedef std::vector<cv::Point> Contour;
				typedef std::vector<Contour> VContours;
				typedef std::vector<cv::Vec4i> Hierarchy;

				VContours contours;
				Hierarchy hierarchy;
				cv::findContours(mask, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
				for (auto & contour : contours)
				{
					cv::polylines(output, contour, true, {0, 0, 255}, contours_size, line_type);
				}
				
			}

			if (bbox_enabled)
			{
				cv::rectangle(output, cv::boundingRect(mask), {0, 255, 255}, bbox_size, line_type);
			}
		}
	}

	// see if we need to keep this image as a "key" frame
	if (frame_index >= next_key_frame or control.size() < number_of_control_frames)
	{
		control[frame_index] = thumbnail;
		while (control.size() > number_of_control_frames)
		{
			auto iter = control.begin();
			control.erase(iter);
		}
		next_key_frame = frame_index + key_frame_frequency;
	}

	next_frame_index = frame_index + 1;

	return movement_detected;
}
