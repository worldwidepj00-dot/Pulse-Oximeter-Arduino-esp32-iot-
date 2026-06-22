# Pulse Oximeter – Arduino Uno Prototype and ESP32 IoT Upgrade

This repository contains a pulse oximeter project that measures **heart rate (BPM)** and **SpO₂ (blood oxygen saturation)** using a MAX3010x sensor.  
The project is developed in two stages:

- **Stage 1 **: Arduino Uno + I2C LCD display – local measurement and display.  
- **Stage 2 **: ESP32-based IoT version with web dashboard and optional cloud integration.

## 1. Features

### Arduino Uno + LCD (implemented)

- Measures **heart rate** and **SpO₂** using a MAX30100/MAX30102 pulse oximeter sensor.
- Uses an **I2C 16x2 LCD** to show:
  - Heart rate in BPM with icons and LOW / HI / OK indication.
  - SpO₂ percentage with OK / warning icons.
- Collects **20 samples** and:
  - Sorts the values.
  - Trims the lowest 2 and highest 2 readings.
  - Calculates using the beer-lambert law for more stable results.
- Shows measurement progress on the LCD (`Sample X/20`).
- Prints final averaged results to the Serial Monitor an lcd for debugging.

### ESP32 IoT Web Dashboard (planned)

> **Note:** The ESP32 version will be added in the next update.

Planned features:

- Replace Arduino Uno with **ESP32** for Wi‑Fi and IoT capability.
- Host a **web dashboard** directly from the ESP32:
  - Show a table of recent heart rate and SpO₂ readings.
  - Display status (e.g., OK / warning) for each sample.
- Optionally send data to a **cloud platform** (ThingSpeak / Firebase, etc.) for remote monitoring.
- Later enhancement: add **charts** (e.g., using Chart.js) to visualize trends over time.

---

## 2. Project structure

pulse-oximeter-arduino-esp32-iot/
├── arduino_uno_lcd/
│   └── arduino_uno_pulse_oximeter_lcd.ino   # Arduino Uno + LCD implementation
├── esp32_iot_web/
│   └── (to be added)                        # ESP32 + IoT web dashboard implementation
├── images/
│   └── (screenshots and hardware photos)
└── README.md
```

---

## 3. Hardware and software
### Hardware used (Arduino Uno version)

- Arduino Uno
- MAX30100 / MAX30102 pulse oximeter sensor module
- 16x2 I2C LCD display
- Jumper wires and breadboard
- USB cable for programming and Serial Monitor

### Software

- Arduino IDE
- C/C++
- Required Arduino libraries:
  - `Wire.h`
  - `LiquidCrystal_I2C.h`
  - `MAX30100_PulseOximeter.h`
- Serial Monitor for debugging and viewing final values

---

## 4. Arduino Uno + LCD: How it works

1. The MAX3010x sensor is initialized and starts sampling IR/Red signals.
2. Heart rate (BPM) and SpO₂ values are obtained using the pulse oximeter library.
3. The code waits until the signal is valid (ignores unrealistic or unstable values).
4. Every few hundred milliseconds, a new sample is stored in a buffer.
5. After collecting 20 valid samples:
   - Both heart rate and SpO₂ arrays are **sorted**.
   - The first 2 and last 2 values are **discarded** to remove outliers.
   - The remaining 16 values are averaged to compute stable BPM and SpO₂.
6. The final averaged values are displayed on the LCD:
   - Heart rate with heart icon and LOW / HI / OK indicator.
   - SpO₂ with oxygen icon and OK / warning indicator.
7. The same final values are sent to the Serial Monitor.

This approach improves stability compared to using a single raw reading.

---

## 5. Wiring (Arduino Uno + LCD)

> This is a general outline. Pin numbers can be adjusted in the code.

- MAX3010x sensor:
  - VCC → 3.3V 
  - GND → GND
  - SDA → A4 (I2C SDA on Arduino Uno)
  - SCL → A5 (I2C SCL on Arduino Uno)

- I2C LCD (16x2):
  - VCC → 5V
  - GND → GND
  - SDA → A4 (shared I2C bus)
  - SCL → A5 (shared I2C bus)

Check your LCD I2C address (often `0x27` or `0x3F`) and update in the code if needed.

---

## 6. How to run the Arduino Uno version

1. Install **Arduino IDE**.
2. Install the required libraries:
   - `LiquidCrystal_I2C`
   - `MAX30100_PulseOximeter`
3. Open the file:

   - `arduino_uno_lcd/arduino_uno_pulse_oximeter_lcd.ino`

4. Select the correct board and port:
   - Board: `Arduino Uno`
   - Port: your Arduino COM port
5. Upload the sketch to the board.
6. Place your finger properly on the sensor.
7. Watch the LCD for:
   - Sampling progress (`Sample X/20`)
   - Final heart rate and SpO₂ readings.
8. Open the Serial Monitor at 9600 baud to see the final averaged values.

---

## 7. Upcoming ESP32 IoT implementation

The next step of this project is to migrate the sensor reading logic to an **ESP32** and add an IoT layer:

- Use ESP32 to read heart rate and SpO₂ from the MAX3010x sensor.
- Start a Wi‑Fi access point or connect to existing Wi‑Fi.
- Host a simple web server on the ESP32:
  - Show recent readings in an HTML table.
  - Include timestamps and status indicators.
- Optionally integrate with:
  - ThingSpeak / Firebase / other cloud services for remote data logging.
  - Chart.js or similar library to plot HR and SpO₂ graphs in the browser.

The ESP32 `.ino` file and documentation will be added in the `esp32_iot_web/` folder in a future commit.

---

## 8. Future improvements

- Add data logging to SD card or external storage.
- Implement basic alert logic (e.g., buzzer or LED on abnormal values).
- Build an enclosure for better finger placement and measurement stability.
- Add more advanced visualizations and authentication on the IoT dashboard.

---

## 9. Author

- Developer: *Priyanshu Jha*  
- GitHub: [https://github.com/worldwidepj00-dot]

Feel free to fork, modify, and build upon this project for learning or prototyping.
