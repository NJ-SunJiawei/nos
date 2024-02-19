set(SRC_FILES
    main_test.c
)

add_executable(test ${SRC_FILES})
target_include_directories(test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
#target_compile_options(test PUBLIC -D )
target_link_libraries(test static_os)