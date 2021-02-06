# MoveDetect - C++ library to detect movement.
# Copyright 2021 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.

# static library
ADD_LIBRARY (movedetect STATIC MoveDetect.cpp)
INSTALL (TARGETS movedetect DESTINATION lib)
INSTALL (FILES MoveDetect.hpp DESTINATION include)
