# Detailed Technical Report

## Predictive Maintenance RTOS Framework on QNX

---

## Abstract

Industrial process equipment frequently experiences performance degradation and unexpected failures due to thermal stress, delayed fault detection, and inefficient actuator control. Traditional monitoring systems often rely on static threshold alarms and non-deterministic software architectures, limiting their responsiveness in safety-critical environments.

This project presents a predictive maintenance framework implemented on **QNX Neutrino RTOS** for simulating an industrial thermal monitoring and adaptive cooling control system. The framework employs **native synchronous QNX message passing**, **priority-based thread scheduling**, **adaptive threshold-based fault prediction**, and a **finite state machine actuator model** to emulate real-time industrial monitoring and response behavior.

The system demonstrates deterministic inter-thread communication, modular subsystem decomposition, adaptive fault logic, and closed-loop thermal process control.

---

## Problem Statement

Industrial systems operating under dynamic thermal and electrical loads require continuous real-time monitoring to prevent equipment failure. Conventional systems often depend on fixed threshold alarms and delayed response mechanisms, which may result in unnecessary downtime or catastrophic failures.

The goal of this project is to design a real-time predictive maintenance framework capable of:

* Monitoring dynamic process conditions
* Predicting fault conditions before critical failure
* Triggering adaptive cooling responses
* Maintaining deterministic RTOS scheduling behavior

---

## Objectives

The primary objectives of the project are:

* To implement a real-time predictive maintenance simulation using QNX RTOS
* To demonstrate deterministic inter-process communication using QNX native IPC
* To model adaptive fault prediction using dynamic threshold logic
* To implement finite state machine-based actuator control
* To visualize real-time process conditions through dashboard monitoring
* To simulate industrial thermal process dynamics in software

---

## System Architecture

The framework follows a modular client-server RTOS architecture.

```text id="arch_block"
Thermal Sensor Thread
        ↓
AI Prediction Thread
        ↓
QNX IPC Server / State Manager
        ↓
Actuator FSM Controller
        ↓
Dashboard Monitoring Thread
```

The central server maintains all process state variables and acts as the synchronization hub for all subsystem communication.

---

## Module Descriptions

### Server Core

The server thread functions as the centralized state management engine. It creates a QNX communication channel and handles all incoming subsystem messages using synchronous message passing.

Responsibilities include:

* Maintaining current system state
* Routing incoming messages via dispatcher table
* Replying with updated system data
* Coordinating shutdown behavior

---

### Thermal Sensor Module

The thermal sensor thread simulates real-world thermal plant behavior.

Behavior modeled:

* Gradual temperature rise during normal operation
* Temperature reduction when cooling is active
* Randomized thermal drift for dynamic simulation

---

### AI Prediction Module

The AI module performs adaptive threshold-based predictive maintenance analysis.

Functions:

* Continuously evaluates process temperature
* Dynamically adjusts alarm threshold during stable operation
* Implements hysteresis to reduce alarm oscillation
* Predicts overheating before critical failure

---

### Actuator FSM Module

The cooling actuator is modeled using a finite state machine.

States implemented:

* **IDLE** – Cooling inactive
* **ENGAGING** – Startup/transition delay
* **ACTIVE** – Cooling fully operational

This improves realism by simulating actuator startup delays found in industrial systems.

---

### Dashboard Module

The dashboard thread provides real-time monitoring output.

Displayed parameters:

* Current temperature
* Adaptive threshold
* Voltage
* Fluid density
* Alarm status
* Cooling actuator state

---

## Inter-Process Communication Design

The framework uses **QNX Neutrino synchronous message passing**.

APIs used:

* `ChannelCreate()`
* `ConnectAttach()`
* `MsgSend()`
* `MsgReceive()`
* `MsgReply()`

### Design Rationale

Message passing was selected instead of shared memory because it:

* Eliminates race conditions in shared-state access
* Avoids mutex overhead in control-critical paths
* Provides deterministic blocking semantics
* Aligns with QNX industrial design patterns

---

## Scheduling Strategy

The system uses **priority-based round-robin scheduling (`SCHED_RR`)**.

| Thread         | Priority | Function               |
| -------------- | -------- | ---------------------- |
| Server Core    | 50       | State management / IPC |
| AI Logic       | 45       | Predictive analysis    |
| Actuator FSM   | 40       | Cooling control        |
| Thermal Sensor | 30       | Sensor simulation      |
| Dashboard      | 20       | Visualization          |

This ensures safety-critical control logic preempts non-critical monitoring tasks.

---

## Predictive Maintenance Logic

Adaptive threshold logic is used instead of static fault thresholds.

Features include:

* Dynamic threshold learning
* Threshold adaptation during stable operation
* Alarm hysteresis for chatter reduction
* Early warning before critical thermal runaway

This better reflects industrial predictive maintenance strategies compared to fixed-limit monitoring.

---

## Results and Observations

The implemented framework successfully demonstrates:

* Deterministic communication between all RTOS subsystems
* Adaptive thermal threshold adjustment over runtime
* Realistic cooling FSM transitions
* Stable closed-loop thermal regulation
* Early predictive alarm generation under overheating conditions

---

## Future Enhancements

Potential improvements include:

* Integration with physical ESP32 / industrial sensors
* Persistent fault/event logging
* Web-based dashboard visualization
* Machine learning-based remaining useful life estimation
* CAN/MQTT/OPC-UA communication support
* Multi-node distributed predictive monitoring

---

## Conclusion

The developed predictive maintenance RTOS framework successfully demonstrates a modular industrial-grade monitoring and control architecture on QNX Neutrino RTOS. By combining deterministic native IPC, adaptive predictive logic, FSM-based actuator control, and priority-driven scheduling, the project models the behavior of a real-time industrial predictive maintenance system.

The architecture provides a scalable foundation for future hardware integration and advanced industrial fault prediction research.

---
