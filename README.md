🚗 STM32 Mini ADAS Subsystem (FreeRTOS)

 📌 Overview

This project implements a real-time ADAS (Advanced Driver Assistance System) prototype on STM32 using FreeRTOS.

It simulates core automotive safety features such as:

* 🚧 Lane Departure Warning
* 🚗 Obstacle Proximity Alert

The system is designed using a modular RTOS-based architecture, similar to real automotive ECUs.

---

 🧠 System Architecture

SensorTask → sensorQueue → DecisionTask → alertQueue → AlertTask

---

 ⚙️ Task Responsibilities

 SensorTask

* Simulates input data (lane status & obstacle distance)
* Sends structured data via sensorQueue

 DecisionTask

* Receives sensor data
* Applies safety logic:

  * Lane deviation detection
  * Obstacle distance threshold
* Sends decisions via alertQueue

 AlertTask

* Receives alert signals
* Outputs warnings (UART / future buzzer/LED)

---

 🔥 Key Features

* Real-time task scheduling using FreeRTOS (CMSIS-RTOS v2)
* Inter-task communication using message queues
* Modular and scalable system design
* UART-based debugging output (printf redirection)
* Automotive-style pipeline architecture

---

 🔄 Data Flow Explained

1. SensorTask generates:

   * lane_status (0/1)
   * obstacle_distance (0–100)

2. Data is sent via sensorQueue

3. DecisionTask processes:

   * Lane deviation → Warning
   * Distance < threshold → Alert

4. Alert data is sent via alertQueue

5. AlertTask outputs:
   ALERT: Lane Deviation!
   ALERT: Obstacle Too Close!

---

 💻 C Simulation Version (Windows Runnable)

To validate logic before hardware deployment, a simplified C-based simulation is implemented.

 Features

* Lane change detection
* Obstacle distance monitoring
* Speed-based activation logic
* Console alerts

---

 Simulation Code

```
include <stdio.h>

int main() {
int lane_status, distance, speed;

printf("Enter lane status (0 = Normal, 1 = Lane Change): ");
scanf("%d", &lane_status);

printf("Enter obstacle distance (in meters): ");
scanf("%d", &distance);

printf("Enter vehicle speed (kmph): ");
scanf("%d", &speed);

if (speed < 30) {
    printf("ADAS inactive (Low Speed)\n");
} else {
    if (lane_status == 1) {
        printf("WARNING: Lane Deviation!\n");
    }
    if (distance < 20) {
        printf("ALERT: Obstacle Too Close!\n");
    }
    if (speed > 120) {
        printf("WARNING: Overspeed!\n");
    }
}

return 0;


}
```
---

 How to Run (Windows)

gcc main.c -o adas
adas

---

 🛠 Tech Stack

* Microcontroller: STM32 (ARM Cortex-M)
* RTOS: FreeRTOS (CMSIS-RTOS v2)
* Language: Embedded C
* IDE: STM32CubeIDE
* Communication: UART (debugging)

---

 📂 Project Structure

Mini_ADAS_RTOS/
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
│   └── FreeRTOS/   → RTOS kernel & config
│
├── .ioc            → STM32Cube configuration file
├── .gitignore      → Ignored files

---

 🔧 Important Implementation Details

Queue-Based Communication:

* sensorQueue → Sensor → Decision
* alertQueue → Decision → Alert

Ensures:

* Thread safety
* Decoupled design
* Deterministic execution

---

 📡 UART Output (Debugging)

int _write(int file, char *ptr, int len)
{
HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
return len;
}

---

 📊 Example Output

Sensor Sent -> Lane:1 Dist:12
ALERT: Lane Deviation!
ALERT: Obstacle Too Close!

---

 🚀 Future Improvements

* Real sensor integration (Ultrasonic / Camera)
* Edge AI (lane & object detection)
* Buzzer / LED alerts
* CAN bus communication
* Functional safety concepts (ASIL-inspired)

---

 🎯 What This Project Demonstrates

* RTOS-based embedded design
* Task scheduling & synchronization
* Inter-task communication
* Real-time decision making
* Automotive system architecture thinking

---

 👨‍💻 Author

Mohit Babu Pasam
Email: mohitbabuuol@gmail.com

---

 📌 Notes

This is a simulation-based prototype demonstrating ADAS architecture and embedded system design.
It can be extended to real-world automotive applications with hardware integration.

---

