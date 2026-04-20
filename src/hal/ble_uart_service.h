#pragma once

void ble_uart_init(void);
void ble_uart_stop(void);
void ble_uart_loop(void);
bool ble_uart_is_active(void);    // BLE initialized and advertising/connected
bool ble_uart_is_connected(void); // client connected
void ble_uart_send(const char *data);
const char* ble_uart_get_pin(void);
