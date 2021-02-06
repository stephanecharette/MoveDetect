# MoveDetect - C++ library to detect movement.
# Copyright 2021 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.


CMAKE_MINIMUM_REQUIRED (VERSION 3.0)

PROJECT (MoveDetect C CXX)

IF (NOT DEFINED CMAKE_BUILD_TYPE)
	SET (CMAKE_BUILD_TYPE Release)
ENDIF ()

SET (CMAKE_CXX_STANDARD				17	)
SET (CMAKE_CXX_STANDARD_REQUIRED	ON	)
SET (CMAKE_CXX_EXTENSIONS			OFF	)

ADD_DEFINITIONS ("-Wall -Wextra -Werror -Wno-unused-parameter")

FIND_PACKAGE (Threads	REQUIRED)
FIND_PACKAGE (OpenCV	REQUIRED)

INCLUDE_DIRECTORIES (${OpenCV_INCLUDE_DIRS})
INCLUDE_DIRECTORIES (src-lib)

ADD_SUBDIRECTORY (src-dox)
ADD_SUBDIRECTORY (src-lib)
ADD_SUBDIRECTORY (src-test)

