add_subdirectory(variant)
add_subdirectory(url_req)

add_executable(sha1_test sha1_test.c)
target_link_libraries(sha1_test nlutils)

add_executable(sha1_selftest sha1_selftest.c)
target_link_libraries(sha1_selftest nlutils)

add_executable(escape_test escape_test.c)
target_link_libraries(escape_test nlutils)

add_executable(net_test net_test.c)
target_link_libraries(net_test nlutils)

add_executable(string_test string_test.c)
target_link_libraries(string_test nlutils)

add_executable(thread_test thread_test.c)
target_link_libraries(thread_test nlutils)

add_executable(kvp_test kvp_test.c)
target_link_libraries(kvp_test nlutils)

add_executable(url_test url_test.c)
target_link_libraries(url_test nlutils)

add_executable(fifo_test fifo_test.c)
target_link_libraries(fifo_test nlutils)

add_executable(hash_test hash_test.c)
target_link_libraries(hash_test nlutils)

add_executable(exec_test exec_test.c)
target_link_libraries(exec_test nlutils)

add_executable(mem_test mem_test.c)
target_link_libraries(mem_test nlutils)

add_executable(time_test time_test.c)
target_link_libraries(time_test nlutils)

add_executable(timespec_benchmark timespec_benchmark.c)
target_link_libraries(timespec_benchmark nlutils)

add_executable(stream_test stream_test.c)
target_link_libraries(stream_test nlutils)

add_executable(debug_test debug_test.c)
target_link_libraries(debug_test nlutils)

add_executable(term_test term_test.c)
target_link_libraries(term_test nlutils)

file(COPY firmware tests.sh
	DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
