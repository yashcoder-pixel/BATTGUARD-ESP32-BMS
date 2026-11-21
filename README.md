
# BATTGUARD – ESP32-Based Battery Management System Simulation

BATTGUARD is a simulation-based project exploring the working principles of a Battery Management System (BMS) using an ESP32 microcontroller and the Wokwi simulation environment.

The project focuses on battery parameter monitoring, abnormal condition detection, fault-handling logic, State of Charge (SOC) estimation, and software-based cell balancing selection.

## Project Overview

A Battery Management System is an important part of battery-powered applications, particularly electric vehicles and energy storage systems.

It monitors battery operating conditions and helps identify potentially unsafe situations such as overvoltage, undervoltage, excessive current, and high temperature.

BATTGUARD explores these concepts through an interactive ESP32 simulation.

## Simulation Platform

- Microcontroller: ESP32 DevKit V1
- Development Framework: Arduino Framework
- Programming Language: Embedded C/C++
- Simulation Environment: Wokwi
- Display: 128x64 SSD1306 OLED
- Display Communication: I2C
- OLED Libraries: Adafruit GFX and Adafruit SSD1306

## Features and Concepts Explored

- Monitoring of three simulated battery-cell voltages
- Battery pack voltage calculation
- Cell voltage imbalance calculation
- Simulated current monitoring
- Temperature monitoring using an NTC thermistor model
- Overvoltage detection
- Undervoltage detection
- Overcurrent detection
- Overtemperature detection
- Fault latching and manual reset logic
- Coulomb-counting-based SOC estimation
- Software-based cell balancing selection
- OLED-based monitoring and diagnostic interface
- Serial monitoring for debugging

## System Inputs and Outputs

### Inputs

- Three potentiometers for simulated cell voltages
- One potentiometer for simulated current
- NTC thermistor input for temperature monitoring
- Push button for fault injection
- Push button for fault reset

### Outputs

- OLED monitoring display
- Normal status LED
- Fault indication LED
- Three cell-balancing indication LEDs
- Contactor status indication LED
- Serial diagnostic output

## Pin Configuration

| Component | ESP32 GPIO |
|---|---|
| Cell 1 voltage | GPIO 34 |
| Cell 2 voltage | GPIO 35 |
| Cell 3 voltage | GPIO 32 |
| Current input | GPIO 33 |
| NTC input | GPIO 36 |
| OLED SDA | GPIO 21 |
| OLED SCL | GPIO 22 |
| Status LED | GPIO 18 |
| Fault LED | GPIO 19 |
| Balancing LED 1 | GPIO 23 |
| Balancing LED 2 | GPIO 25 |
| Balancing LED 3 | GPIO 26 |
| Contactor LED | GPIO 27 |
| Fault injection button | GPIO 16 |
| Reset button | GPIO 17 |

## Fault Conditions Explored

| Condition | Simulation Threshold |
|---|---|
| Cell overvoltage | Above 4.20 V |
| Cell undervoltage | Below 3.00 V |
| Overcurrent | Above 5 A |
| Overtemperature | Above 60°C |
| Sensor fault | Invalid NTC ADC reading |

These are prototype simulation thresholds, not validated battery safety limits.

## Cell Balancing Concept

The simulation identifies the highest-voltage cell when the configured voltage and imbalance conditions are met.

The corresponding LED indicates which cell has been selected by the software logic.

This is a software-based balancing selection demonstration. It does not perform physical energy transfer or actual cell balancing.

## State of Charge Estimation

The project explores a simplified coulomb-counting approach to estimate battery State of Charge.

The estimation uses simulated current measurements and an assumed battery capacity.

The displayed SOC is an educational estimate and is not calibrated against a real battery.

## Simulation

**Wokwi Simulation:**  
https://wokwi.com/projects/481311884893197313

## Project Screenshots


## Learning Outcomes

Through this project, I am exploring:

- ESP32 GPIO and ADC interfacing
- I2C communication with an OLED display
- Embedded C/C++ programming
- Sensor input simulation
- Fault detection and handling logic
- State of Charge estimation concepts
- Debugging through serial output
- Basic battery management system architecture

## Limitations

This project is currently a simulation-based learning prototype.

- Battery voltage and current inputs are simulated using potentiometers.
- Temperature sensing is represented through a simulated NTC input.
- Cell balancing is represented by software selection and LEDs.
- The contactor is represented by an indicator LED.
- No real battery pack or high-current power circuitry is connected.
- The SOC estimation is simplified and has not been validated against real battery measurements.

This simulation must not be used as a real battery protection system.

## Future Scope

- Integration of actual voltage sensing circuits
- Current measurement using a suitable current sensor
- Real temperature sensor integration
- Hardware-based cell balancing
- Physical protection circuitry
- Battery data logging
- Cloud-based monitoring
- Testing with a physical battery monitoring setup

## Author

**Yash Doke**

GitHub: https://github.com/yashcoder-pixel

LinkedIn: https://www.linkedin.com/in/yash-doke369
