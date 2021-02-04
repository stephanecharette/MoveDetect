// MoveDetect -- Library to detect whether movement can be detected between two images or video frames.
// See LICENSE.TXT for details.
// $Id$

#pragma once

#include <chrono>
#include <map>
#include <opencv2/opencv.hpp>


namespace MoveDetect
{
	/** Peak Signal To Noise is an algorithm which as a side effect can be used to compare two similar images.
	 *
	 * Source:  https://docs.opencv.org/master/d5/dc4/tutorial_video_input_psnr_ssim.html
	 *
	 * @return Values higher than 30 indicate very similar images.  Values < 30 indicate changes were detected.
	 * The closer to zero, the more changes have been detected.
	 */
	double psnr(const cv::Mat & src, const cv::Mat & dst);


	/** Simple colour balancing.
	 *
	 * Source:  https://stackoverflow.com/a/49481583/13022
	 * Source:  http://web.stanford.edu/~sujason/ColorBalancing/simplestcb.html
	 * Source:  http://www.morethantechnical.com/2015/01/14/simplest-color-balance-with-opencv-wcode/
	 */
	cv::Mat simple_colour_balance(const cv::Mat & src);


	/** We keep several control thumbnail images to compare against.  This typedef defines the map that tracks these
	 * thumbnails.  The key is the frame index, the value is the thumbnail itself.
	 */
	typedef std::map<size_t, cv::Mat> ControlMap;

	class Handler
	{
		public:

			/// Constructor.
			Handler();

			/// Destructor.
			virtual ~Handler();

			/// Determine if this handler has any images against which to check for movement.
			bool empty() const;

			/// Clear out the object and get it ready to be reused.
			Handler & clear();

			/** Detect whether there is any movement in this next image.  This assumes the images are sequential.  If the image
			 * provided is not the next sequential image, then use the other @ref detect() where a frame index is provided.
			 */
			bool detect(cv::Mat & next_image);

			/** Detect whether there is any movement in an arbritrary image frame.
			 * The frame index must be greater than or equal to @ref next_frame_index.
			 */
			bool detect(const size_t frame_index, cv::Mat & image);

			/// The index of the next frame expected to be seen by @ref detect().
			size_t next_frame_index;

			/** The next index where we want to store a thumbnail in the @ref control object.  This is handled internally,
			 * and users of this library shouldn't have to modify or reference this value.  @see @ref key_frame_frequency
			 */
			size_t next_key_frame;

			/** How often new frames inserted into @ref control.  Default value is @p 15, which for videos recorded at 30 FPS
			 * means every 1/2 second a new image is added to @ref control.
			 */
			size_t key_frame_frequency;

			/** The threshold value below which it is assumed that movement has been detected.
			 * Default value is @p 32.0.  @see @ref psnr()
			 */
			double psnr_threshold;

			/// The PSNR value received from the most recent call to @ref detect().
			double psnr_score;

			/** The width of the thumbnail to generate from the images passed to @ref detect().  The height will be determined
			 * automatically to keep the aspect ratio consistent.  The larger the thumbnail, the more precise the detection,
			 * but the more processing needs to be done for every frame.  And if it is made too large, the it will pick up
			 * image artifacts or subtle tiny changes and think that movement was detected.  Default value is @p 24.
			 */
			size_t thumbnail_width;

			/// The most recent frame index where movement was detected.  @see @ref movement_last_detected
			size_t frame_index_with_movement;

			/** The number of frames to which we want to hold on to in @ref control.  Images will be compared against all these
			 * control frames to determine if there was any movement, so you'll want to keep it short.  Default value is @p 2.
			 */
			size_t number_of_control_frames;

			/// The timestamp when @ref detect() last returned @p true.
			std::chrono::high_resolution_clock::time_point movement_last_detected;

			/// The size of the thumbnail that will be generated.  Instead of modifying this value, see @ref thumbnail_width.
			cv::Size thumbnail_size;

			/** All of the key control thumbnail images are stored in this map.  When calling @ref detect(), the images are
			 * compared against the thumbnails stored within.
			 */
			ControlMap control;

			/// Set to @p true to get the library to calculate a mask showing where movement was detected.
			bool calculate_mask;

			/// When @ref calculate_mask is enabled, the results are saved to @p mask.
			cv::Mat mask;
	};
}
