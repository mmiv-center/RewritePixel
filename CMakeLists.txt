cmake_minimum_required (VERSION 2.8.11)

project (rewritepixel)

# set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD 17)
#SET(CMAKE_C_FLAGS "-std=c++17")

FIND_LIBRARY(LIBXML2_LIBRARY NAMES xml2)
FIND_LIBRARY(JPEG_LIBRARY NAMES jpeg)
FIND_LIBRARY(ZLIB_LIBRARY NAMES z)
FIND_LIBRARY(XLST_LIBRARY NAMES exslt)

message(STATUS LIBXML2_LIBRARY = ${LIBXML2_LIBRARY})
message(STATUS JPEG_LIBRARY = ${JPEG_LIBRARY})
message(STATUS ZLIB_LIBRARY = ${ZLIB_LIBRARY})
message(STATUS XLST_LIBRARY = ${XLST_LIBRARY})

add_executable (rewritepixel rewritepixel.cxx ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories (rewritepixel PUBLIC
			   ${CMAKE_CURRENT_SOURCE_DIR}/gdcm-build/Source/DataStructureAndEncodingDefinition
			   ${CMAKE_CURRENT_SOURCE_DIR}/gdcm-build/Source/Common
			   ${CMAKE_CURRENT_SOURCE_DIR}/GDCM-3.0.4/Source/Common
			   ${CMAKE_CURRENT_SOURCE_DIR}/GDCM-3.0.4/Source/MediaStorageAndFileFormat/
			   ${CMAKE_CURRENT_SOURCE_DIR}/GDCM-3.0.4/Source/InformationObjectDefinition/
			   ${CMAKE_CURRENT_SOURCE_DIR}/GDCM-3.0.4/Source/DataDictionary/
			   ${CMAKE_CURRENT_SOURCE_DIR}/GDCM-3.0.4/bin/Source/Common
			   ${CMAKE_CURRENT_SOURCE_DIR}/GDCM-3.0.4/Source/DataStructureAndEncodingDefinition
			   /usr/local/Cellar/leptonica/1.78.0_1/include
			   /usr/local/Cellar/tesseract/4.1.1/include/ ${LIBXML2_INCLUDE_DIR})

SET(GDCM_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/gdcm-build/bin/)
SET(LEPTONICA_LIB_PATH /usr/local/Cellar/leptonica/1.78.0_1/lib)
link_directories(rewritepixel ${CMAKE_CURRENT_SOURCE_DIR}/gdcm-build/bin/)
FIND_LIBRARY(COMMON_LIBRARY       NAMES gdcmCommon       PATHS ${GDCM_LIB_PATH} NO_DEFAULT_PATH)
FIND_LIBRARY(IOD_LIBRARY       NAMES gdcmIOD       PATHS ${GDCM_LIB_PATH} NO_DEFAULT_PATH)
FIND_LIBRARY(MSFF_LIBRARY       NAMES gdcmMSFF       PATHS ${GDCM_LIB_PATH} NO_DEFAULT_PATH)
FIND_LIBRARY(DSED_LIBRARY       NAMES gdcmDSED       PATHS ${GDCM_LIB_PATH} NO_DEFAULT_PATH)
FIND_LIBRARY(DICT_LIBRARY       NAMES gdcmDICT       PATHS ${GDCM_LIB_PATH} NO_DEFAULT_PATH)

# - Try to find Tesseract-OCR
# Once done, this will define
#
#  Tesseract_FOUND - system has Tesseract
#  Tesseract_INCLUDE_DIRS - the Tesseract include directories
#  Tesseract_LIBRARIES - link these to use Tesseract

include(LibFindMacros.cmake)

# Use pkg-config to get hints about paths
#libfind_pkg_check_modules(Tesseract_PKGCONF Tesseract)

# Include dir
find_path(Tesseract_INCLUDE_BASEAPI_DIR
  NAMES tesseract/baseapi.h
  HINTS "/usr/include"
        "/usr/include/tesseract"
        "/usr/local/include"
        "/usr/local/include/tesseract"
        "/opt/local/include"
        "/opt/local/include/tesseract"
        ${Tesseract_PKGCONF_INCLUDE_DIRS}
		${CMAKE_SOURCE_DIR}/../libraries/tesseract-ocr/api/
)
find_path(Tesseract_INCLUDE_CCSTRUCT_DIR
  NAMES publictypes.h
  HINTS "/usr/include"
        "/usr/include/tesseract"
        "/usr/local/include"
        "/usr/local/include/tesseract"
        "/opt/local/include"
        "/opt/local/include/tesseract"
        ${Tesseract_PKGCONF_INCLUDE_DIRS}
		${CMAKE_SOURCE_DIR}/../libraries/tesseract-ocr/ccstruct/
)
find_path(Tesseract_INCLUDE_CCMAIN_DIR
  NAMES thresholder.h
  HINTS "/usr/include"
        "/usr/include/tesseract"
        "/usr/local/include"
        "/usr/local/include/tesseract"
        "/opt/local/include"
        "/opt/local/include/tesseract"
        ${Tesseract_PKGCONF_INCLUDE_DIRS}
		${CMAKE_SOURCE_DIR}/../libraries/tesseract-ocr/ccmain/
)
find_path(Tesseract_INCLUDE_CCUTIL_DIR
  NAMES platform.h
  HINTS "/usr/include"
        "/usr/include/tesseract"
        "/usr/local/include"
        "/usr/local/include/tesseract"
        "/opt/local/include"
        "/opt/local/include/tesseract"
        ${Tesseract_PKGCONF_INCLUDE_DIRS}
		${CMAKE_SOURCE_DIR}/../libraries/tesseract-ocr/ccutil/
)


# Finally the library itself
find_library(Tesseract_LIB
  NAMES tesseract libtesseract tesseract-static libtesseract303-static
  HINTS "/usr/lib"
        "/usr/local/lib"
        "/opt/local/lib"
        ${Tesseract_PKGCONF_LIBRARY_DIRS}
		${CMAKE_SOURCE_DIR}/../libraries/tesseract-ocr/api/.libs
		${CMAKE_SOURCE_DIR}/../libraries/tesseract-ocr/vs2010/LIB_Release
)

find_library(Leptonica_LIB
  NAMES liblept170 liblept lept
  HINTS "/usr/lib"
        "/usr/local/lib"
        "/opt/local/lib"
        ${Tesseract_PKGCONF_LIBRARY_DIRS}
		${CMAKE_SOURCE_DIR}/../libraries/tesseract-ocr/api/.libs
		${CMAKE_SOURCE_DIR}/../libraries/tesseract-ocr/vs2010/LIB_Release
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(Tesseract_PROCESS_INCLUDES 
    Tesseract_INCLUDE_BASEAPI_DIR 
	Tesseract_INCLUDE_CCSTRUCT_DIR
	Tesseract_INCLUDE_CCMAIN_DIR
	Tesseract_INCLUDE_CCUTIL_DIR
	Tesseract_INCLUDE_DIRS)
set(Tesseract_PROCESS_LIBS Tesseract_LIB Leptonica_LIB Tesseract_LIBRARIES)
libfind_process(Tesseract)

#message(AUTHOR_WARNING ${COMMON_LIBRARY})
#message(AUTHOR_WARNING ${IOD_LIBRARY})
message(STATUS COMMON_LIBRARY = ${COMMON_LIBRARY})


target_link_libraries(rewritepixel ${COMMON_LIBRARY} ${IOD_LIBRARY} ${MSFF_LIBRARY} ${DICT_LIBRARY} ${DSED_LIBRARY} ${LIBXML2_LIBRARY} ${JPEG_LIBRARY} ${ZLIB_LIBRARY} ${XLST_LIBRARY} ${Tesseract_LIBRARIES} pthread)
