#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#define CFG_TUSB_OS OPT_OS_PICO

// Device configuration
#define CFG_TUD_ENABLED 1
#define CFG_TUD_RHPORT 0
#define CFG_TUD_HID 1
#define CFG_TUD_ENDPOINT0_SIZE 64
#define CFG_TUD_HID_EP_BUFSIZE 64

// Host configuration
#define CFG_TUH_ENABLED 0

// Common configuration
#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))

// Port modes
#define BOARD_DEVICE_RHPORT_NUM 0
#define BOARD_DEVICE_RHPORT_SPEED OPT_MODE_FULL_SPEED
#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE | BOARD_DEVICE_RHPORT_SPEED)

#endif
