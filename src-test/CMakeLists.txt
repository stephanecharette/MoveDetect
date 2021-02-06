# MoveDetect - C++ library to detect movement.
# Copyright 2021 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.

# test application
ADD_EXECUTABLE (movement_detection main.cpp)
TARGET_LINK_LIBRARIES (movement_detection PRIVATE Threads::Threads ${OpenCV_LIBS} movedetect)

