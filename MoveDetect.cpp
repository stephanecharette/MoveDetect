// MoveDetect -- Library to detect whether movement can be detected between two images or video frames.
// See LICENSE.TXT for details.
// $Id: $

#include "MoveDetect.hpp"


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
	next_frame_index			= 0;
	next_key_frame				= 0;
	key_frame_frequency			= 15;
	frame_index_with_movement	= 0;
	psnr_threshold				= 32.0;
	psnr_score					= 0.0;
	thumbnail_width				= 24;
	thumbnail_size				= cv::Size(0, 0);
	number_of_control_frames	= 2;
	movement_last_detected		= std::chrono::high_resolution_clock::time_point();

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
		thumbnail_size.width = thumbnail_width;
		double factor = static_cast<double>(thumbnail_size.width) / static_cast<double>(image.cols);
		thumbnail_size.height = std::round(factor * static_cast<double>(image.rows));
	}

	cv::Mat scb = image; //simple_colour_balance(image);
	cv::Mat thumbnail;
	cv::resize(scb, thumbnail, thumbnail_size, 0, 0, cv::INTER_AREA);

	// Now compare this image against all the other control images we've kept.
	//
	// Do the comparison in reverse order, starting with the most recent thumbnail
	// since if there was movement, that would be the first place we'd detect it.
	bool movement_detected = false;
	for (auto iter = control.rbegin(); iter != control.rend(); iter ++)
	{
//		const auto & key = iter->first;		// the index value
		const auto & val = iter->second;	// the stored thumbnail for the given index

		psnr_score = psnr(val, thumbnail);
		if (psnr_score < psnr_threshold)
		{
			movement_detected = true;
			movement_last_detected = std::chrono::high_resolution_clock::now();
			frame_index_with_movement = frame_index;
			break;
		}
	}

	if (frame_index >= next_key_frame)
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

#if 0
	cv::imwrite("frame_"		+ std::to_string(frame_index) + "_" + (movement_detected ? "YES" : "NO") + ".jpg", image	, {cv::IMWRITE_JPEG_QUALITY, 75});
	cv::imwrite("thumbnail_"	+ std::to_string(frame_index) + "_" + (movement_detected ? "YES" : "NO") + ".jpg", thumbnail, {cv::IMWRITE_JPEG_QUALITY, 75});
	cv::imwrite("scb_"			+ std::to_string(frame_index) + "_" + (movement_detected ? "YES" : "NO") + ".jpg", scb		, {cv::IMWRITE_JPEG_QUALITY, 75});
#endif

	return movement_detected;
}
