// MoveDetect -- Library to detect whether movement can be detected between two images or video frames.
// See LICENSE.TXT for details.
// $Id: $

#include "MoveDetect.hpp"


int main(int argc, char *argv[])
{
	std::cout << "Movement Detection" << std::endl;

	if (argc < 2)
	{
		std::cout
			<< "Usage:" << std::endl
			<< "\t" << argv[0] << " <video1> [<video2> ...]" << std::endl;
		return 1;
	}

	for (int video_index = 1; video_index < argc; video_index ++)
	{
		cv::VideoCapture video(argv[video_index]);
		if (video.isOpened() == false)
		{
			std::cout << "ERROR: failed to open " << argv[video_index] << std::endl;
			continue;
		}

		const double input_fps		= video.get(cv::VideoCaptureProperties::CAP_PROP_FPS			);
		const double input_width	= video.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_WIDTH	);
		const double input_height	= video.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_HEIGHT	);

		std::cout
			<< "Input video ... " << argv[video_index]					<< std::endl
			<< "Dimensions .... " << input_width << "x" << input_height	<< std::endl
			<< "Frame rate .... " << input_fps							<< std::endl;

		MoveDetect::Handler movement_detection;

		bool previous_value = false;

		while (true)
		{
			cv::Mat mat;
			video >> mat;
			if (mat.empty())
			{
				break;
			}

			const bool moved = movement_detection.detect(mat);

			if (previous_value != moved)
			{
				std::cout << "-> starting at index #" << (movement_detection.next_frame_index - 1) << ": moved=" << (moved ? "TRUE" : "FALSE") << std::endl;
				previous_value = moved;
			}

			if (movement_detection.next_frame_index >= 200)
			{
				break;
			}
		}
	}

	return 0;
}
