# CMakeList.txt : CMake project for KinectInfraredCam, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Point CMake to the custom modules

project (KinectInfraredCam VERSION 0.0.1)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

find_package(Kinect 1.8 REQUIRED)
find_package(DirectShow REQUIRED)

set(KinectInfraredCam_HEADERS
	"KinectRGBCam.h"
	KinectVirtualCamera.h "util.h")

set(KinectInfraredCam_SOURCES
    "KinectRGBCam.cpp"
	KinectRGBCam.def # Module definition for exports
	KinectVirtualCamera.cpp
	Dll.cpp)

# Add source to this project's executable.

message("Found base classes ${DIRECTSHOW_BASECLASS_DIR}")
add_library(KinectInfraredCam MODULE ${KinectInfraredCam_HEADERS} ${KinectInfraredCam_SOURCES})
target_include_directories(KinectInfraredCam SYSTEM PRIVATE ${KINECT_INCLUDE_DIR} ${DIRECTSHOW_INCLUDE_DIRS})
target_link_libraries(KinectInfraredCam ${KINECT_LIBRARIES} ${DIRECTSHOW_STRMIIDS_LIBRARY} legacy_stdio_definitions.lib)
target_link_libraries(KinectInfraredCam winmm debug ${DIRECTSHOW_BASECLASS_DIR}/strmbasd.lib optimized strmbase)
set_target_properties(KinectInfraredCam PROPERTIES SUFFIX .ax COMPILE_FLAGS "/Zc:wchar_t-")

# Simple scripts to enable/disable filter registration
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/Reg.cmd" "%WINDIR%\\System32\\regsvr32.exe %~dp0\\KinectInfraredCam.ax")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/UnReg.cmd" "%WINDIR%\\System32\\regsvr32.exe /u %~dp0\\KinectInfraredCam.ax")

# Specify files to be installed
install(TARGETS KinectInfraredCam LIBRARY DESTINATION .)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/Reg.cmd"
    "${CMAKE_CURRENT_BINARY_DIR}/UnReg.cmd"
    DESTINATION .)

include(CPack)
