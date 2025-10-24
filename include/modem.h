#ifndef MODEM_H
#define MODEM_H

#include <modem/lte_lc.h>
#include <zephyr/net/socket.h>
#include <zephyr/random/rand32.h>
#include <nrf_socket.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <nrf_modem_at.h>
#include <modem/modem_info.h>
#include <modem/nrf_modem_lib.h>
#include "i2c_sensor4.h"


#define UDP_IP_HEADER_SIZE 28
#define IMEI_LEN        15
#define PICTURE_LEN     
#define TX_BUFFER_LEN_DEPTH         (IMEI_LEN + 2)
#define TX_BUFFER_LEN_DEPTH_ASTAR   (IMEI_LEN + 10)
// #define TX_BUFFER_LEN_PICTURE   (IMEI_LEN + 100)

#define MSGTYPE_INVALID                 0
#define MSGTYPE_SURVEY_RESULT_UPLOAD    1 
#define MSGTYPE_ACK                     5
#define MSGTYPE_ACK_UPDATEREADY         6 

// ----------------------- S - Preparing data for sending -----------------------
// To send ONLY "Depth" to the server"
void modem_transmitData_depth(uint16_t Distance_Data);     
// To send "Depth" + "ASTAR parameter" to the server
void modem_transmitData_depth_astar(uint16_t capMilliVolt, uint16_t sleepTime, uint16_t Distance_Data,
						uint16_t solarV, uint16_t reconnection_times);
// To send WHOLE PICTURE to the server"
void modem_transmitData_picture( uint16_t *modem_tx_buf);
// ----------------------- E - Preparing data for sending -----------------------

// Sending data to server
void modem_server_transmission_work_fn(struct k_work *work);

// called in "main" to initialize the MODEM and UDP connection
void modem_main_init(void);  
void modem_lte_handler(const struct lte_lc_evt *const evt);

// Modem to Cellular network connection
void modem_modem_init(void);
int8_t modem_configure_low_power(void);
void modem_modem_connect(void);

// UDP connection
int8_t modem_udp_init(void);
void modem_server_disconnect(void);
void modem_server_disconnect_rx(void);

//----------------------------- S - Reconnection ----------------------------
bool check_network_connection(void);
int8_t reconnect_to_network(void);
uint16_t reconnection_numbers(void);    // The number of reconnections
//---------------------------- E - Reconnection --------------------------
#endif