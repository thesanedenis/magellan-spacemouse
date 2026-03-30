cmake_minimum_required(VERSION 3.13)

set(MAGELLAN_TARGET "rp2040usb" CACHE STRING "Magellan target")
set_property(CACHE MAGELLAN_TARGET PROPERTY STRINGS "rp2040usb" "rp2040rs232")

add_compile_definitions(PICO_DEFAULT_UART_BAUD_RATE=921600)

set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/pico-sdk")
set(PICO_TINYUSB_PATH "${CMAKE_CURRENT_LIST_DIR}/tinyusb")

include(pico_sdk_import.cmake)

project(${MAGELLAN_TARGET})

pico_sdk_init()

if (${MAGELLAN_TARGET} STREQUAL "rp2040usb")
    add_compile_definitions(MAGELLAN_TARGET_USB)
    include(FetchContent)
    FetchContent_Declare(
        pio_usb
        GIT_REPOSITORY https://github.com/sekigon-gonnoc/Pico-PIO-USB
        GIT_TAG main
    )
    FetchContent_MakeAvailable(pio_usb)
    add_executable(${MAGELLAN_TARGET}
        src/magellan.c
        src/descriptors.c
        tinyusb/src/portable/raspberrypi/pio_usb/hcd_pio_usb.c
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

if (${MAGELLAN_TARGET} STREQUAL "rp2040usb")
    target_link_libraries(${MAGELLAN_TARGET}
        tinyusb_device
        tinyusb_host
        tinyusb_board
        pico_pio_usb
    )
else()
    target_link_libraries(${MAGELLAN_TARGET}
        tinyusb_device
        tinyusb_board
    )
endif()

pico_add_extra_outputs(${MAGELLAN_TARGET})
