cmake_minimum_required(VERSION 3.18)
include_guard()

include(ExternalProject)

ExternalProject_Add(miniaudio_
	GIT_REPOSITORY https://github.com/mackron/miniaudio.git
	GIT_TAG 0a19c7441782fa6713b0f960458bfac59d14d876 #0.11.12
	GIT_PROGRESS ON

	BUILD_COMMAND ""
	CONFIGURE_COMMAND ""
	UPDATE_COMMAND ""
	INSTALL_COMMAND ""
)
# define header only miniaudio
ExternalProject_Get_property(miniaudio_ SOURCE_DIR)

add_library(miniaudio INTERFACE)
target_include_directories(miniaudio SYSTEM INTERFACE
	$<BUILD_INTERFACE:${SOURCE_DIR}>
)

