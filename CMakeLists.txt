cmake_minimum_required(VERSION 3.20)

project(
  ezo_driver
  VERSION 0.5.1
  DESCRIPTION "Cross-platform Atlas Scientific EZO driver"
  LANGUAGES C CXX
)

include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(EZO_BUILD_TESTS "Build unit tests" OFF)
option(EZO_BUILD_EXAMPLES "Build examples" OFF)
option(EZO_BUILD_ARDUINO_ADAPTER "Build Arduino adapter sources" OFF)
option(EZO_BUILD_LINUX_ADAPTER "Build Linux adapter sources" OFF)
option(EZO_BUILD_POSIX_UART_ADAPTER "Build POSIX UART adapter sources" OFF)

function(ezo_enable_test_assertions target_name)
  # Keep assert() active for test code in Release builds so test expressions
  # with setup side effects still execute.
  if(MSVC)
    target_compile_options(${target_name} PRIVATE /UNDEBUG)
  else()
    target_compile_options(${target_name} PRIVATE -UNDEBUG)
  endif()
endfunction()

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
  src/ezo_linux_device.h
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

    add_executable(ezo_linux_device_tests
      tests/test_linux_device.c
    )

    target_include_directories(ezo_linux_device_tests
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(ezo_linux_device_tests PRIVATE ezo_core ezo_i2c_linux_adapter)

    add_test(
      NAME ezo_linux_device_tests
      COMMAND ezo_linux_device_tests
    )
  endif()

  set(ezo_test_targets
    ezo_i2c_core_tests
    ezo_i2c_cpp_tests
    ezo_uart_core_tests
    ezo_product_tests
    ezo_parse_tests
    ezo_control_tests
    ezo_calibration_transfer_tests
    ezo_ph_tests
    ezo_orp_tests
    ezo_rtd_tests
    ezo_ec_tests
    ezo_do_tests
    ezo_hum_tests
  )

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND ezo_test_targets
      ezo_i2c_linux_adapter_tests
      ezo_uart_posix_adapter_tests
      ezo_linux_device_tests
    )
  endif()

  foreach(ezo_test_target IN LISTS ezo_test_targets)
    ezo_enable_test_assertions(${ezo_test_target})
  endforeach()
endif()

if((EZO_BUILD_LINUX_ADAPTER OR EZO_BUILD_TESTS) AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
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

if(EZO_BUILD_LINUX_ADAPTER AND EZO_BUILD_POSIX_UART_ADAPTER AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_library(ezo_linux_device_adapter STATIC
    platform/linux/ezo_linux_device.c
  )

  target_include_directories(ezo_linux_device_adapter
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  target_link_libraries(ezo_linux_device_adapter PUBLIC ezo_core ezo_i2c_linux_adapter ezo_uart_posix_adapter)
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

function(ezo_add_linux_i2c_example target source_path)
  add_executable(${target}
    ${source_path}
  )

  target_link_libraries(${target} PRIVATE ezo_i2c_linux_example_common)
endfunction()

function(ezo_add_linux_uart_example target source_path)
  add_executable(${target}
    ${source_path}
  )

  target_link_libraries(${target} PRIVATE ezo_uart_linux_example_common)
endfunction()

if(EZO_BUILD_EXAMPLES AND EZO_BUILD_LINUX_ADAPTER)
  add_library(ezo_i2c_linux_example_common STATIC
    examples/linux/common/example_base.c
    examples/linux/common/example_control.c
    examples/linux/common/example_i2c.c
    examples/linux/common/example_products.c
  )

  target_include_directories(ezo_i2c_linux_example_common
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/examples/linux/common
  )

  target_link_libraries(ezo_i2c_linux_example_common PUBLIC ezo_i2c_linux_adapter)

  ezo_add_linux_i2c_example(
    ezo_linux_i2c_raw_command_example
    examples/linux/i2c/raw/raw_command.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_inspect_device_example
    examples/linux/i2c/commissioning/inspect_device.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_readiness_check_example
    examples/linux/i2c/commissioning/readiness_check.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_read_ph_example
    examples/linux/i2c/typed/read_ph.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_read_orp_example
    examples/linux/i2c/typed/read_orp.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_read_ec_example
    examples/linux/i2c/typed/read_ec.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_read_do_example
    examples/linux/i2c/typed/read_do.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_read_rtd_example
    examples/linux/i2c/typed/read_rtd.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_read_hum_example
    examples/linux/i2c/typed/read_hum.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_ph_workflow_example
    examples/linux/i2c/advanced/ph_workflow.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_ph_calibration_example
    examples/linux/i2c/advanced/ph_calibration.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_ph_temp_comp_from_rtd_example
    examples/linux/i2c/advanced/ph_temp_comp_from_rtd.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_do_workflow_example
    examples/linux/i2c/advanced/do_workflow.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_do_calibration_example
    examples/linux/i2c/advanced/do_calibration.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_do_temp_comp_from_rtd_example
    examples/linux/i2c/advanced/do_temp_comp_from_rtd.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_ec_temp_comp_from_rtd_example
    examples/linux/i2c/advanced/ec_temp_comp_from_rtd.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_ec_workflow_example
    examples/linux/i2c/advanced/ec_workflow.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_ec_calibration_example
    examples/linux/i2c/advanced/ec_calibration.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_do_salinity_comp_from_ec_example
    examples/linux/i2c/advanced/do_salinity_comp_from_ec.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_do_full_compensation_chain_example
    examples/linux/i2c/advanced/do_full_compensation_chain.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_rtd_workflow_example
    examples/linux/i2c/advanced/rtd_workflow.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_rtd_calibration_example
    examples/linux/i2c/advanced/rtd_calibration.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_orp_workflow_example
    examples/linux/i2c/advanced/orp_workflow.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_orp_calibration_example
    examples/linux/i2c/advanced/orp_calibration.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_hum_workflow_example
    examples/linux/i2c/advanced/hum_workflow.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_hum_temperature_calibration_example
    examples/linux/i2c/advanced/hum_temperature_calibration.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_calibration_transfer_example
    examples/linux/i2c/advanced/calibration_transfer.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_control_workflow_example
    examples/linux/i2c/advanced/control_workflow.c
  )
  ezo_add_linux_i2c_example(
    ezo_linux_i2c_transport_switch_example
    examples/linux/i2c/advanced/transport_switch.c
  )
endif()

if(EZO_BUILD_EXAMPLES AND EZO_BUILD_POSIX_UART_ADAPTER AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_library(ezo_uart_linux_example_common STATIC
    examples/linux/common/example_base.c
    examples/linux/common/example_uart.c
    examples/linux/common/example_products.c
  )

  target_include_directories(ezo_uart_linux_example_common
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/examples/linux/common
  )

  target_link_libraries(ezo_uart_linux_example_common PUBLIC ezo_uart_posix_adapter)

  ezo_add_linux_uart_example(
    ezo_linux_uart_raw_command_example
    examples/linux/uart/raw/raw_command.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_inspect_device_example
    examples/linux/uart/commissioning/inspect_device.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_readiness_check_example
    examples/linux/uart/commissioning/readiness_check.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_read_ph_example
    examples/linux/uart/typed/read_ph.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_read_orp_example
    examples/linux/uart/typed/read_orp.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_read_ec_example
    examples/linux/uart/typed/read_ec.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_read_do_example
    examples/linux/uart/typed/read_do.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_read_rtd_example
    examples/linux/uart/typed/read_rtd.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_read_hum_example
    examples/linux/uart/typed/read_hum.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_ph_workflow_example
    examples/linux/uart/advanced/ph_workflow.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_ph_calibration_example
    examples/linux/uart/advanced/ph_calibration.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_ph_temp_comp_from_rtd_example
    examples/linux/uart/advanced/ph_temp_comp_from_rtd.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_do_workflow_example
    examples/linux/uart/advanced/do_workflow.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_do_calibration_example
    examples/linux/uart/advanced/do_calibration.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_do_temp_comp_from_rtd_example
    examples/linux/uart/advanced/do_temp_comp_from_rtd.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_ec_temp_comp_from_rtd_example
    examples/linux/uart/advanced/ec_temp_comp_from_rtd.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_ec_workflow_example
    examples/linux/uart/advanced/ec_workflow.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_ec_calibration_example
    examples/linux/uart/advanced/ec_calibration.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_do_salinity_comp_from_ec_example
    examples/linux/uart/advanced/do_salinity_comp_from_ec.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_do_full_compensation_chain_example
    examples/linux/uart/advanced/do_full_compensation_chain.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_rtd_workflow_example
    examples/linux/uart/advanced/rtd_workflow.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_rtd_calibration_example
    examples/linux/uart/advanced/rtd_calibration.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_orp_workflow_example
    examples/linux/uart/advanced/orp_workflow.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_orp_calibration_example
    examples/linux/uart/advanced/orp_calibration.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_hum_workflow_example
    examples/linux/uart/advanced/hum_workflow.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_hum_temperature_calibration_example
    examples/linux/uart/advanced/hum_temperature_calibration.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_calibration_transfer_example
    examples/linux/uart/advanced/calibration_transfer.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_control_workflow_example
    examples/linux/uart/advanced/control_workflow.c
  )
  ezo_add_linux_uart_example(
    ezo_linux_uart_transport_switch_example
    examples/linux/uart/advanced/transport_switch.c
  )
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

if(EZO_BUILD_LINUX_ADAPTER AND TARGET ezo_i2c_linux_adapter)
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

if(TARGET ezo_linux_device_adapter)
  install(TARGETS ezo_linux_device_adapter
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

set(EZO_DRIVER_CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/ezo_driver)

configure_package_config_file(
  cmake/ezo_driverConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/ezo_driverConfig.cmake
  INSTALL_DESTINATION ${EZO_DRIVER_CONFIG_INSTALL_DIR}
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/ezo_driverConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/ezo_driverConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/ezo_driverConfigVersion.cmake
  DESTINATION ${EZO_DRIVER_CONFIG_INSTALL_DIR}
)
