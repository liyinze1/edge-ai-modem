
#include "modem.h"
#include "serial_interface.h"

#define UDP_IP_HEADER_SIZE 28

// UDP Addresses and Sockets 
static int16_t server_send_fd;
static struct sockaddr_in addr_server_sendto;

// Random number initialised message ID
uint32_t msg_rand_id;

static char client_id_imei[16] = {0};												
static char client_rsrp[5] = {0};				
static char client_rsrp_val = 0;

static uint8_t txbuf[65535] = {0};
static uint16_t txbuf_len;

int8_t err;
char imei_buf[20 + sizeof("OK\r\n")];
char rsrp_buf[10 + sizeof("OK\r\n")];

bool is_connected = true;

uint16_t reconnection_times = 0;
K_SEM_DEFINE(lte_connected, 0, 1);
void rsrp_cb(char rsrp_value)
{
	client_rsrp_val = rsrp_value;
	printk("The RSRP of the system: %d\n", client_rsrp_val);
}

// - ------------------- Formated Data + Send - S ----------------------

// void modem_transmitData(uint16_t capMilliVolt, uint16_t sleepTime, uint16_t Distance_Data, 
// 						uint16_t solarV, uint16_t reconnection_times, uint16_t *lidar_reading_buff) {
// 	memset(txbuf, 0, TX_BUFFER_LEN);
// 	memcpy(&txbuf[0], client_id_imei, 15);		// Copy 15 bytes of data from client_id_imei to the txbuf buffer
// 												// IMEI often have 15 digits (lengths)
// 	txbuf[15] = (capMilliVolt >> 8) & 0xFFu;
// 	txbuf[16] = capMilliVolt & 0xFFu;
// 	txbuf[17] = (sleepTime >> 8) & 0xFFu;
// 	txbuf[18] = sleepTime & 0xFFu;
// 	txbuf[19] = (Distance_Data >> 8) & 0xFFu;
// 	txbuf[20] = Distance_Data & 0xFFu;
// 	txbuf[21] = (solarV >> 8) & 0xFFu;
// 	txbuf[22] = solarV & 0xFFu;
// 	txbuf[23] = (reconnection_times >> 8) & 0xFFu;
// 	txbuf[24] = reconnection_times & 0xFFu;

// 	// Insert lidar_reading_buff[100] data into the sent buffer
//     uint16_t offset = 25;
//     for (uint16_t i = 0; i < RAW_READING_NUMBERS; i++) {
//         txbuf[offset++] = (lidar_reading_buff[i] >> 8) & 0xFFu; // High byte
//         txbuf[offset++] = lidar_reading_buff[i] & 0xFFu;        // Low byte
//     }

// 	txbuf_len = TX_BUFFER_LEN;
//     modem_server_transmission_work_fn(NULL);

// }



/**
 * @brief: To send "Start" Byte/Header to the server
 * 			+ Header Byte = 0
 */
void modem_transmitData_startByte(void) {
	memset(txbuf, 0, sizeof(txbuf));
	txbuf[0] = 0;
	txbuf_len = 1;	
	modem_server_transmission_work_fn(NULL);
}


/**
 * @bried: To send "IMEI + AsTAR++ parameters" to the server
 * 			+ Header Byte: = 1
 */
void modem_transmitData_astar(uint16_t capMilliVolt, uint16_t sleepTime, 
						uint16_t solarV, uint16_t reconnection_times) {
	memset(txbuf, 0, TX_BUFFER_LEN_DEPTH_ASTAR);
	txbuf[0] = 1;		// Header Byte
	// IMEI often have 15 digits (lengths)
	memcpy(&txbuf[1], client_id_imei, 15);		
	txbuf[16] = (capMilliVolt >> 8) & 0xFFu;
	txbuf[17] = capMilliVolt & 0xFFu;
	txbuf[18] = (sleepTime >> 8) & 0xFFu;
	txbuf[19] = sleepTime & 0xFFu;
	txbuf[20] = (solarV >> 8) & 0xFFu;
	txbuf[21] = solarV & 0xFFu;
	txbuf[22] = (reconnection_times >> 8) & 0xFFu;
	txbuf[23] = reconnection_times & 0xFFu;

	txbuf_len = TX_BUFFER_LEN_DEPTH_ASTAR;
    modem_server_transmission_work_fn(NULL);
}



/**
 * @brief: To send Depth/Distance to the server
 * 			+ Header Byte = 2
 */

void modem_transmitData_depth(volatile uint8_t *modem_tx_buf) {
	memset(txbuf, 0, sizeof(txbuf));
	txbuf[0] = 2;		// Header Byte
	// "modem_tx_len": obtained in UART callback
	txbuf_len = uart_rx_len - 3;				
	for (uint16_t i = 1; i <= txbuf_len; i++)
	{
		txbuf[i] =  modem_tx_buf[i+3];		// Ignore 03 first header Bytes 
	}
	modem_server_transmission_work_fn(NULL);
}


/**
 * @brief: To send the whole picture to the server
 * 			+ Header Byte = 3
 */
void modem_transmitData_picture(volatile uint8_t *modem_tx_buf) {
	memset(txbuf, 0, sizeof(txbuf));
	txbuf[0] = 3;		// Header Byte
	// "modem_tx_len": obtained in UART callback
	txbuf_len = uart_rx_len - 3;				
	for (uint16_t i = 1; i <= txbuf_len; i++)
	{
		txbuf[i] =  modem_tx_buf[i+3];		// Ignore 03 first header Bytes
	}
	modem_server_transmission_work_fn(NULL);
}


/**
 * @brief: To send "Stop" Byte/Header to the server
 * 			+ Header Byte = 4
 */
void modem_transmitData_stopByte(void) {
	memset(txbuf, 0, sizeof(txbuf));
	txbuf[0] = 4;
	txbuf_len = 1;	
	modem_server_transmission_work_fn(NULL);
}


void modem_server_transmission_work_fn(struct k_work *work)
{
	int8_t err;
	printk("\nIP address %s, Port number %d\n",
		       CONFIG_UDP_SERVER_ADDRESS_STATIC,
		       CONFIG_UDP_SERVER_PORT);	

	printk("\n*********************************************\n");
	printk("Data to send over LTE-M (as integers, with IMEI embedded as ASCII):\n");
	for(uint16_t i = 0; i < txbuf_len; i++) {
		printk("%d\t", txbuf[i]);
	}
	printk("\n*********************************************\n");
	
	err = send(server_send_fd, txbuf, (size_t)txbuf_len, 0);
	if (err < 0) {
		printk("Failed to transmit UDP packet, %d %s\n", errno, strerror(errno));
		// is_connected = false;
		printk("Failed to transmit UDP packet. is_connected = false \n");
	} else {
		// is_connected = true;
		printk("Successfully transmitted UDP packet\n");
	}
}
// --------------------- Formated Data + Send - E ----------------------


/**
 * @brief: This function is used for initialize the Modem, NOT FOR UDP.
 * 			Called in main().
*/ 
void modem_main_init(void)
{
	#if defined(CONFIG_NRF_MODEM_LIB) 
		modem_modem_init();
		err = modem_configure_low_power();
		if (err) {
			printk("Unable to set low power configuration, error: %d\n",
		    	   err);
		}
		modem_modem_connect(); 
		k_sem_take(&lte_connected, K_SECONDS(60));
	#endif

	err = modem_udp_init();
	if (err) {
		printk("Not able to initialize UDP server connection\n");
		return;
	}

	printk("Initialising modem info library...\n");
	err = modem_info_init();
	if (err) {
		printk("Unable to initialise modem info library, error: %d\n", err);
	}

    printk("Querying IMEI\n");
	/*err = nrf_modem_at_cmd(imei_buf, sizeof(imei_buf), "AT+CGSN");
	if (err) {
		printk("Not able to retrieve device IMEI from modem\n");
    	//return;
	}*/
	modem_info_string_get(MODEM_INFO_IMEI, imei_buf, sizeof(imei_buf));
	strncpy(client_id_imei, imei_buf, sizeof(client_id_imei) - 1);
	printk("IMEI : %s\n", client_id_imei);

	modem_info_string_get(MODEM_INFO_RSRP, rsrp_buf, sizeof(rsrp_buf));
	strncpy(client_rsrp, rsrp_buf, sizeof(client_rsrp) - 1);
	printk("RSRP : %s\n", client_rsrp);
}



// **************************************** Function Definitions **************************************
// --------------------- S - Modem to Cellular network connection ------------------
#if defined(CONFIG_NRF_MODEM_LIB)
	
	// To initialize the modem 
	void modem_modem_init(void)
	{
		int8_t err;

		printk("Modem initialization...\n");

		err = nrf_modem_lib_init();
		if (err) {
			printk("Failed to initialize modem library, error: %d\n", err);
			return;
		}

		#if defined(CONFIG_UDP_RAI_ENABLE)
			/* Enable Access Stratum RAI support for nRF9160.
	 		* Note: The 1.3.x modem firmware release is certified to be compliant with 3GPP Release 13.
	 		* %REL14FEAT enables selected optional features from 3GPP Release 14. The 3GPP Release 14
	 		* features are not GCF or PTCRB conformance certified by Nordic and must be certified
	 		* by MNO before being used in commercial products.
	 		* nRF9161 is certified to be compliant with 3GPP Release 14.
	 		*/
			if (is_nrf9160()) {
				err = nrf_modem_at_printf("AT%%REL14FEAT=0,1,0,0,0");
				if (err) {
					printk("Failed to enable Access Stratum RAI support, error: %d\n", err);
					return err;
				}
			}
		#endif

		err = lte_lc_init();
		if (err) {
			printk("Modem initialization failed, error: %d\n", err);
			return;
		}

		return;
	}

	int8_t modem_configure_low_power(void)
	{
		int8_t err;

		#if defined(CONFIG_UDP_PSM_ENABLE)
			/** Power Saving Mode */
			err = lte_lc_psm_req(true);
			if (err) {
				printk("lte_lc_psm_req, error: %d\n", err);
			}
		#else
			err = lte_lc_psm_req(false);
			if (err) {
				printk("lte_lc_psm_req, error: %d\n", err);
			}
		#endif

		#if defined(CONFIG_UDP_EDRX_ENABLE)
			/** enhanced Discontinuous Reception */
			err = lte_lc_edrx_req(true);
			if (err) {
				printk("lte_lc_edrx_req, error: %d\n", err);
			}
		#else
			err = lte_lc_edrx_req(false);
			if (err) {
				printk("lte_lc_edrx_req, error: %d\n", err);
			}
		#endif

		#if defined(CONFIG_UDP_RAI_ENABLE)
			/** Release Assistance Indication  */
			err = lte_lc_rai_req(true);
			if (err) {
				printk("lte_lc_rai_req, error: %d\n", err);
			}
		#endif

			return err;
	}

	// Connect MODEM to the Cellular Network
	void modem_modem_connect(void)
	{
		int8_t err;
		if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		} else {
			err = lte_lc_connect_async(modem_lte_handler); // Take callback function "modem_lte_handler" as argument
			if (err) {
				printk("Connecting to LTE network failed, error: %d\n", err);
				return;
			}
		}
	}

	void modem_lte_handler(const struct lte_lc_evt *const evt)
	{
		switch (evt->type) {
		case LTE_LC_EVT_NW_REG_STATUS:
			if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
	     		(evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
				is_connected = false;
				printk ("Cannot register the network: is_connected =  false \n");
				break;
			}
			printk("Network registration status: %s\n",
				evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
				"Connected - home network" : "Connected - roaming\n");
			
			is_connected = true;
			
			k_sem_give(&lte_connected);
			break;
		case LTE_LC_EVT_PSM_UPDATE:
			printk("PSM parameter update: TAU: %d, Active time: %d\n",
			evt->psm_cfg.tau, evt->psm_cfg.active_time);
			break;
		case LTE_LC_EVT_EDRX_UPDATE: {
			char log_buf[60];
			ssize_t len;
			len = snprintf(log_buf, sizeof(log_buf),
		       		"eDRX parameter update: eDRX: %f, PTW: %f\n",
		       		evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
			if (len > 0) {
				printk("%s\n", log_buf);
			}
			break;
		}
		case LTE_LC_EVT_RRC_UPDATE:
			printk("RRC mode: %s\n",
				evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
				"Connected" : "Idle\n");
			if (evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED)
				is_connected = true;
			break;
		case LTE_LC_EVT_CELL_UPDATE:
			printk("LTE cell changed: Cell ID: %d, Tracking area: %d\n",
	       		evt->cell.id, evt->cell.tac);
			// Reconnect if the connection is lossed 
			if (evt->cell.id == -1)
			{
				is_connected = false;
				printk ("++ Cannot find CELL ID. is_connected = false! \n");
			}
			break;

		default:
			break;
		}
	}
#endif
// --------------------- E - Modem to Cellular network connection ------------------


// ---------------------------- S - UDP configuration ----------------------------
int8_t modem_udp_init(void)
{
	int8_t err;
	int8_t status;

	// Initialise random number based message ID
	msg_rand_id = sys_rand32_get();

	// Set up server and client addresses
	addr_server_sendto.sin_family = AF_INET;
	addr_server_sendto.sin_port = htons(CONFIG_UDP_SERVER_PORT);
	status = inet_pton(AF_INET, CONFIG_UDP_SERVER_ADDRESS_STATIC, &addr_server_sendto.sin_addr);
	if (status == 0) {
		printk("src does not contain a character string representing a valid network address\n");
		return -1;
	} else if(status < 0) {
		printk("inet_pton failed: %d %s\n", errno, strerror(errno));
		err = -errno;
		goto error;
	}

	// Set up server socket and connect
	server_send_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (server_send_fd < 0) {
		printk("UDP socket creation failed: %d\n", errno);
		err = -errno;
		goto error;
	}

	// After udp_init() => Connect to the UDP 
	err = connect(server_send_fd, (struct sockaddr *)&addr_server_sendto,
		      sizeof(struct sockaddr_in));
	if (err < 0) {
		printk("Connect failed : %d\n", errno);
		goto error;
	}

	return 0;
error:
	modem_server_disconnect();
	return err;
}

// Disconnect MODEM to UDP, if UDP configuration is not successful
void modem_server_disconnect(void)
{
	(void)close(server_send_fd);
}
// ---------------------------- E - UDP configuration ----------------------------


//----------------------------------- S - Re-connection -----------------------------------
/** 
 * @brief: To check if the device is connected to the network
*/
bool check_network_connection(void)
{
	if (is_connected) {
        return true;
    } else {
        return false;
    }
}

/** 
 * @brief:To handle re-connecting to the cellular network when the connection is lost 
*/
int8_t reconnect_to_network(void)
{
	int8_t err4;
	printk("************ Attempting to reconnect... **************** \n");
	modem_modem_init();
	err4 = modem_configure_low_power();
		if (err4) {
			printk("Unable to set low power configuration, error: %d\n", err4);
		}
	reconnection_times = reconnection_times + 1;
	modem_modem_connect(); 
	if (k_sem_take(&lte_connected, K_SECONDS(60)) == 0) {
		printk("Finished Re-initialize the modem\n");
		is_connected = true;
	} 
	else{ 
		is_connected = false;
		printk("cannot connect to eNode B in 60s!\n");
	}

	err4 = modem_info_init();
	if (err4) {
		printk("Unable to initialise modem info library, error: %d\n", err4);
	}

    printk("Querying IMEI\n");
	modem_info_string_get(MODEM_INFO_IMEI, imei_buf, sizeof(imei_buf));
	strncpy(client_id_imei, imei_buf, sizeof(client_id_imei) - 1);
	printk("IMEI : %s\n", client_id_imei);

	//modem_info_rsrp_register(rsrp_cb);
	modem_info_string_get(MODEM_INFO_RSRP, rsrp_buf, sizeof(rsrp_buf));
	strncpy(client_rsrp, rsrp_buf, sizeof(client_rsrp) - 1);
	printk("RSRP : %s\n", client_rsrp);

	err4 = modem_udp_init();
	if (err4) {
		printk("Not able to initialize UDP server connection\n");
		return 1;
	}
    return 0;
}

/** 
 * @brief: To save the total number of reconnection attempts during entire operational period
*/
uint16_t reconnection_numbers(void)
{
	uint16_t ReconnectionTimes = reconnection_times;
	return ReconnectionTimes;
}
//----------------------------------- E - Re-connection -----------------------------------