#include <stdio.h>
#include <string.h>

#ifdef MAGELLAN_TARGET_USB
#include <bsp/board.h>
#include <tusb.h>
#include <pio_usb.h>
#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <pico/multicore.h>
#include <pico/stdio.h>
#endif

#ifdef MAGELLAN_TARGET_RS232
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "tusb.h"
#endif

#ifdef MAGELLAN_TARGET_USB
uint16_t trans_report[3];
uint16_t rot_report[3];
uint8_t buttons_report[6];

volatile bool trans_pending = false;
volatile bool rot_pending = false;
volatile bool buttons_pending = false;

#ifdef ENABLE_LONG_PRESS
uint32_t host_buttons = 0;
uint32_t last_host_buttons = 0;
uint32_t press_start_ms[15] = {0};
bool long_press_sent[15] = {false};
const uint8_t button_to_key[] = {
    HID_KEY_1, HID_KEY_2, HID_KEY_3, HID_KEY_4,
    HID_KEY_5, HID_KEY_6, HID_KEY_7, HID_KEY_8,
    HID_KEY_9, HID_KEY_0, HID_KEY_A, HID_KEY_B,
    HID_KEY_C, HID_KEY_D, HID_KEY_E
};
#endif

// Mapping for SpaceMouse Pro buttons (matching the serial version's intent)
uint8_t button_bits[] = { 12, 13, 14, 15, 22, 25, 23, 24, 0, 1, 2, 4, 5, 8, 26 };

// Core1: handle host events
void core1_main() {
    sleep_ms(10);

    // Use tuh_configure() to pass pio configuration to the host stack
    pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
    pio_cfg.pin_dp = 0; // GPIO 0 is D+, GPIO 1 is D-
    pio_cfg.pinout = PIO_USB_PINOUT_DPDM;
    tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);

    // To run USB SOF interrupt in core1, init host stack for pio_usb (roothub port1) on core1
    tuh_init(1);

    while (true) {
        tuh_task(); // tinyusb host task
    }
}

// This is required for PIO-USB host to work with TinyUSB
void irq_handler_fingerprint() {} // Placeholder for potential future use

int main() {
    // Sysclock should be multiple of 12MHz for PIO-USB.
    set_sys_clock_khz(120000, true);

    board_init();
    stdio_init_all();

    printf("SpaceMouse USB-to-USB Remapper started\n");

    // Initialize device stack on native usb (roothub port0)
    // It is important to init device BEFORE starting core1 to ensure interrupts are set up
    tud_init(0);

    multicore_reset_core1();
    multicore_launch_core1(core1_main);

    while (true) {
        tud_task(); // tinyusb device task

#ifdef ENABLE_LONG_PRESS
        static uint32_t last_check_ms = 0;
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_check_ms > 10) {
            last_check_ms = now;
            bool changed = false;
            for (int i = 0; i < 15; i++) {
                if ((host_buttons & (1 << i)) && !long_press_sent[i]) {
                    if (now - press_start_ms[i] > 2000) {
                        long_press_sent[i] = true;
                        changed = true;
                        
                        // Send Keyboard report (Win+Alt+Key)
                        // Report ID 32 (0x20) as defined in descriptors.c
                        uint8_t kb_report[8] = {0};
                        kb_report[0] = KEYBOARD_MODIFIER_LEFTGUI | KEYBOARD_MODIFIER_LEFTALT;
                        kb_report[2] = button_to_key[i];
                        tud_hid_report(32, kb_report, 8);
                    }
                }
            }
            if (changed) {
                // Re-calculate buttons_report to "release" the long-pressed button in SpaceMouse report
                memset(buttons_report, 0, sizeof(buttons_report));
                for (int i = 0; i < 15; i++) {
                    if ((host_buttons & (1 << i)) && !long_press_sent[i]) {
                        buttons_report[button_bits[i] / 8] |= (1 << (button_bits[i] % 8));
                    }
                }
                buttons_pending = true;
            }
        }
#endif

        if (trans_pending && tud_hid_ready()) {
            tud_hid_report(1, trans_report, 6);
            trans_pending = false;
        }
        if (rot_pending && tud_hid_ready()) {
            tud_hid_report(2, rot_report, 6);
            rot_pending = false;
        }
        if (buttons_pending && tud_hid_ready()) {
            tud_hid_report(3, buttons_report, 6);
            buttons_pending = false;
        }
    }

    return 0;
}

//--------------------------------------------------------------------+
// USB Host HID Callbacks
//--------------------------------------------------------------------+

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {
    (void)desc_report;
    (void)desc_len;
    printf("HID device mounted: dev_addr %u, instance %u\n", dev_addr, instance);
    
    // Start receiving reports
    if (!tuh_hid_receive_report(dev_addr, instance)) {
        printf("Error: cannot request report from dev %u instance %u\n", dev_addr, instance);
    }
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    printf("HID device unmounted: dev_addr %u, instance %u\n", dev_addr, instance);
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
    if (len == 0) return;

    // Many 3Dconnexion devices use report IDs.
    // If the report descriptor indicates no report ID, TinyUSB might omit the first byte.
    // However, for SpaceMouse Plus USB, it usually has ID 1, 2, and 3.
    
    uint8_t report_id = report[0];
    uint8_t const* data = report + 1;
    uint16_t data_len = len - 1;

    switch (report_id) {
        case 1: // Translation
            if (data_len >= 6) {
                memcpy(trans_report, data, 6);
                trans_pending = true;
            }
            break;

        case 2: // Rotation
            if (data_len >= 6) {
                memcpy(rot_report, data, 6);
                rot_pending = true;
            }
            break;

        case 3: // Buttons
            if (data_len >= 1) {
                // The SpaceMouse Plus XT USB sends 2 or 3 bytes of buttons.
                // We map them to the SpaceMouse Pro report structure.
                memset(buttons_report, 0, sizeof(buttons_report));
                
                uint32_t buttons = 0;
                if (data_len >= 3) {
                    buttons = data[0] | (data[1] << 8) | (data[2] << 16);
                } else if (data_len >= 2) {
                    buttons = data[0] | (data[1] << 8);
                } else {
                    buttons = data[0];
                }

#ifdef ENABLE_LONG_PRESS
                host_buttons = buttons;
                uint32_t now = to_ms_since_boot(get_absolute_time());
                for (int i = 0; i < 15; i++) {
                    bool is_pressed = (buttons & (1 << i));
                    bool was_pressed = (last_host_buttons & (1 << i));
                    
                    if (is_pressed && !was_pressed) {
                        press_start_ms[i] = now;
                        long_press_sent[i] = false;
                    } else if (!is_pressed && was_pressed) {
                        if (long_press_sent[i]) {
                            // Send Keyboard release
                            uint8_t kb_release[8] = {0};
                            tud_hid_report(32, kb_release, 8);
                            long_press_sent[i] = false;
                        }
                    }
                }
                last_host_buttons = buttons;
#endif

                // Map the first 15 bits using the button_bits table
                for (int i = 0; i < 15; i++) {
#ifdef ENABLE_LONG_PRESS
                    if ((buttons & (1 << i)) && !long_press_sent[i]) {
#else
                    if (buttons & (1 << i)) {
#endif
                        buttons_report[button_bits[i] / 8] |= (1 << (button_bits[i] % 8));
                    }
                }
                buttons_pending = true;
            }
            break;

        default:
            // Some devices might not have report IDs, or use different ones.
            // If it's a 6-byte report, it might be Translation or Rotation without ID.
            // But we'll stick to 3Dconnexion standard for now.
            break;
    }

    // Continue receiving reports
    if (!tuh_hid_receive_report(dev_addr, instance)) {
        printf("Error: cannot request report from dev %u instance %u\n", dev_addr, instance);
    }
}
#endif

#ifdef MAGELLAN_TARGET_RS232
#define UART_ID uart1
#define BAUD_RATE 9600
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE
#define UART_TX_PIN 4
#define UART_RX_PIN 5

// ... HID report descriptors and other necessary definitions will go here ...

// Main application entry
int main(void)
{
    board_init();
    tusb_init();

    // UART initialization
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

    printf("Magellan RS232 to USB adapter started\n");

    while (1)
    {
        tud_task(); // TinyUSB device task

        // Check for UART data and process it
        if (uart_is_readable(UART_ID)) {
            uint8_t ch = uart_getc(UART_ID);
            // Process the received character from Magellan
            // This is where the logic to parse the Magellan protocol will go
        }
    }

    return 0;
}
#endif

//--------------------------------------------------------------------+
// USB Device HID Callbacks (PC side)
//--------------------------------------------------------------------+

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    (void) itf;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) bufsize;
}

uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    (void) itf;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;
    return 0;
}
