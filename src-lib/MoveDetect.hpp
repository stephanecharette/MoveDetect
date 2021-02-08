// MoveDetect -- Library to detect whether movement can be detected between two images or video frames.
// Copyright 2021 Stephane Charette <stephanecharette@gmail.com>
// MIT license applies.  See "license.txt" for details.

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
	 * @li Source:  https://stackoverflow.com/a/49481583/13022
	 * @li Source:  http://web.stanford.edu/~sujason/ColorBalancing/simplestcb.html
	 * @li Source:  http://www.morethantechnical.com/2015/01/14/simplest-color-balance-with-opencv-wcode/
	 */
	cv::Mat simple_colour_balance(const cv::Mat & src);


	/** We keep several control thumbnail images to compare against.  This typedef defines the map that tracks these
	 * thumbnails.  The key is the frame index, the value is the thumbnail itself.
	 */
	typedef std::map<size_t, cv::Mat> ControlMap;

	/** This class is used to store some image thumbnails, configuration settings, and also contains the @ref detect()
	 * method which is used to determine if a video frame has movement.  @see @ref Summary
	 */
	class Handler
	{
		public:

			/// Constructor.
			Handler();

			/// Destructor.
			virtual ~Handler();

			/// Determine if this handler has any images against which to check for movement.
			bool empty() const;

			/** Clear out the object and get it ready to be reused.  This will reset all the configuration settings to their
			 * default values.
			 */
			Handler & clear();

			/** Detect whether there is any movement in this next image.  This assumes the images or frames are sequential.
			 * If the image provided is not the next sequential image, then use the other @ref detect() where a frame index
			 * is provided.
			 *
			 * @return @p true if movement is detected, otherwise returns @p false.
			 *
			 * @warning This method adds and removes image thumbnails from a @p std::map, so it must not be called simultaneously
			 * from multiple threads.
			 *
			 * @see @ref movement_detected
			 * @see @ref transition_detected
			 *
			 * Given this video frame of someone walking across a parking lot:
			 *
			 * @image html movement_frame.png
			 *
			 * When @ref contours_enabled and/or @ref bbox_enabled have been enabled, in addition to returning @p true to indicate
			 * that movement was detected, the @ref output will be similar to this:
			 *
			 * @image html movement_with_contour_and_bbox.png
			 */
			bool detect(cv::Mat & next_image);

			/** Detect whether there is any movement in an arbritrary image frame.
			 * The frame index must be greater than or equal to @ref next_frame_index.
			 *
			 * @warning This method adds and removes image thumbnails from a @p std::map, so it must not be called simultaneously
			 * from multiple threads.
			 *
			 * @return @p true if movement is detected, otherwise returns @p false.
			 *
			 * @see @ref movement_detected
			 * @see @ref transition_detected
			 */
			bool detect(const size_t frame_index, cv::Mat & image);

			/// This is the same as the value returne by @ref detect() and indicates whether the last image seen showed movement.
			bool movement_detected;

			/** When @ref movement_detected is toggled between @p true and @p false, the @p transition_detected variable will be
			 * set to @p true.  Callers can use this to determine when a video experiences a significant state change.
			 */
			bool transition_detected;

			/// The index of the next frame expected to be seen by @ref detect().
			size_t next_frame_index;

			/** The next index when this library will store a thumbnail in the @ref control object.  This is handled internally,
			 * and users of the library shouldn't have to modify or reference this value.  @see @ref key_frame_frequency
			 */
			size_t next_key_frame;

			/** This determines how often new frames are inserted into @ref control.  Default value is @p 10, which for videos
			 * recorded at 30 FPS means 3 key frames per second.  If masking is enabled (@ref mask_enabled) and the mask is
			 * displayed or used to show bounding boxes, then this value should be small to record many key frames per second.
			 * If masking is disabled, then this value may be set to a much higher value, such as the same as the original FPS.
			 *
			 * @see @ref number_of_control_frames
			 */
			size_t key_frame_frequency;

			/** The number of frames to which we want to hold on to in @ref control.  Images will be compared against all these
			 * control frames to determine if there was any movement, so you'll want to keep it short.  Default value is @p 4.
			 */
			size_t number_of_control_frames;

			/** The threshold value below which it is assumed that movement has been detected.
			 * Default value is @p 32.0.  @see @ref psnr()
			 */
			double psnr_threshold;

			/// The PSNR value received from the most recent call to @ref detect().  @see @ref psnr()
			double most_recent_psnr_score;

			/** How much the image will be reduced to generate the thumbnails.  The larger the thumbnail, the more precise the
			 * detection, but the more processing needs to be done for every frame.  And if it is made too large, the library
			 * will pick up image artifacts or subtle tiny changes and think that movement was detected.
			 * Default value is @p 0.05, meaning the width and height will be reduced to 5% of the original image size.
			 */
			double thumbnail_ratio;

			/// The size of the thumbnail that will be generated.  Instead of modifying this value, see @ref thumbnail_ratio.
			cv::Size thumbnail_size;

			/// The most recent frame index where movement was detected.  @see @ref movement_last_detected
			size_t frame_index_with_movement;

			/// The timestamp when @ref detect() last returned @p true.
			std::chrono::high_resolution_clock::time_point movement_last_detected;

			/** All of the key control thumbnail images are stored in this map.  When calling @ref detect(), the images are
			 * compared against the thumbnails stored within.
			 */
			ControlMap control;

			/** Set to @p true to get the library to create a mask showing where movement was detected.
			 * The default value for @p mask_enabled is @p false.  @see @ref mask
			 */
			bool mask_enabled;

			/** When @ref mask_enabled is enabled, the result is saved to @p mask.  This is a binary 1-channel image,
			 * OpenCV type @p CV_8UC1, where @p 0 is the background and @p 1 is where movement is detected.
			 *
			 * Given this video frame of someone walking across a parking lot:
			 *
			 * @image html movement_frame.png
			 *
			 * When @p mask_enabled has been set, in addition to @ref detect() returning @p true when movement is detected
			 * the @p mask will look similar to this:
			 *
			 * @image html movement_mask.png
			 */
			cv::Mat mask;

			/** The type of line type drawing OpenCV will use if @ref contours_enabled or @ref bbox_enabled have been set.
			 * Set this to @p cv::LINE_AA to get prettier anti-alised lines.  Default is @p cv::LINE_4.
			 */
			cv::LineTypes line_type;

			/** Set to @p true to get the library to draw the mask contours onto the output image.  If enabled,
			 * this will also enable @ref mask_enabled as it requires the @ref mask.  @see @ref output
			 *
			 * Given this video frame of someone walking across a parking lot:
			 *
			 * @image html movement_frame.png
			 *
			 * When @p contours_enabled has been set, in addition to @ref detect() returning @p true when movement is detected
			 * the output image will look similar to this:
			 *
			 * @image html movement_with_contour.png
			 */
			bool contours_enabled;

			/** The width of the line to use when drawing contours.  This is only used when @ref contours_enabled has been set.
			 * The default contours size is @p 1.  @see @ref output
			 */
			int contours_size;

			/** Set to @p true to get the library to draw a bounding box around any detected movement.  If enabled,
			 * this will also enable @ref mask_enabled as it requires the @ref mask.  @see @ref output
			 *
			 * Given this video frame of someone walking across a parking lot:
			 *
			 * @image html movement_frame.png
			 *
			 * When @p bbox_enabled has been set, in addition to @ref detect() returning @p true when movement is detected the
			 * output image will look similar to this:
			 *
			 * @image html movement_with_bbox.png
			 */
			bool bbox_enabled;

			/** The width of the line to use when drawing the bounding box.  This is only used when @ref bbox_enabled has been
			 * set.  The default line size is @p 1.  @see @ref output
			 */
			int bbox_size;

			/** When either @ref contours_enabled or @ref bbox_enabled is enabled, the resulting image is stored in @p output.
			 *
			 * Given this video frame of someone walking across a parking lot:
			 *
			 * @image html movement_frame.png
			 *
			 * When both @ref contours_enabled and @ref bbox_enabled have been set, in addition to @ref detect() returning
			 * @p true when movement is detected the output image will look similar to this:
			 *
			 * @image html movement_with_contour_and_bbox.png
			 */
			cv::Mat output;
	};
}
