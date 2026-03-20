cmake_minimum_required(VERSION 3.20)

project(
  ezo_i2c_driver
  VERSION 0.1.0
  DESCRIPTION "Cross-platform EZO I2C driver rewrite"
  LANGUAGES C CXX
)

include(CTest)

option(EZO_BUILD_TESTS "Build unit tests" OFF)
option(EZO_BUILD_EXAMPLES "Build examples" OFF)
option(EZO_BUILD_ARDUINO_ADAPTER "Build Arduino adapter sources" OFF)
option(EZO_BUILD_LINUX_ADAPTER "Build Linux adapter sources" OFF)

add_library(ezo_i2c_core STATIC
  src/ezo_common.c
  src/ezo_i2c.c
)

target_include_directories(ezo_i2c_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(ezo_i2c_core PUBLIC c_std_99)

add_library(ezo_i2c_cpp INTERFACE)

target_include_directories(ezo_i2c_cpp
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ezo_i2c_cpp INTERFACE ezo_i2c_core)

target_compile_features(ezo_i2c_cpp INTERFACE cxx_std_11)

if(EZO_BUILD_TESTS)
  add_executable(ezo_i2c_core_tests
    tests/fakes/ezo_fake_transport.c
    tests/test_ezo_i2c.c
  )

  target_include_directories(ezo_i2c_core_tests
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

  target_link_libraries(ezo_i2c_core_tests PRIVATE ezo_i2c_core)

  add_test(
    NAME ezo_i2c_core_tests
    COMMAND ezo_i2c_core_tests
  )

  add_executable(ezo_i2c_cpp_tests
    tests/fakes/ezo_fake_transport.c
    tests/test_ezo_i2c_cpp.cpp
  )

  target_include_directories(ezo_i2c_cpp_tests
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

  target_link_libraries(ezo_i2c_cpp_tests PRIVATE ezo_i2c_cpp)

  add_test(
    NAME ezo_i2c_cpp_tests
    COMMAND ezo_i2c_cpp_tests
  )

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(ezo_i2c_linux_adapter_tests
      tests/test_linux_adapter.c
    )

    target_include_directories(ezo_i2c_linux_adapter_tests
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    add_test(
      NAME ezo_i2c_linux_adapter_tests
      COMMAND ezo_i2c_linux_adapter_tests
    )
  endif()
endif()

if(EZO_BUILD_LINUX_ADAPTER)
  add_library(ezo_i2c_linux_adapter STATIC
    platform/linux/ezo_i2c_linux_i2c.c
  )

  target_include_directories(ezo_i2c_linux_adapter
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

  target_link_libraries(ezo_i2c_linux_adapter PUBLIC ezo_i2c_core)
endif()

if(EZO_BUILD_ARDUINO_ADAPTER)
  add_library(ezo_i2c_arduino_adapter STATIC
    src/ezo_i2c_arduino_wire.cpp
  )

  target_include_directories(ezo_i2c_arduino_adapter
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

  target_link_libraries(ezo_i2c_arduino_adapter PUBLIC ezo_i2c_core)
endif()

if(EZO_BUILD_EXAMPLES AND EZO_BUILD_LINUX_ADAPTER)
  add_executable(ezo_i2c_linux_read_example
    examples/linux_read.c
  )

  target_link_libraries(ezo_i2c_linux_read_example PRIVATE ezo_i2c_linux_adapter)
endif()
