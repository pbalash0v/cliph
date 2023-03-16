cmake_minimum_required(VERSION 3.18)

#################################
# Find deps
#################################
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

#################################
# Define targets
#################################
set(TARGET_NAME ${PROJECT_NAME}) 
add_executable(${TARGET_NAME})
target_sources(${TARGET_NAME} PRIVATE
	main.cpp
	audio_engine.cpp
	sip_agent.cpp
	net.cpp
)
add_subdirectory(rtp)
#
target_link_libraries(${TARGET_NAME} PRIVATE
	asio::asio
	${CMAKE_DL_LIBS}
	Threads::Threads
	miniaudio
	opus
	resip
	dum
)
set_target_properties(${TARGET_NAME} PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
#	INTERPROCEDURAL_OPTIMIZATION ON	
)
target_compile_options(${TARGET_NAME} PRIVATE
	$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall> #-Wextra -pedantic># -Werror>
)
target_link_options(${TARGET_NAME} PRIVATE
#	$<BUILD_INTERFACE:$<$<CXX_COMPILER_ID:Clang>:-stdlib=libc++>>
)

