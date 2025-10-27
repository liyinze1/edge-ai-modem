#include "serial_interface.h"
#include "modem.h"

static const struct device *uart2_dev = DEVICE_DT_GET(DT_NODELABEL(uart2));

LOG_MODULE_REGISTER(UART_interface);

// static volatile uint8_t uart_rx_buf[65535];

// Shared counters, volatile since ISR + thread both access them
// static volatile uint16_t uart_rx_len;
// static volatile uint16_t uart_rx_offset;

K_SEM_DEFINE(uart_data_ready, 0, 1);          // RX Data is received well 
K_SEM_DEFINE(uart_process_rx_done, 0, 1);     // Finished processing UART RX data



static void uart_cb(const struct device *dev, struct uart_event *evt,
             void *user_data) {
    switch (evt->type) {
        case UART_TX_DONE:
            // LOG_INF("UART_TX_DONE");
            break;

        case UART_TX_ABORTED:
            LOG_ERR("UART_TX_ABORTED");
            break;

        case UART_RX_RDY:
            uart_rx_len = evt->data.rx.len;
            uart_rx_offset = evt->data.rx.offset;
            uart_rx_disable(uart2_dev);                 // Received enough bytes => stop listening for new incoming data
			k_sem_give(&uart_data_ready);               // Allow the <uart_process_rx()> to be executed to handle incoming data in another Functions
            break;

        case UART_RX_BUF_REQUEST:
            break;

        case UART_RX_BUF_RELEASED:
            break;

        case UART_RX_DISABLED:  // RX stopped, re-enables with uart_rx_enable
            if (uart_rx_enable(uart2_dev, (uint8_t *)uart_rx_buf, sizeof(uart_rx_buf),
                               UART_RX_TIMEOUT_US)) {
                LOG_ERR("Couldn't assign UART buffer");
            }
            break;

        case UART_RX_STOPPED:
            LOG_INF("UART_RX_STOPPED, reason: %u", evt->data.rx_stop.reason);
            break;

        default:
            break;
    }
}



void uart_init(void) {
    if (!device_is_ready(uart2_dev)) {
        LOG_ERR("UART device not found!");
        return;
    }
    if (uart_callback_set(uart2_dev, uart_cb, NULL)) {
        LOG_ERR("Couldn't set UART callback function");
        return;
    }
    if (uart_rx_enable(uart2_dev, (uint8_t *)uart_rx_buf, sizeof(uart_rx_buf),
                       UART_RX_TIMEOUT_US)) {
        LOG_ERR("Couldn't assign UART buffer");
        return;
    }
    LOG_INF("UART init successfully");
}


/**
 *  Tells the RR “I am awake and ready to communicate” 
 *  Called in GPIO
 */
void uart_send_ready(void) {
    static const uint8_t tx_char = 'A';
    if (uart_tx(uart2_dev, &tx_char, sizeof(tx_char), SYS_FOREVER_US)) {
        LOG_ERR("Couldn't send Acknowledgement");
    }
}


/**
 *  Tells the RR “Please fall into suspend-to-RAM mode” 
 */
void uart_send_cmd_suspendRAM(void) {
    static const uint8_t tx_char = 'S';
    if (uart_tx(uart2_dev, &tx_char, sizeof(tx_char), SYS_FOREVER_US)) {
        LOG_ERR("Couldn't send suspend-to-RAM command");
    }
}

/**
 *  Tells the RR “Please fall into power-down mode” 
 */
void uart_send_cmd_powerdown(void) {
    static const uint8_t tx_char = 'P';
    if (uart_tx(uart2_dev, &tx_char, sizeof(tx_char), SYS_FOREVER_US)) {
        LOG_ERR("Couldn't send power-down command");
    }
}


void uart_send_ack(void) {
    static const uint8_t tx_char = 'A';
    if (uart_tx(uart2_dev, &tx_char, sizeof(tx_char), SYS_FOREVER_US)) {
        LOG_ERR("Couldn't send ACK");
    }
}

// void uart_send_nack(void) {
//     static const uint8_t tx_char = 'N';
//     if (uart_tx(uart2_dev, &tx_char, sizeof(tx_char), SYS_FOREVER_US)) {
//         LOG_ERR("Couldn't send NACK");
//     }
// }

void uart_process_rx(void) {
    LOG_INF("current offset %d, current length %d", uart_rx_offset, uart_rx_len);
    uint8_t payload_len_predec;

    if (uart_rx_len >= 1) {
        switch (uart_rx_buf[(uart_rx_offset + 0) % sizeof(uart_rx_buf)]) {
            
            //-----------------------------------------------------------------------------------------------------
            // if received bytes: D  00 02 xx xx  [0x11 0x22 0x33 0x44 0x55]
            //      + 'D' = Depth packet
            //      +  00 05 = 5 bytes payload length predeclared on RR
            //      +  Payload = [0x11, 0x22, 0x33, 0x44, 0x55]
            //-----------------------------------------------------------------------------------------------------
            case 'D':
                LOG_INF("Bytes RX: %u (bytes)", uart_rx_len);
                payload_len_predec =  (uart_rx_buf[(uart_rx_offset + 1) % sizeof(uart_rx_buf)] << 8) | uart_rx_buf[(uart_rx_offset + 2) % sizeof(uart_rx_buf)];
                if (payload_len_predec != uart_rx_len - 3) {
                    LOG_ERR("Message specified length %u doesn't match rx'd %u", payload_len_predec, uart_rx_len - 3);
                    // uart_send_nack();
                    break;
                }

                // Trim message type and length (first 3 bytes)
                for (size_t i = 0; i < (uart_rx_len - 3); i++) {
                    waterLevel_tx_buf[i] = uart_rx_buf[(uart_rx_offset + i + 3) % sizeof(uart_rx_buf)];
                }
                waterLevel_tx_len = uart_rx_len - 3;
                uart_send_ack();
                waterlevel_tx = (waterLevel_tx_buf[0] << 8) | waterLevel_tx_buf[1];
                LOG_INF("Water level is %d (cm)", waterlevel_tx);


                // to-do: send data to server
                modem_transmitData_startByte();
                modem_transmitData_depth(uart_rx_buf);
                modem_transmitData_stopByte();

                break;

            case 'P':
                LOG_INF("Received Picture from RR");
                payload_len_predec =  (uart_rx_buf[(uart_rx_offset + 1) % sizeof(uart_rx_buf)] << 8) | uart_rx_buf[(uart_rx_offset + 2) % sizeof(uart_rx_buf)];
                if (payload_len_predec != uart_rx_len - 3) {
                    LOG_ERR("Message specified length %u doesn't match rx'd %u", payload_len_predec, uart_rx_len - 3);
                    // uart_send_nack();
                    break;
                }
                uart_send_ack();

                // to-do: send Picture to server
                modem_transmitData_startByte();
                modem_transmitData_picture(uart_rx_buf);
                modem_transmitData_stopByte();

                break;

            case 'E':
                LOG_INF("Received end message from RR");
                k_sem_give(&uart_process_rx_done);              // Finished processing UART data
                break;

            default:
                LOG_ERR("Received unknown message type");
                // uart_send_nack();
                break;
        }
    }
}




// ---------------------------------------- S- Send Data to Cloud ------------------------------------
/**
 * How can the server distinguish between Depth, Picture or AsTAR Data type?
 * Do we need to spilit the whole picture data into small chunks? 
 */

void send_astar_params(void)
{

}


void send_depth(void)
{
    
}


void Send_picture(void)
{
    
}


// ---------------------------------------- E- Send Data to Cloud ------------------------------------
















// ========================================= References ===========================================
/*
Step-by-step sequence at runtime
    1.	// Semaphore: signals "new data available"
    	K_SEM_DEFINE(uart_data_ready, 0, 1); // (count =0) => đang khóa Main thread
    3.	UART hardware receives a byte.
    4.	ISR runs:
        •	Stores byte into uart_rx_buf.
        •	Updates uart_rx_len.
        •	Calls k_sem_give(&uart_data_ready).   // Mở khóa cho main thread
    5.	Main thread was blocked in k_sem_take() → now it wakes up.
    6.	Main thread reads bytes from buffer (using uart_rx_len / uart_rx_offset).
    7.	When buffer is emptied, it goes back to waiting.

*/