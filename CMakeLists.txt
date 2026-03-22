cmake_minimum_required(VERSION 3.20)

project(
  ezo_driver
  VERSION 0.3.0
  DESCRIPTION "Cross-platform Atlas Scientific EZO driver"
  LANGUAGES C CXX
)

include(CTest)
include(GNUInstallDirs)

option(EZO_BUILD_TESTS "Build unit tests" OFF)
option(EZO_BUILD_EXAMPLES "Build examples" OFF)
option(EZO_BUILD_ARDUINO_ADAPTER "Build Arduino adapter sources" OFF)
option(EZO_BUILD_LINUX_ADAPTER "Build Linux adapter sources" OFF)
option(EZO_BUILD_POSIX_UART_ADAPTER "Build POSIX UART adapter sources" OFF)

set(EZO_PUBLIC_HEADERS
  src/ezo.h
  src/ezo_calibration_transfer.h
  src/ezo_control.h
  src/ezo_do.h
  src/ezo_ec.h
  src/ezo_hum.h
  src/ezo_i2c.h
  src/ezo_i2c.hpp
  src/ezo_i2c_arduino_wire.h
  src/ezo_i2c_linux_i2c.h
  src/ezo_orp.h
  src/ezo_parse.h
  src/ezo_ph.h
  src/ezo_product.h
  src/ezo_rtd.h
  src/ezo_schema.h
  src/ezo_uart.h
  src/ezo_uart_arduino_stream.h
  src/ezo_uart_posix_serial.h
)

add_library(ezo_core STATIC
  src/ezo.c
  src/ezo_calibration_transfer.c
  src/ezo_common.c
  src/ezo_control.c
  src/ezo_do.c
  src/ezo_ec.c
  src/ezo_hum.c
  src/ezo_i2c.c
  src/ezo_parse.c
  src/ezo_orp.c
  src/ezo_ph.c
  src/ezo_product.c
  src/ezo_rtd.c
  src/ezo_schema.c
  src/ezo_uart.c
)

target_include_directories(ezo_core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(ezo_core PUBLIC c_std_99)

add_library(ezo_i2c_cpp INTERFACE)

target_include_directories(ezo_i2c_cpp
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(ezo_i2c_cpp INTERFACE ezo_core)

target_compile_features(ezo_i2c_cpp INTERFACE cxx_std_11)

if(EZO_BUILD_TESTS)
  add_executable(ezo_i2c_core_tests
    tests/fakes/ezo_fake_i2c_transport.c
    tests/test_ezo_i2c.c
  )

  target_include_directories(ezo_i2c_core_tests
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

  target_link_libraries(ezo_i2c_core_tests PRIVATE ezo_core)

  add_test(
    NAME ezo_i2c_core_tests
    COMMAND ezo_i2c_core_tests
  )

  add_executable(ezo_i2c_cpp_tests
    tests/fakes/ezo_fake_i2c_transport.c
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

  add_executable(ezo_uart_core_tests
    tests/fakes/ezo_fake_uart_transport.c
    tests/test_ezo_uart.c
  )

  target_include_directories(ezo_uart_core_tests
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

  target_link_libraries(ezo_uart_core_tests PRIVATE ezo_core)

  add_test(
    NAME ezo_uart_core_tests
    COMMAND ezo_uart_core_tests
  )

  add_executable(ezo_product_tests
    tests/test_ezo_product.c
  )

  target_include_directories(ezo_product_tests
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

  target_link_libraries(ezo_product_tests PRIVATE ezo_core)

  add_test(
    NAME ezo_product_tests
    COMMAND ezo_product_tests
  )

  add_executable(ezo_parse_tests
    tests/test_ezo_parse.c
  )

  target_include_directories(ezo_parse_tests
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

  target_link_libraries(ezo_parse_tests PRIVATE ezo_core)

  add_test(
    NAME ezo_parse_tests
    COMMAND ezo_parse_tests
  )

  add_executable(ezo_control_tests
    tests/fakes/ezo_fake_i2c_transport.c
    tests/fakes/ezo_fake_uart_transport.c
    tests/test_ezo_control.c
  )

  target_include_directories(ezo_control_tests
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

  target_link_libraries(ezo_control_tests PRIVATE ezo_core)

  add_test(
    NAME ezo_control_tests
    COMMAND ezo_control_tests
  )

  add_executable(ezo_calibration_transfer_tests
    tests/fakes/ezo_fake_i2c_transport.c
    tests/fakes/ezo_fake_uart_transport.c
    tests/test_ezo_calibration_transfer.c
  )

  target_include_directories(ezo_calibration_transfer_tests
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

  target_link_libraries(ezo_calibration_transfer_tests PRIVATE ezo_core)

  add_test(
    NAME ezo_calibration_transfer_tests
    COMMAND ezo_calibration_transfer_tests
  )

  add_executable(ezo_ph_tests
    tests/fakes/ezo_fake_i2c_transport.c
    tests/fakes/ezo_fake_uart_transport.c
    tests/test_ezo_ph.c
  )

  target_include_directories(ezo_ph_tests
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

  target_link_libraries(ezo_ph_tests PRIVATE ezo_core)

  add_test(
    NAME ezo_ph_tests
    COMMAND ezo_ph_tests
  )

  add_executable(ezo_orp_tests
    tests/fakes/ezo_fake_i2c_transport.c
    tests/fakes/ezo_fake_uart_transport.c
    tests/test_ezo_orp.c
  )

  target_include_directories(ezo_orp_tests
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

  target_link_libraries(ezo_orp_tests PRIVATE ezo_core)

  add_test(
    NAME ezo_orp_tests
    COMMAND ezo_orp_tests
  )

  add_executable(ezo_rtd_tests
    tests/fakes/ezo_fake_i2c_transport.c
    tests/fakes/ezo_fake_uart_transport.c
    tests/test_ezo_rtd.c
  )

  target_include_directories(ezo_rtd_tests
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

  target_link_libraries(ezo_rtd_tests PRIVATE ezo_core)

  add_test(
    NAME ezo_rtd_tests
    COMMAND ezo_rtd_tests
  )

  add_executable(ezo_ec_tests
    tests/fakes/ezo_fake_i2c_transport.c
    tests/fakes/ezo_fake_uart_transport.c
    tests/test_ezo_ec.c
  )

  target_include_directories(ezo_ec_tests
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

  target_link_libraries(ezo_ec_tests PRIVATE ezo_core)

  add_test(
    NAME ezo_ec_tests
    COMMAND ezo_ec_tests
  )

  add_executable(ezo_do_tests
    tests/fakes/ezo_fake_i2c_transport.c
    tests/fakes/ezo_fake_uart_transport.c
    tests/test_ezo_do.c
  )

  target_include_directories(ezo_do_tests
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

  target_link_libraries(ezo_do_tests PRIVATE ezo_core)

  add_test(
    NAME ezo_do_tests
    COMMAND ezo_do_tests
  )

  add_executable(ezo_hum_tests
    tests/fakes/ezo_fake_i2c_transport.c
    tests/fakes/ezo_fake_uart_transport.c
    tests/test_ezo_hum.c
  )

  target_include_directories(ezo_hum_tests
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

  target_link_libraries(ezo_hum_tests PRIVATE ezo_core)

  add_test(
    NAME ezo_hum_tests
    COMMAND ezo_hum_tests
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

    add_executable(ezo_uart_posix_adapter_tests
      platform/linux/ezo_uart_posix_serial.c
      tests/test_uart_posix_adapter.c
    )

    target_include_directories(ezo_uart_posix_adapter_tests
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(ezo_uart_posix_adapter_tests PRIVATE ezo_core)

    add_test(
      NAME ezo_uart_posix_adapter_tests
      COMMAND ezo_uart_posix_adapter_tests
    )
  endif()
endif()

if(EZO_BUILD_LINUX_ADAPTER)
  add_library(ezo_i2c_linux_adapter STATIC
    platform/linux/ezo_i2c_linux_i2c.c
  )

  target_include_directories(ezo_i2c_linux_adapter
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  target_link_libraries(ezo_i2c_linux_adapter PUBLIC ezo_core)
endif()

if(EZO_BUILD_POSIX_UART_ADAPTER AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_library(ezo_uart_posix_adapter STATIC
    platform/linux/ezo_uart_posix_serial.c
  )

  target_include_directories(ezo_uart_posix_adapter
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  target_link_libraries(ezo_uart_posix_adapter PUBLIC ezo_core)
endif()

if(EZO_BUILD_ARDUINO_ADAPTER)
  add_library(ezo_i2c_arduino_adapter STATIC
    src/ezo_i2c_arduino_wire.cpp
  )

  target_include_directories(ezo_i2c_arduino_adapter
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  target_link_libraries(ezo_i2c_arduino_adapter PUBLIC ezo_core)

  add_library(ezo_uart_arduino_adapter STATIC
    src/ezo_uart_arduino_stream.cpp
  )

  target_include_directories(ezo_uart_arduino_adapter
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  target_link_libraries(ezo_uart_arduino_adapter PUBLIC ezo_core)
endif()

if(EZO_BUILD_EXAMPLES AND EZO_BUILD_LINUX_ADAPTER)
  add_executable(ezo_i2c_linux_read_example
    examples/linux_read.c
  )

  target_link_libraries(ezo_i2c_linux_read_example PRIVATE ezo_i2c_linux_adapter)

  add_executable(ezo_ph_linux_read_example
    examples/linux_ph_read.c
  )

  target_link_libraries(ezo_ph_linux_read_example PRIVATE ezo_i2c_linux_adapter)

  add_executable(ezo_ec_linux_read_example
    examples/linux_ec_read.c
  )

  target_link_libraries(ezo_ec_linux_read_example PRIVATE ezo_i2c_linux_adapter)

  add_executable(ezo_do_linux_read_example
    examples/linux_do_read.c
  )

  target_link_libraries(ezo_do_linux_read_example PRIVATE ezo_i2c_linux_adapter)

  add_executable(ezo_hum_linux_read_example
    examples/linux_hum_read.c
  )

  target_link_libraries(ezo_hum_linux_read_example PRIVATE ezo_i2c_linux_adapter)

  add_executable(ezo_identity_linux_example
    examples/linux_identity.c
  )

  target_link_libraries(ezo_identity_linux_example PRIVATE ezo_i2c_linux_adapter)

  add_executable(ezo_control_status_linux_example
    examples/linux_control_status.c
  )

  target_link_libraries(ezo_control_status_linux_example PRIVATE ezo_i2c_linux_adapter)
endif()

if(EZO_BUILD_EXAMPLES AND EZO_BUILD_POSIX_UART_ADAPTER AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_executable(ezo_uart_linux_read_example
    examples/linux_uart_read.c
  )

  target_link_libraries(ezo_uart_linux_read_example PRIVATE ezo_uart_posix_adapter)
endif()

install(TARGETS ezo_core ezo_i2c_cpp
  EXPORT ezo_driver_targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${EZO_PUBLIC_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(TARGET ezo_i2c_linux_adapter)
  install(TARGETS ezo_i2c_linux_adapter
    EXPORT ezo_driver_targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endif()

if(TARGET ezo_uart_posix_adapter)
  install(TARGETS ezo_uart_posix_adapter
    EXPORT ezo_driver_targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endif()

install(EXPORT ezo_driver_targets
  FILE ezo_driver_targets.cmake
  NAMESPACE ezo::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ezo_driver
)