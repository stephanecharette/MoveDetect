# MoveDetect
# See LICENSE.TXT for details.

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

# static library
ADD_LIBRARY (movedetect STATIC MoveDetect.cpp)
INSTALL (TARGETS movedetect DESTINATION lib)

# test application
ADD_EXECUTABLE (movement_detection main.cpp)
TARGET_LINK_LIBRARIES (movement_detection PRIVATE Threads::Threads ${OpenCV_LIBS} movedetect)
INSTALL (TARGETS movement_detection DESTINATION bin)
