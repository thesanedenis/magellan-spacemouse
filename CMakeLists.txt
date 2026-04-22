cmake_minimum_required(VERSION 3.13)

set(MAGELLAN_TARGET "rp2040usb" CACHE STRING "Magellan target")
set_property(CACHE MAGELLAN_TARGET PROPERTY STRINGS "rp2040usb" "rp2040rs232")

add_compile_definitions(PICO_DEFAULT_UART_BAUD_RATE=921600)

if(DEFINED ENV{MAGELLAN_LONG_PRESS_ENABLE})
    if($ENV{MAGELLAN_LONG_PRESS_ENABLE} STREQUAL "ON" OR $ENV{MAGELLAN_LONG_PRESS_ENABLE} STREQUAL "1")
        add_compile_definitions(ENABLE_LONG_PRESS)
    endif()
endif()

set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/pico-sdk")
set(PICO_TINYUSB_PATH "${CMAKE_CURRENT_LIST_DIR}/tinyusb")

include(pico_sdk_import.cmake)

project(${MAGELLAN_TARGET})

pico_sdk_init()

if (${MAGELLAN_TARGET} STREQUAL "rp2040usb")
    add_compile_definitions(MAGELLAN_TARGET_USB)
    add_executable(${MAGELLAN_TARGET}
        src/magellan.c
        src/descriptors.c
    )
else()
    add_compile_definitions(MAGELLAN_TARGET_RS232)
    add_executable(${MAGELLAN_TARGET}
        src/magellan.c
        src/descriptors.c
    )
endif()

add_compile_options(-Wall)

target_include_directories(${MAGELLAN_TARGET} PRIVATE
    src
)

if (${MAGELLAN_TARGET} STREQUAL "rp2040usb")
    target_include_directories(${MAGELLAN_TARGET} PRIVATE
        tinyusb/src/portable/raspberrypi/pio_usb
    )
endif()

target_link_libraries(${MAGELLAN_TARGET}
    pico_stdlib
)

# Disable UART stdio to avoid conflict with our mouse on pins 0/1
pico_enable_stdio_uart(${MAGELLAN_TARGET} 0)

if (${MAGELLAN_TARGET} STREQUAL "rp2040rs232")
    # RS232 target can have USB stdio enabled for debug/stability
    pico_enable_stdio_usb(${MAGELLAN_TARGET} 1)
else()
    # USB target uses PIO_USB host, must disable USB stdio to avoid conflict
    pico_enable_stdio_usb(${MAGELLAN_TARGET} 0)
endif()

if (${MAGELLAN_TARGET} STREQUAL "rp2040usb")
    target_link_libraries(${MAGELLAN_TARGET}
        tinyusb_device
        tinyusb_board
    )
else()
    target_link_libraries(${MAGELLAN_TARGET}
        tinyusb_device
        tinyusb_board
    )
endif()

pico_add_extra_outputs(${MAGELLAN_TARGET})
