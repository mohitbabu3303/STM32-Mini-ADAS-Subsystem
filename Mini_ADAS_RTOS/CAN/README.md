# CAN Bus Integration for STM32 ADAS

This directory contains the CAN bus communication layer for the STM32-based Mini ADAS system.

## Files

- **can.h**: CAN peripheral initialization and callback declarations
- **can.c**: CAN peripheral implementation and interrupt handlers
- **can_messages.h**: CAN message definitions and structures

## CAN Configuration

The system uses the following configuration:

- **Baud Rate**: 500 kbps (standard automotive rate)
- **Mode**: Normal (production mode)
- **Identifier Type**: Standard (11-bit)
- **Filter**: Accepts all messages (FilterIdHigh/Low = 0x0000)

## CAN Message IDs

| ID | Purpose | Data Structure |
|---|---|---|
| 0x100 | Sensor Data | lane_status, obstacle_distance |
| 0x200 | Decision Alerts | lane_warning, obstacle_warning, risk_level |
| 0x300 | System Status | system_state, task_count, error_flags |

## Integration with FreeRTOS

The CAN module integrates with FreeRTOS tasks:

- **CAN_TxTask**: Transmits sensor and decision data over CAN bus
- **CAN_RxTask**: Receives and processes incoming CAN messages
- **SensorTask**: Generates sensor data
- **DecisionTask**: Processes sensor data and generates alerts

## Hardware Requirements

For real hardware deployment, you will need:

- STM32F1xx microcontroller with CAN peripheral
- CAN transceiver (e.g., MCP2551, TJA1050, SN65HVD230)
- CAN bus twisted pair (CAN_H and CAN_L)
- 120-ohm termination resistors at both ends of the bus

## Usage

The CAN peripheral is initialized in `main.c` during system startup. Messages are transmitted and received through the FreeRTOS task scheduler.

Example transmission:

```c
CAN_TxHeaderTypeDef TxHeader;
uint8_t TxData[8];

TxHeader.StdId = CAN_ID_SENSOR_DATA;
TxHeader.IDE = CAN_ID_STD;
TxHeader.RTR = CAN_RTR_DATA;
TxHeader.DLC = 8;

// Fill TxData with sensor information
HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox);
```

## Future Enhancements

- Implement CAN FD for higher bandwidth
- Add message filtering by ID
- Implement error handling and bus-off recovery
- Add DBC file support for message definition
