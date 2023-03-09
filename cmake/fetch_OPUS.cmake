cmake_minimum_required(VERSION 3.18)
include_guard()

include(FetchContent)

message(STATUS "Standby, pulling OPUS...")
FetchContent_Declare(opus_
	GIT_REPOSITORY https://github.com/xiph/opus.git
	GIT_TAG v1.3.1
	GIT_PROGRESS ON
)
FetchContent_MakeAvailable(opus_)

