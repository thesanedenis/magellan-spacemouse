#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "tusb.h"

#ifdef MAGELLAN_TARGET_RS232
#define UART_ID uart0
#define UART_TX_PIN 0
#define UART_RX_PIN 1

uint16_t trans_report[3];
uint16_t rot_report[3];
uint8_t buttons_report[6];

volatile bool trans_pending = false;
volatile bool rot_pending = false;
volatile bool buttons_pending = false;

uint8_t button_bits[] = { 12, 13, 14, 15, 22, 25, 23, 24, 0, 1, 2, 4, 5, 8, 26 };

int main(void)
{
    tusb_init();

    uart_init(UART_ID, 9600);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);

    sleep_ms(1000);

    uint8_t init_buf[] = { '\r', 'v', 'Q', '\r', 'm', '3', '\r' };
    uart_write_blocking(UART_ID, init_buf, sizeof(init_buf));

    uint8_t buf[64];
    uint8_t idx = 0;

    while (1)
    {
        tud_task();

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

        while (uart_is_readable(UART_ID)) {
            uint8_t ch = uart_getc(UART_ID);
            buf[idx] = ch;
            idx = (idx + 1) % sizeof(buf);
            
            if (ch == '\r') {
                if (idx >= 25 && buf[0] == 'd') {
                    int16_t v[6];
                    for (int i = 0; i < 6; i++) {
                        v[i] = -32768;
                        for (int j = 0; j < 4; j++) {
                            v[i] += (buf[1 + i * 4 + 3 - j] & 0xf) << (4 * j);
                        }
                    }
                    trans_report[0] = v[0];
                    trans_report[1] = v[2];
                    trans_report[2] = -v[1];
                    rot_report[0] = v[3];
                    rot_report[1] = v[5];
                    rot_report[2] = -v[4];
                    trans_pending = true;
                    rot_pending = true;
                } else if (idx >= 4 && buf[0] == 'k') {
                    uint16_t buttons = 0;
                    for (int i = 0; i < 3; i++) {
                        buttons |= (buf[1 + i] & 0x0f) << (4 * i);
                    }
                    memset(buttons_report, 0, 6);
                    for (int i = 0; i < 15; i++) {
                        if (buttons & (1 << i)) {
                            buttons_report[button_bits[i] / 8] |= (1 << (button_bits[i] % 8));
                        }
                    }
                    buttons_pending = true;
                }
                idx = 0;
            }
        }
    }
    return 0;
}
#endif

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {}
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) { return 0; }
