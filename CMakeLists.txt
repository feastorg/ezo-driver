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

# Phase 00 foundation skeleton:
# - core sources will live under src/ and public headers under include/
# - adapters remain isolated in adapters/
# - tests and examples are opt-in
# Actual targets are added in later phases once the API and contracts are defined.

message(STATUS "Configured Phase 00 foundation skeleton for ezo_i2c_driver")
