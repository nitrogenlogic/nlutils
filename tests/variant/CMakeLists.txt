add_executable(compare_test compare_test.c)
target_link_libraries(compare_test nlutils)

add_executable(clamp_test clamp_test.c)
target_link_libraries(clamp_test nlutils)

add_executable(to_string_test to_string_test.c)
target_link_libraries(to_string_test nlutils)

add_executable(data_test data_test.c)
target_link_libraries(data_test nlutils)
