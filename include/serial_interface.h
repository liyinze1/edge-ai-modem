#ifndef SERIAL_INTERFACE_H
#define SERIAL_INTERFACE_H

#include <stdint.h>  // for uint16_t
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

#define UART_RX_TIMEOUT_US          2000
#define ZEPHYR_USER_NODE    DT_PATH(zephyr_user)

extern struct k_sem uart_data_ready;                // Finished receiving UART RX data 
extern struct k_sem uart_process_rx_done;           // Finished processing UART RX data   

extern uint8_t waterLevel_tx_len;
extern uint16_t waterLevel_tx_buf[2];
extern uint16_t waterlevel_tx;


void uart_init(void);
void uart_send_ack(void);
void uart_send_nack(void);
void uart_send_ready(void);
void uart_process_rx(void);
void uart_send_cmd_suspendRAM(void);
void uart_send_cmd_powerdown(void);


#endif /*SERIAL_INTERFACE_H*/