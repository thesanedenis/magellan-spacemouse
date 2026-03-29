cmake_minimum_required(VERSION 3.13)

add_compile_definitions(PICO_DEFAULT_UART_BAUD_RATE=921600)

set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/pico-sdk")
set(PICO_TINYUSB_PATH "${CMAKE_CURRENT_LIST_DIR}/tinyusb")

include(pico_sdk_import.cmake)

project(magellan)

pico_sdk_init()

include(FetchContent)
FetchContent_Declare(
    pio_usb
    GIT_REPOSITORY https://github.com/sekigon-gonnoc/Pico-PIO-USB
    GIT_TAG main
)
FetchContent_MakeAvailable(pio_usb)

add_compile_options(-Wall)

add_executable(magellan
    src/magellan.c
    src/descriptors.c
    tinyusb/src/portable/raspberrypi/pio_usb/hcd_pio_usb.c
)

target_include_directories(magellan PRIVATE 
    src
    tinyusb/src/portable/raspberrypi/pio_usb
)

target_link_libraries(magellan
    pico_stdlib
    tinyusb_device
    tinyusb_host
    tinyusb_board
    pico_pio_usb
)

pico_add_extra_outputs(magellan)
