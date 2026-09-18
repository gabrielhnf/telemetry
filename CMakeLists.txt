cmake_minimum_required(VERSION 3.10)
project(telemetry LANGUAGES C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/src/*.c")
#
#foreach(SOURCE_FILE ${SOURCES})
#  get_filename_component(EXE_NAME ${SOURCE_FILE} NAME_WE)
#  add_executable(${EXE_NAME} ${SOURCE_FILE})
#  target_include_directories(${EXE_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)
#endforeach()

add_executable(test_packet 
  test/test_packet.c 
  test/unity/unity.c 
  src/packet.c
  src/crc.c
)

target_include_directories(test_packet PRIVATE include test/unity)
#target_compile_options(test_packet PRIVATE -Wall -Wextra -Wpedantic -Wconversion -fsanitize=address,undefined)
