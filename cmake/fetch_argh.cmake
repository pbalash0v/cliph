cmake_minimum_required(VERSION 3.18)
include_guard()

include(FetchContent)

message(STATUS "Standby, pulling argh...")
FetchContent_Declare(argh_
	GIT_REPOSITORY https://github.com/adishavit/argh.git
	GIT_TAG v1.3.2
	GIT_PROGRESS ON
)
FetchContent_MakeAvailable(argh_)

