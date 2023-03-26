cmake_minimum_required(VERSION 3.18)

set(TARGET_NAME rtpp)
add_library(${TARGET_NAME})
target_sources(${TARGET_NAME} PRIVATE
	rtp.cpp
	rtp.hpp
	rtcp.hpp
	rtcp.cpp
	rtp_stream.hpp
	rtp_stream.cpp
)

set_target_properties(${TARGET_NAME} PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)
target_compile_features(${TARGET_NAME} PUBLIC
	cxx_std_17
)
target_compile_options(${TARGET_NAME} PRIVATE
	$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror>
)
target_include_directories(${TARGET_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
