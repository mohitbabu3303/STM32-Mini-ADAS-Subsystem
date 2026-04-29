## &#x20; **STM32 Mini ADAS Subsystem (FreeRTOS)**



&#x20;📌 Overview



This project implements a real-time ADAS (Advanced Driver Assistance System) prototype on STM32 using FreeRTOS.

It simulates core automotive safety features such as:



&#x20;🚧 Lane Departure Warning

&#x20;🚗 Obstacle Proximity Alert



The system is designed using a modular RTOS-based architecture, similar to real automotive ECUs.





&#x20;System Architecture





SensorTask → sensorQueue → DecisionTask → alertQueue → AlertTask





&#x20;Task Responsibilities



&#x20;SensorTask



&#x20;  Simulates input data (lane status \& obstacle distance)

&#x20;  Sends structured data via `sensorQueue`



&#x20;DecisionTask



&#x20;  Receives sensor data

&#x20;  Applies safety logic:



&#x20;    Lane deviation detection

&#x20;    Obstacle distance threshold

&#x20;  Sends decisions via `alertQueue`



&#x20;AlertTask



&#x20;  Receives alert signals

&#x20;  Outputs warnings (UART / future buzzer/LED)



\---



&#x20;Key Features



&#x20;Real-time task scheduling using FreeRTOS (CMSIS-RTOS v2)

&#x20;Inter-task communication using message queues

&#x20;Modular and scalable system design

&#x20;UART-based debugging output (`printf` redirection)

&#x20;Designed with automotive-style pipeline thinking



\---



&#x20;Data Flow Explained



1\. SensorTask generates:



&#x20;   `lane\\\_status` (0/1)

&#x20;   `obstacle\\\_distance` (0–100)



2\. Data is sent via:



&#x20;  ```

&#x20;  sensorQueue

&#x20;  ```



3\. DecisionTask processes:



&#x20;   Lane deviation → Warning

&#x20;   Distance < threshold → Alert



4\. Alert data is sent via:



&#x20;  ```

&#x20;  alertQueue

&#x20;  ```



5\. AlertTask prints warnings:



&#x20;  ```

&#x20;  ALERT: Lane Deviation!

&#x20;  ALERT: Obstacle Too Close!

&#x20;  ```



\---



&#x20;Tech Stack



&#x20;Microcontroller: STM32 (ARM Cortex-M)

&#x20;RTOS: FreeRTOS (CMSIS-RTOS v2)

&#x20;Language: Embedded C

&#x20;IDE: STM32CubeIDE

&#x20;Communication: UART (for debugging)



\---



&#x20;📂 Project Structure



```

Mini\\\_ADAS\\\_RTOS/

│

├── Core/

│   ├── Inc/        → Header files

│   ├── Src/        → Application code (main.c, tasks)

│

├── Drivers/

│   ├── CMSIS/      → Core processor support

│   ├── HAL/        → Hardware abstraction layer

│

├── Middlewares/

│   └── FreeRTOS/   → RTOS kernel \\\& config

│

├── .ioc            → STM32Cube configuration file

├── .gitignore      → Ignored build/system files

```



\---



&#x20;Important Implementation Details



&#x20;Queue-Based Communication



&#x20;`sensorQueue` → Sensor → Decision

&#x20;`alertQueue` → Decision → Alert



This ensures:



&#x20;Thread safety

&#x20;Decoupled architecture

&#x20;Deterministic execution



\---



&#x20;UART Output (Debugging)



`printf()` is redirected to UART using:



```c

int \\\_write(int file, char ptr, int len)

{

\&#x20;   HAL\\\_UART\\\_Transmit(\\\&huart1, (uint8\\\_t)ptr, len, HAL\\\_MAX\\\_DELAY);

\&#x20;   return len;

}

```



\---



&#x20;Example Output



```

Sensor Sent -> Lane:1 Dist:12

ALERT: Lane Deviation!

ALERT: Obstacle Too Close!

```



\---



&#x20;Future Improvements



&#x20;Integration with real sensors (Ultrasonic / Camera)

&#x20;Edge AI for lane \& object detection

&#x20;Buzzer / LED alert system

&#x20;CAN bus communication

&#x20;Functional safety (ASIL-inspired design)



\---



&#x20;What This Project Demonstrates



&#x20;RTOS-based embedded system design

&#x20;Task synchronization \& inter-task communication

&#x20;Real-time data processing

&#x20;Automotive system architecture thinking



\---



&#x20;Author



Mohit Babu Pasam

Email: mohitbabuuol@gmail.com



\---



Notes



This is a simulation-based prototype designed to demonstrate embedded system architecture and ADAS concepts.

Hardware integration can be extended for real-world deployment.

