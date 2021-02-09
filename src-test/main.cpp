// MoveDetect - C++ library to detect movement.
// Copyright 2021 Stephane Charette <stephanecharette@gmail.com>
// MIT license applies.  See "license.txt" for details.

#include "MoveDetect.hpp"


int main(int argc, char *argv[])
{
	std::cout << "Simple test application for the Movement Detection library." << std::endl;

	if (argc < 2)
	{
		std::cout
			<< "Usage:" << std::endl
			<< "\t" << argv[0] << " <video1> [<video2> ...]" << std::endl;
		return 1;
	}

	for (int video_index = 1; video_index < argc; video_index ++)
	{
		cv::VideoCapture video_input(argv[video_index]);
		if (video_input.isOpened() == false)
		{
			std::cout << "ERROR: failed to open " << argv[video_index] << std::endl;
			continue;
		}

		const bool save_output_video	= false;
		const double zoom_factor		= 0.85;
		const double input_fps			= video_input.get(cv::VideoCaptureProperties::CAP_PROP_FPS			);
		const double original_width		= video_input.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_WIDTH	);
		const double original_height	= video_input.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_HEIGHT	);
		const double total_frames		= video_input.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_COUNT	);
		const double length_in_seconds	= total_frames / input_fps;
		const int desired_width			= std::round(zoom_factor * original_width);
		const int desired_height		= std::round(zoom_factor * original_height);
		const size_t frame_length_ns	= std::round(1000000000.0 / input_fps);
		const auto desired_frame_size	= cv::Size(desired_width, desired_height);

		std::cout
			<< ""																						<< std::endl
			<< "Input video .......... " << argv[video_index]											<< std::endl
			<< "Number of frames ..... " << total_frames												<< std::endl
			<< "Length of video ...... " << length_in_seconds				<< " seconds"				<< std::endl
			<< "Zoom factor .......... " << zoom_factor													<< std::endl
			<< "Original dimensions .. " << original_width					<< "x" << original_height	<< std::endl
			<< "Desired dimensions ... " << desired_width					<< "x" << desired_height	<< std::endl
			<< "Frame rate ........... " << input_fps						<< " FPS"					<< std::endl
			<< "Frame interval ....... " << frame_length_ns					<< " nanoseconds"			<< std::endl
			<< "Frame interval ....... " << (frame_length_ns / 1000000.0)	<< " milliseconds"			<< std::endl;

		MoveDetect::Handler movement_detection;
		movement_detection.mask_enabled				= true;
		movement_detection.bbox_enabled				= true;
		movement_detection.contours_enabled			= true;
		movement_detection.contours_size			= 4;

		// If you are generating either the contours or the bounding boxes,
		// then you'll want to increase the frequency and keep more frames.
		movement_detection.key_frame_frequency		= 1;
		movement_detection.number_of_control_frames	= 10;

		// Larger "thumbnails" improves precision, but takes longer to process each frame.
		movement_detection.thumbnail_ratio			= 0.25;

		// More expensive but slightly prettier anti-aliased lines.
		movement_detection.line_type				= cv::LINE_AA;

		// Lower threshold ignore smaller changes, while higher threshold will trigger on smaller movement changes.
//		movement_detection.psnr_threshold			= 28.0;

		// Create a single large mat.  This will be used to combine the 3 images (original frame, mask, output).
		cv::Mat mat(desired_height, 3 * desired_width, CV_8UC3);

		// create 3 RoI which we'll use to copy the specific images into the large mat defined on the line above
		cv::Mat frame	= mat(cv::Rect({desired_width * 0, 0}, desired_frame_size));
		cv::Mat mask	= mat(cv::Rect({desired_width * 1, 0}, desired_frame_size));
		cv::Mat output	= mat(cv::Rect({desired_width * 2, 0}, desired_frame_size));

		cv::VideoWriter video_output;
		if (save_output_video)
		{
			video_output.open("output_" + std::to_string(std::time(nullptr)) + ".mp4", cv::VideoWriter::fourcc('m', 'p', '4', 'v'), input_fps, mat.size());
		}

		const std::chrono::high_resolution_clock::duration		duration				= std::chrono::nanoseconds(frame_length_ns);
		const std::chrono::high_resolution_clock::time_point	start_time				= std::chrono::high_resolution_clock::now();
		std::chrono::high_resolution_clock::time_point			next_frame_time_point	= start_time + duration;

		size_t frame_index = 0;
		while (true)
		{
			cv::Mat tmp;
			video_input >> tmp;
			if (tmp.empty())
			{
				break;
			}

			cv::resize(tmp, frame, desired_frame_size, 0, 0, cv::INTER_LINEAR);

			const bool moved = movement_detection.detect(frame);
			if (movement_detection.transition_detected)
			{
				std::cout << "-> starting at index #" << frame_index << ": moved=" << (moved ? "TRUE" : "FALSE") << std::endl;
			}

			// The mask is a binary image.  We need to convert it to BGR so
			// we can combine it with our large mat that we'll then display.
			cv::cvtColor(movement_detection.mask, tmp, cv::COLOR_GRAY2BGR);
			tmp.copyTo(mask);

			movement_detection.output.copyTo(output);

			cv::imshow("MoveDetect", mat);

			if (video_output.isOpened())
			{
				video_output.write(mat);
			}

//			cv::imwrite("frame_" + std::to_string(frame_index) + ".png", mat, {cv::ImwriteFlags::IMWRITE_PNG_COMPRESSION, 7});
//			cv::imwrite("frame_" + std::to_string(frame_index) + ".jpg", mat, {cv::ImwriteFlags::IMWRITE_JPEG_QUALITY, 70});

			// wait for the right amount of time so the video is played back at the right FPS
			const std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
			const int milliseconds_to_wait = std::chrono::duration_cast<std::chrono::milliseconds>(next_frame_time_point - now).count();
			if (milliseconds_to_wait > 0)
			{
				cv::waitKey(milliseconds_to_wait);
			}
			next_frame_time_point += duration;

			if (now > next_frame_time_point)
			{
				// we've fallen too far behind, reset the time we need to show the next frame
				next_frame_time_point = now + duration;
			}

			frame_index ++;
		}

		const std::chrono::high_resolution_clock::time_point end_time = std::chrono::high_resolution_clock::now();

		std::cout
			<< "-> processed " << frame_index << " frames" << std::endl
			<< "-> time elapsed: " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << " milliseconds" << std::endl;
	}

	return 0;
}
