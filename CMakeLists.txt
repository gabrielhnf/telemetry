cmake_minimum_required(VERSION 3.13)

project(telemetry LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

enable_testing()

option(TELEMETRY_SANITIZERS "Enable ASan and UBSan" ON)

# Project library
add_library(telemetry_core STATIC
  src/crc.c
  src/packet.c
  src/parser.c
  src/message.c
)

target_include_directories(telemetry_core PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_options(telemetry_core PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

# Unity test framework
add_library(unity STATIC
  test/unity/unity.c
)

target_include_directories(unity PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/test"
  "${CMAKE_CURRENT_SOURCE_DIR}/test/unity"
)

# Sanitizer configuration
if(TELEMETRY_SANITIZERS)
  if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    message(FATAL_ERROR "Sanitizers require GCC or Clang")
  endif()

  target_compile_options(telemetry_core PRIVATE
    -g
    -O1
    -fno-omit-frame-pointer
    -fsanitize=address,undefined
  )
endif()

# Helper to register a Unity test executable
function(add_telemetry_test name source)
  add_executable(${name}
    ${source}
  )

  target_link_libraries(${name} PRIVATE
    telemetry_core
    unity
  )

  target_compile_options(${name} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
  )

  if(TELEMETRY_SANITIZERS)
    target_compile_options(${name} PRIVATE
      -g
      -O1
      -fno-omit-frame-pointer
      -fsanitize=address,undefined
    )

    target_link_options(${name} PRIVATE
      -fsanitize=address,undefined
    )
  endif()

  add_test(NAME ${name} COMMAND ${name})
endfunction()

# Tests
add_telemetry_test(test_packet  test/test_packet.c)
add_telemetry_test(test_parser  test/test_parser.c)
add_telemetry_test(test_message test/test_message.c)
