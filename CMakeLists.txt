cmake_minimum_required(VERSION 3.20)
project(stereo_depth_lab VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(stereo_depth src/stereo.cpp)
target_include_directories(stereo_depth PUBLIC include)

add_executable(stereo_point src/main.cpp)
target_link_libraries(stereo_point PRIVATE stereo_depth)

add_executable(stereo_batch src/batch.cpp)
target_link_libraries(stereo_batch PRIVATE stereo_depth)

add_executable(stereo_noise experiments/stereo_noise.cpp)
target_link_libraries(stereo_noise PRIVATE stereo_depth)

include(CTest)
if(BUILD_TESTING)
    add_executable(stereo_tests tests/stereo_tests.cpp)
    target_link_libraries(stereo_tests PRIVATE stereo_depth)
    add_test(NAME triangulation COMMAND stereo_tests)

    add_test(NAME example_cli COMMAND stereo_point 800 800 320 240 0.12 400 260 376 260)
    set_tests_properties(example_cli PROPERTIES
        PASS_REGULAR_EXPRESSION "x_m=0[.]4 y_m=0[.]1 z_m=4")
    add_test(NAME rejects_invalid_argument COMMAND stereo_point 800oops 800 320 240 0.12 400 260 376 260)
    set_tests_properties(rejects_invalid_argument PROPERTIES WILL_FAIL TRUE)

    add_test(NAME pixel_noise_experiment COMMAND stereo_noise)
    set_tests_properties(pixel_noise_experiment PROPERTIES
        PASS_REGULAR_EXPRESSION "12,8,-1,7,2,13[.]71428571,1[.]714285714,1[.]5,ok")

    add_test(NAME batch_csv COMMAND ${CMAKE_COMMAND}
        "-DBATCH_EXE=$<TARGET_FILE:stereo_batch>"
        "-DINPUT_CSV=${CMAKE_CURRENT_SOURCE_DIR}/examples/matches.csv"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/batch_csv.cmake")
endif()
