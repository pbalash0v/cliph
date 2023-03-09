cmake_minimum_required(VERSION 3.18)
include_guard()

include(ExternalProject)

ExternalProject_Add(miniaudio_
	GIT_REPOSITORY https://github.com/mackron/miniaudio.git
	GIT_TAG a0dc1037f99a643ff5fad7272cd3d6461f2d63fa #0.11.11
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

