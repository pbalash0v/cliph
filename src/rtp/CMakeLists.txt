cmake_minimum_required(VERSION 3.18)

target_sources(${PROJECT_NAME} PRIVATE
	rtp.cpp
	rtp.hpp
	rtcp.hpp
	rtcp.cpp
	rtp_stream.hpp
	rtp_stream.cpp
)
target_include_directories(${PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_LIST_DIR}
)
