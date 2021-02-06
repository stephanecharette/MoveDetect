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

# create .deb and source tarballs
SET (CPACK_PACKAGE_VENDOR				"Stephane Charette")
SET (CPACK_PACKAGE_CONTACT				"stephanecharette@gmail.com")
SET (CPACK_PACKAGE_NAME					"movedetect")
SET (CPACK_PACKAGE_VERSION				"0.0.1")
SET (CPACK_PACKAGE_VERSION_MAJOR		"0")
SET (CPACK_PACKAGE_VERSION_MINOR		"0")
SET (CPACK_PACKAGE_VERSION_PATCH		"1")
SET (CPACK_PACKAGE_DESCRIPTION_SUMMARY	"MoveDetect C++ library")
SET (CPACK_PACKAGE_DESCRIPTION			"MoveDetect C++ library")
SET (CPACK_PACKAGE_HOMEPAGE_URL			"https://www.ccoderun.ca/")
SET (CPACK_GENERATOR					"DEB")
SET (CPACK_SOURCE_IGNORE_FILES			".svn" ".kdev4" "build/" "build_and_upload" )
SET (CPACK_SOURCE_GENERATOR				"TGZ;ZIP")
INCLUDE( CPack )
