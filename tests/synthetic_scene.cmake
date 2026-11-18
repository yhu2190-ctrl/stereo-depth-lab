set(matches "${CMAKE_CURRENT_BINARY_DIR}/synthetic_scene_matches.csv")
execute_process(
    COMMAND "${SCENE_EXE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE pixels
    ERROR_VARIABLE diagnostic)
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "stereo_scene failed (${result}): ${diagnostic}")
endif()

string(REPLACE "\r\n" "\n" pixels "${pixels}")
if(NOT pixels MATCHES "^u_left,v_left,u_right,v_right\n128,96,80,96\n" OR
   NOT pixels MATCHES "\n448,336,416,336\n$")
    message(FATAL_ERROR "The opposite near and far corners do not match their known pixel coordinates")
endif()
file(WRITE "${matches}" "${pixels}")

execute_process(
    COMMAND "${BATCH_EXE}" 768 768 320 240 0.125 "${matches}"
            --max-vertical-mismatch 0 --format ply
    RESULT_VARIABLE result
    OUTPUT_VARIABLE reconstructed
    ERROR_VARIABLE diagnostic)
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "stereo_batch failed (${result}): ${diagnostic}")
endif()

string(REPLACE "\r\n" "\n" diagnostic "${diagnostic}")
if(NOT diagnostic STREQUAL "stereo_batch: exported 84 of 84 rows\n")
    message(FATAL_ERROR "Expected 84 valid points: ${diagnostic}")
endif()

# The reference lists the sampled edge coordinates directly in meters.
file(READ "${REFERENCE_PLY}" expected)
string(REPLACE "\r\n" "\n" expected "${expected}")
string(REPLACE "\r\n" "\n" reconstructed "${reconstructed}")
if(NOT reconstructed STREQUAL expected)
    message(FATAL_ERROR "Reconstructed point cloud differs from ${REFERENCE_PLY}:\n${reconstructed}")
endif()
