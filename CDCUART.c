#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "hardware/timer.h"
#include "bsp/board.h"
#include "tusb.h"

#define UART_ID     uart0

#define TX_PIN      0
#define RX_PIN      1

#define LED_PIN     25
#define LED_LEVEL   1
#define LED_PULSE   20 // 20 ms.

static uint32_t millis() {
    return time_us_64() / 1000;
}

static void init_uart() {
    uart_init(UART_ID, 115200);

    gpio_set_function(TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(RX_PIN, GPIO_FUNC_UART);
}

#ifdef LED_PIN
static void init_led() {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, true);
    gpio_put(LED_PIN, ! LED_LEVEL);
}
#endif

int main() {
    board_init();

    // init host stack on configured roothub port
    tud_init(BOARD_TUD_RHPORT);

    init_uart();

#ifdef LED_PIN
    init_led();

    uint32_t led_time = 0;
#endif

    while (true) {
        tud_task();

        if (tud_cdc_connected()) {
#ifdef LED_PIN
            if (tud_cdc_available()) {
                gpio_put(LED_PIN, LED_LEVEL);
                led_time = millis();

                while (tud_cdc_available()) {
                    uart_putc_raw(UART_ID, (char)tud_cdc_read_char());
                }
            }

            if (uart_is_readable(UART_ID)) {
                gpio_put(LED_PIN, LED_LEVEL);
                led_time = millis();

                while (uart_is_readable(UART_ID)) {
                    tud_cdc_write_char(uart_getc(UART_ID));
                }
                tud_cdc_write_flush();
            }
#else
            while (tud_cdc_available()) {
                uart_putc_raw(UART_ID, (char)tud_cdc_read_char());
            }

            if (uart_is_readable(UART_ID)) {
                while (uart_is_readable(UART_ID)) {
                    tud_cdc_write_char(uart_getc(UART_ID));
                }
                tud_cdc_write_flush();
            }
#endif
        }

#ifdef LED_PIN
        if (led_time && (millis() - led_time >= LED_PULSE)) {
            gpio_put(LED_PIN, ! LED_LEVEL);
            led_time = 0;
        }
#endif
    }
}

void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const *line_coding) {
    uart_set_baudrate(UART_ID, line_coding->bit_rate);
//    uart_set_format(UART_ID, line_coding->data_bits, line_coding->stop_bits, line_coding->parity);
}
