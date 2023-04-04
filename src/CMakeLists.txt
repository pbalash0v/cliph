cmake_minimum_required(VERSION 3.18)

#################################
# Find deps
#################################
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_subdirectory(rtpp)
#################################
# Define targets
#################################
set(TARGET_NAME ${PROJECT_NAME}) 
add_executable(${TARGET_NAME})
target_sources(${TARGET_NAME} PRIVATE
	q_test.cpp
#	main.cpp
#	audio_engine.cpp
#	sip_agent.cpp
#	net.cpp
)

#
target_link_libraries(${TARGET_NAME} PRIVATE
	asio::asio
	${CMAKE_DL_LIBS}
	Threads::Threads
	miniaudio
	opus
	resip
	dum
	argh
	rtpp
)
set_target_properties(${TARGET_NAME} PROPERTIES
	CXX_STANDARD 20#17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
#	$<$<CONFIG:Release>:INTERPROCEDURAL_OPTIMIZATION ON>
)
target_compile_options(${TARGET_NAME} PRIVATE
	$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall> #-Wextra -pedantic># -Werror>
)

set(is_gnu "$<CXX_COMPILER_ID:GNU>")
set(is_clang "$<CXX_COMPILER_ID:Clang>")
set(meet_requirements "$<OR:${is_gnu},${is_clang}>")
target_link_options(${TARGET_NAME} PRIVATE
#	$<$<CXX_COMPILER_ID:Clang>:-stdlib=libc++>
#	$<$<CXX_COMPILER_ID:GNU>:-static-libgcc -static-libstdc++>
	$<${meet_requirements}:-static-libgcc -static-libstdc++>
)

