cmake_minimum_required(VERSION 2.6)
PROJECT(CalibTool)

find_package (OpenCV REQUIRED)
if (OpenCV_FOUND)
  include_directories(${OpenCV_INCLUDE_DIRS})
  set(LIBS ${LIBS} ${OpenCV_LIBS})
  message(STATUS ${OpenCV_INCLUDE_DIRS})
  message(STATUS ${LIBS})
endif (OpenCV_FOUND)

ADD_LIBRARY(calib STATIC src/calib.cpp)
ADD_EXECUTABLE( gencalib src/gencalib.cpp )
TARGET_LINK_LIBRARIES ( gencalib calib ${LIBS})