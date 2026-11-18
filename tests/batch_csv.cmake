execute_process(
    COMMAND "${BATCH_EXE}" 800 800 320 240 0.12 "${INPUT_CSV}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE actual
    ERROR_VARIABLE diagnostic)
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "stereo_batch failed (${result}): ${diagnostic}")
endif()

string(CONCAT expected
    "row,x_m,y_m,z_m,disparity_px,vertical_mismatch_px,status\n"
    "1,0.4,0.1,4,24,0,ok\n"
    "2,,,,,,invalid_disparity\n"
    "3,,,,,,malformed_row\n"
    "4,-0.1,-0.05,2,48,0,ok\n")
string(REPLACE "\r\n" "\n" actual "${actual}")
if(NOT actual STREQUAL expected)
    message(FATAL_ERROR "Unexpected stereo_batch output:\n${actual}")
endif()
