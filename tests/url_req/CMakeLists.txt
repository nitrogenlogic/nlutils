add_executable(url_req_test url_req_test.c)
target_link_libraries(url_req_test nlutils)

add_executable(get_url get_url.c)
target_link_libraries(get_url nlutils)

file(COPY url_req_server.rb DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
