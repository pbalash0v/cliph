cmake_minimum_required(VERSION 3.18)

#################################
# Find deps
#################################
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

#################################
# Define targets
#################################
set(TARGET_NAME ${PROJECT_NAME}_utils)
add_library(${TARGET_NAME} INTERFACE)
set_target_properties(${TARGET_NAME} PROPERTIES
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)
target_compile_features(${TARGET_NAME} INTERFACE
	cxx_std_20
)
target_include_directories(${TARGET_NAME} INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

#################################
# Define tests
#################################
set(TARGET_NAME ${PROJECT_NAME}_utils_test)
add_executable(${TARGET_NAME})
target_sources(${TARGET_NAME} PRIVATE
	ts_mem_chunk_test.cpp
)
target_link_libraries(${TARGET_NAME} PRIVATE
	Threads::Threads
	${PROJECT_NAME}_utils
)
target_compile_options(${TARGET_NAME} PRIVATE
	$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror>
)


set(TARGET_NAME fifo_test)
add_executable(${TARGET_NAME})
target_sources(${TARGET_NAME} PRIVATE
	slot_q_test.cpp
)
target_link_libraries(${TARGET_NAME} PRIVATE
	Threads::Threads
	${PROJECT_NAME}_utils
)
target_compile_options(${TARGET_NAME} PRIVATE
	$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror>
)


set(TARGET_NAME td_idx_q)
add_executable(${TARGET_NAME})
target_sources(${TARGET_NAME} PRIVATE
	ts_idx_q_test.cpp
)
target_link_libraries(${TARGET_NAME} PRIVATE
	Threads::Threads
	${PROJECT_NAME}_utils
)
target_compile_options(${TARGET_NAME} PRIVATE
	$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror>
)

