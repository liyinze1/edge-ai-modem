Zephyr version v2.5.1 or v2.5.3

Author: @LuckyMan23129
GUIDE
This folder is used to contain codes for:
    + 1. How to use Flute node as a master (with the integrated AsTAR++) to schedule Roadrunner


------------------------------------------------------------------------------------------------------
Flute board with the integrated ASTAR++ works as master to schedule and control the RoadRunner as the follows:
-	Power the RoadRunner so that it can take picture and run ML model to detect water
•	Remember that only turn on the camera when needed
-	Then, after the RoadRunner can interfere the water, it send data to the NRF9160
•	Remember that only turn on the camera when needed
-	The, the NRF power down the ROadRunner and Camera sensor => send received data to the server => Sleep

-	 For the Flute board site, we need to read all data:
•	Vcap (Add new Adc Channel to read + no need to use the ½ Voltage divider because maxVcap =3V)
•	Và tất cả các data khác như Flute board đã làm previously



Chương 1: Progress
CT simple to điều khiển RoadRunner bằng một chân (wakeup RoadRunner)
================================================================



---------------------------------------------------------------------------------------------
"02.nrf": UART between nRF and RoadRunner
-	Use the nRF's pin 14 to wake up RoadRunner
-	Use Pin 22 as a input GPIO interrupt to wake up nrf when it is in Active sleep (During the time interval the RR takes picture + ML inference) 
  Nhưng add more:
-	UART communication between nRF and RoadRunner
•	Xem Ý tưởng CT UART tại « Phụ Lục 02.nrf » 
•	Tham khảo UART code của RTASP để viết code này


----------------------------------------------------------------------------------------------
"01.nrf":
    - Use the nRF's pin 14 to wake up RoadRunner
    - Power profile (only consider the Power consumption of nRF9160) as follows:
        + I without 5V Booster: 	8uA at Vps = 4.5V
        + I with 5V Booster:		94uA at Vcap = 2.4V
        + I with 5V Booster:		47uA at Vcap = 4.5V
Chương 2: Phụ Lục 02.nrf
2.1. Ý tưởng UART
 

-	RR khởi động rồi ngủ liền
-	Nrf set pin 14 về 0 để đánh thức RR
-	NRF ngủ inactive (chỉ đánh thức bằng trigger pin)
-	RR thức dậy, chụp ảnh + ML interference
-	RR set 1 pin để đánh thức nrf (đang ngủ Inactive)
-	Nrf thức dậy: Gửi UART ack đến RR
-	Nếu RR nhận được ACK message from the nRF
•	Send data (dummy water level) từ đến RR
-	Nrf nhận data via UART
-	RR sleep
-	Nrf chạy AsTAR++ > timd được sleep timer => enter active sleep

	Lặp tại Chu kỳ tiếp theo
