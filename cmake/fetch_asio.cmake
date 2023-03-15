cmake_minimum_required(VERSION 3.18)
include_guard()

#################################
# Find deps
#################################
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

############################################################
# Pull and build local dependencies
############################################################
include(ExternalProject)

set(LOCAL_BUILD_ARTIFACTS_DIR ${CMAKE_BINARY_DIR}/build_artifacts)

# Fetch asio and define ASIO lib
ExternalProject_Add(asio
	GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
	GIT_TAG	asio-1-27-0
	GIT_PROGRESS ON
	GIT_SHALLOW ON

	INSTALL_DIR ${LOCAL_BUILD_ARTIFACTS_DIR}
	BUILD_COMMAND ""
	UPDATE_COMMAND ""
	CONFIGURE_COMMAND ""
	INSTALL_COMMAND ""
)
ExternalProject_Get_property(asio SOURCE_DIR)
set(ASIO_SOURCE "${SOURCE_DIR}")

#
# Define cmake-usable ASIO
#
set(TARGET_NAME libasio_standalone)
add_library(${TARGET_NAME} INTERFACE)
add_library(asio::asio ALIAS ${TARGET_NAME})
target_compile_definitions(${TARGET_NAME} INTERFACE ASIO_STANDALONE)
target_include_directories(${TARGET_NAME} INTERFACE
	${ASIO_SOURCE}/asio/include
)
target_link_libraries(${TARGET_NAME} INTERFACE
	Threads::Threads
)

