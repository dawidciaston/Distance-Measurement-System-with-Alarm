# Distance Measurement System with Alarm

## Overview
This project is implemented on the **KL05Z** development board and features an ultrasonic distance measurement system with an alarm functionality. The system measures distances using a time-of-flight method and provides real-time feedback via an LCD display. Additionally, it includes an audible warning system that activates when an object is detected within a predefined threshold distance.

## Features
- Ultrasonic distance measurement
- LCD1602 display for real-time distance updates
- Adjustable measurement unit (meters, centimeters, millimeters, inches)
- Configurable distance threshold using a capacitive touch slider
- Audible warning (beep) when the detected distance is below the threshold
- Button-based unit selection
- Efficient delay management with SysTick timer

## Hardware Requirements
- **NXP FRDM-KL05Z** development board
- **LCD1602** display (I2C interface)
- **Ultrasonic Sensor** (e.g., HC-SR04)
- **Buzzer** for sound alerts
- **Capacitive Touch Slider** (TSI module)
- **Push buttons** for user input

## Software Components
- **MKL05Z4.h** – Core microcontroller definitions
- **frdm_bsp.h** – Board-specific support package
- **lcd1602.h** – LCD driver for display updates
- **TPM.h** – Timer/PWM module configuration
- **DAC.h** – Digital-to-Analog Converter for sound generation
- **klaw.h** – Button handling module
- **tsi.h** – Touch slider input processing
- Standard C libraries: `<math.h>`, `<stdio.h>`, `<string.h>`, `<stdlib.h>`

## System Architecture
### 1. Distance Measurement
- The **TPM1 Timer** captures the time delay between the trigger pulse and echo reception.
- The time difference is used to compute the distance based on the speed of sound.
- The computed distance is displayed on the **LCD1602** in the selected unit.

### 2. Unit Selection
- The user can select the measurement unit using **push buttons (S1-S4)**:
  - **S1** – Meters
  - **S2** – Centimeters
  - **S3** – Millimeters
  - **S4** – Inches

### 3. Audible Warning System
- A **DAC-generated sine wave** is used to drive a buzzer when the measured distance is below the set threshold.
- The **SysTick timer** updates the DAC output to create a beep sound.

### 4. Touch Slider for Threshold Adjustment
- The **TSI module** reads user input from the capacitive touch slider.
- The threshold distance can be adjusted dynamically.

## Code Structure
### Main Loop (`main.c`)
- Initializes LCD, DAC, timers, buttons, and touch slider.
- Enters an infinite loop where it:
  - Processes user input (buttons & slider)
  - Captures ultrasonic sensor data
  - Updates the LCD display
  - Controls the buzzer based on distance threshold
  - Manages interrupts and delays

### Interrupt Service Routines
- **SysTick_Handler** – Handles millisecond delays and sound generation.
- **TPM1_IRQHandler** – Captures echo signal timing and calculates distance.
- **PORTA_IRQHandler** – Handles button presses for unit selection.

## Usage
1. Power on the **FRDM-KL05Z** board.
2. The system initializes and prompts for unit selection.
3. Use **S1-S4 buttons** to select the preferred measurement unit.
4. The measured distance is displayed in real time.
5. Adjust the alarm threshold using the **touch slider**.
6. If the measured distance is below the threshold, the buzzer sounds an alarm.

---
**Author:** Dawid Ciaston
**Platform:** FRDM-KL05Z  
**Language:** C


