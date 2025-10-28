Zephyr version v2.5.1 or v2.5.3

Author: [@LuckyMan23129](https://github.com/LuckyMan23129)
Revised by: [@liyinze1](https://github.com/liyinze1)


------------------------------------------------------------------------------------------------------------------------------------------------
@brief: This code configures the Flute node as the master, utilizing the integrated AsTAR++ module to schedule the Roadrunner.

S1: nRF Wakes up RR to start working<br> 
S2: The nRF waits (sleep during waiting time by "k_sem_take(&uart_process_rx_done, K_FOREVER)") until the RR has finished the ML inference and then wake the nRF up by sending water_level or Whole_picture data via ASYNC UART to the nRF<br>
&nbsp;&nbsp;+ First Header Byte: "D"  : Depth data<br>
&nbsp;&nbsp;+ First Header Byte: "P"  : Send whole picture to the cloud<br>
&nbsp;&nbsp;+ First Header Byte: "E"  : Tell the nrf that RR has already sent everything (trigger k_sem_give(&uart_process_rx_done); , then in the main.c , it can start to run AStar)<br>
S3: nRF handles incoming data, and then send to the server<br>
S4: nRF run AsTART++ and decide the sleep mode of the RR based on the calculated Sleeping time.<br>
&nbsp;&nbsp;+ If sleep_time < 1800s:  Send "S" to RR, and then RR going Suspend-to-RAM mode<br>
&nbsp;&nbsp;+ If sleep_time >= 1800s: Send ""<br>



Packets format from nRF to cloud:

Headers:
1 : meta data (IMEI + AsTAR++ parameters)
2 : depth
3 : photo
4 : stop byte
