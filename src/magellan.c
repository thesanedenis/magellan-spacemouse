#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "tusb.h"

#ifdef MAGELLAN_TARGET_RS232
#define UART_ID uart0
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#else
// Default target (Pico GPIO 20/21)
#define UART_ID uart1
#define UART_TX_PIN 20
#define UART_RX_PIN 21
#endif

uint16_t trans_report[3];
uint16_t rot_report[3];
uint8_t buttons_report[6];

volatile bool trans_pending = false;
volatile bool rot_pending = false;
volatile bool buttons_pending = false;

uint8_t button_bits[] = { 12, 13, 14, 15, 22, 25, 23, 24, 0, 1, 2, 4, 5, 8, 26 };

uint32_t current_buttons = 0;
uint32_t star_press_ms = 0;
bool star_active = false;

int main(void)
{
    // Initialize USB only
    tusb_init();

    // UART configuration
    uart_init(UART_ID, 9600);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);

    // Power stabilization - very long wait to ensure internal Magellan caps are charged
    sleep_ms(3000);

    // Drain any garbage that might have been received during power-on
    while (uart_is_readable(UART_ID)) {
        uart_getc(UART_ID);
    }

    // Flush/Reset phase: send several carriage returns to clear Magellan's RX buffer
    for (int i = 0; i < 5; i++) {
        uart_puts(UART_ID, "\r");
        sleep_ms(100);
    }

    // Wake up Magellan and set to streaming mode
    // We send m3 multiple times with very long gaps to ensure one hits correctly
    for (int i = 0; i < 3; i++) {
        uart_puts(UART_ID, "m3\r");
        sleep_ms(500);
    }

    uart_tx_wait_blocking(UART_ID);

    // Final drain before starting main loop
    while (uart_is_readable(UART_ID)) {
        uart_getc(UART_ID);
    }

    uint8_t buf[64];
    uint8_t idx = 0;

    while (1)
    {
        tud_task();
        uint32_t now = to_ms_since_boot(get_absolute_time());

        // Long press logic for '*' button (index 8 in current_buttons)
        uint8_t new_report[6] = {0};
        bool star_is_pressed = (current_buttons & (1 << 8)); 
        
        if (star_is_pressed) {
            if (star_press_ms == 0) star_press_ms = now;
            if (now - star_press_ms > 1000) star_active = true;
        } else {
            star_press_ms = 0;
            star_active = false;
        }

        for (int i = 0; i < 15; i++) {
            if (current_buttons & (1 << i)) {
                if (i == 8) { // '*' button requires long press
                    if (star_active) new_report[button_bits[i] / 8] |= (1 << (button_bits[i] % 8));
                } else {
                    new_report[button_bits[i] / 8] |= (1 << (button_bits[i] % 8));
                }
            }
        }

        if (memcmp(new_report, buttons_report, 6) != 0) {
            memcpy(buttons_report, new_report, 6);
            buttons_pending = true;
        }

        // HID Reporting logic
        if (tud_hid_ready()) {
            if (trans_pending) {
                tud_hid_report(1, trans_report, 6);
                trans_pending = false;
            } else if (rot_pending) {
                tud_hid_report(2, rot_report, 6);
                rot_pending = false;
            } else if (buttons_pending) {
                tud_hid_report(3, buttons_report, 6);
                buttons_pending = false;
            }
        }

        // UART Parser (Original robust logic)
        while (uart_is_readable(UART_ID)) {
            uint8_t ch = uart_getc(UART_ID);
            buf[idx] = ch;
            idx = (idx + 1) % sizeof(buf);
            
            if (ch == '\r') {
                if (idx >= 25 && buf[0] == 'd') { // Movement
                    int16_t values[6];
                    for (int i = 0; i < 6; i++) {
                        values[i] = -32768;
                        for (int j = 0; j < 4; j++) {
                            values[i] += (buf[1 + i * 4 + 3 - j] & 0xf) << (4 * j);
                        }
                    }
                    trans_report[0] = values[0];
                    trans_report[1] = values[2];
                    trans_report[2] = -values[1];
                    rot_report[0] = values[3];
                    rot_report[1] = values[5];
                    rot_report[2] = -values[4];
                    trans_pending = true;
                    rot_pending = true;
                } else if (idx >= 4 && buf[0] == 'k') { // Buttons
                    uint32_t b = 0;
                    for (int i = 0; i < (idx - 2); i++) {
                        b |= (uint32_t)(buf[1 + i] & 0x0F) << (4 * i);
                    }
                    current_buttons = b;
                }
                idx = 0;
            }
        }
    }
    return 0;
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {}
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) { return 0; }

// TinyUSB Host callbacks (required by tinyusb_host library)
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {}
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {}
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {}
