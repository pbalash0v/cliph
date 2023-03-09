cmake_minimum_required(VERSION 3.18)

project(cliphone
	VERSION 0.1.0
	LANGUAGES C CXX
)


# pull third pty libs
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(setup_build)
include(fetch_miniaudio)
include(fetch_OPUS)
include(fetch_reSIP)

add_subdirectory(src)

