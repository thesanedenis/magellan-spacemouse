cmake_minimum_required(VERSION 3.13)

set(MAGELLAN_TARGET "magellan-serial-uart1" CACHE STRING "Magellan target")
set_property(CACHE MAGELLAN_TARGET PROPERTY STRINGS "magellan-serial-uart1" "magellan-serial-uart0")

add_compile_definitions(PICO_DEFAULT_UART_BAUD_RATE=921600)

set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/pico-sdk")
set(PICO_TINYUSB_PATH "${CMAKE_CURRENT_LIST_DIR}/tinyusb")

include(pico_sdk_import.cmake)

project(${MAGELLAN_TARGET})

pico_sdk_init()

if (${MAGELLAN_TARGET} STREQUAL "magellan-serial-uart0")
    add_compile_definitions(MAGELLAN_TARGET_RS232_ADAPTER)
endif()

add_executable(${MAGELLAN_TARGET}
    src/magellan.c
    src/descriptors.c
)

add_compile_options(-Wall)

target_include_directories(${MAGELLAN_TARGET} PRIVATE
    src
)

target_link_libraries(${MAGELLAN_TARGET}
    pico_stdlib
    tinyusb_device
    tinyusb_board
)

# Disable UART stdio to avoid conflict with our mouse
pico_enable_stdio_uart(${MAGELLAN_TARGET} 0)

if (${MAGELLAN_TARGET} STREQUAL "magellan-serial-uart0")
    # UART0 target (GPIO 0/1) can have USB stdio enabled for debug
    pico_enable_stdio_usb(${MAGELLAN_TARGET} 1)
else()
    # UART1 target (GPIO 20/21)
    pico_enable_stdio_usb(${MAGELLAN_TARGET} 1)
endif()

pico_add_extra_outputs(${MAGELLAN_TARGET})
