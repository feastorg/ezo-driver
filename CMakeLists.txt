cmake_minimum_required(VERSION 3.20)

project(
  ezo_i2c_driver
  VERSION 0.1.0
  DESCRIPTION "Cross-platform EZO I2C driver rewrite"
  LANGUAGES C CXX
)

option(EZO_BUILD_TESTS "Build unit tests" OFF)
option(EZO_BUILD_EXAMPLES "Build examples" OFF)
option(EZO_BUILD_ARDUINO_ADAPTER "Build Arduino adapter sources" OFF)
option(EZO_BUILD_LINUX_ADAPTER "Build Linux adapter sources" OFF)
option(EZO_BUILD_CPP_WRAPPER "Build C++ wrapper integration targets" OFF)

add_library(ezo_i2c_core STATIC
  src/ezo_i2c.c
)

target_include_directories(ezo_i2c_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(ezo_i2c_core PUBLIC c_std_99)

if(EZO_BUILD_TESTS)
  add_executable(ezo_i2c_core_tests
    tests/fakes/ezo_fake_transport.c
    tests/test_ezo_i2c.c
  )

  target_include_directories(ezo_i2c_core_tests
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

  target_link_libraries(ezo_i2c_core_tests PRIVATE ezo_i2c_core)
endif()

message(STATUS "Configured ezo_i2c_driver core target")
