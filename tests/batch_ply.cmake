function(check_ply input count total vertices)
    execute_process(
        COMMAND "${BATCH_EXE}" 800 800 320 240 0.12 "${input}" ${ARGN}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE actual
        ERROR_VARIABLE diagnostic)
    if(NOT result STREQUAL "0")
        message(FATAL_ERROR "stereo_batch failed (${result}): ${diagnostic}")
    endif()

    string(CONCAT expected
        "ply\n"
        "format ascii 1.0\n"
        "comment Stereo Depth Lab - left camera coordinates in meters\n"
        "element vertex ${count}\n"
        "property double x\n"
        "property double y\n"
        "property double z\n"
        "end_header\n"
        "${vertices}")
    string(REPLACE "\r\n" "\n" actual "${actual}")
    string(REPLACE "\r\n" "\n" diagnostic "${diagnostic}")
    if(NOT actual STREQUAL expected OR
       NOT diagnostic STREQUAL "stereo_batch: exported ${count} of ${total} rows\n")
        message(FATAL_ERROR "Unexpected export to PLY:\n${actual}\n${diagnostic}")
    endif()
endfunction()

check_ply("${INPUT_CSV}" 3 5 "0.4 0.1 4\n-0.1 -0.05 2\n0.4 0.105 4\n" --format ply)
check_ply("${INPUT_CSV}" 2 5 "0.4 0.1 4\n-0.1 -0.05 2\n" --max-vertical-mismatch 1 --format ply)

set(empty_input "${CMAKE_CURRENT_BINARY_DIR}/batch_ply_empty.csv")
file(WRITE "${empty_input}" "u_left,v_left,u_right,v_right\n")
check_ply("${empty_input}" 0 0 "" --format ply)
file(WRITE "${empty_input}" "u_left,v_left,u_right,v_right\n400,260,400,260\n")
check_ply("${empty_input}" 0 1 "" --format ply)
