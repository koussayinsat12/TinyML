# TinyML Motion Detection with Edge Impulse and ESP32

## Overview
This project demonstrates **real-time motion classification** on an **ESP32** using a **TinyML model** trained with **Edge Impulse**. It uses an **MMA7760 accelerometer** connected via I²C to collect motion data and runs an **on-device neural network** to recognize different motion types (e.g., still, moving, shaking).

## Features
- Reads 3-axis accelerometer data (X, Y, Z) via I²C  
- Preprocesses and normalizes sensor input  
- Runs Edge Impulse inference locally on the ESP32  
- Displays classification results and anomaly score over Serial  

## Hardware Requirements
- **ESP32 board**  
- **MMA7760 accelerometer**  
- I²C connection pins:  
  - SDA → GPIO 21  
  - SCL → GPIO 22  
  - Power → 3.3V and GND  

## Software Requirements
- Arduino IDE  
- Edge Impulse Arduino library (exported from your project)  
- Wire library (for I²C communication)

## How It Works
1. The ESP32 initializes the MMA7760 sensor over I²C.  
2. It continuously reads acceleration values along X, Y, and Z axes.  
3. Sensor readings are buffered and preprocessed.  
4. The Edge Impulse model (`Motion_detection_inferencing.h`) classifies the current motion pattern.  
5. The results and confidence scores are printed on the Serial Monitor.

## How to Run the Project

### 1. Clone the Repository
```bash
git clone <your-repo-url>
cd TinyML-Motion-Detection
```

### 2. Open the Arduino Sketch
- Open `tinyML.ino` in **Arduino IDE**.

### 3. Connect the Hardware
- ESP32 board via USB.  
- MMA7760 accelerometer wired as follows:
  - SDA → GPIO 21  
  - SCL → GPIO 22  
  - VCC → 3.3V  
  - GND → GND  

### 4. Install Dependencies
- **Edge Impulse Arduino library** (exported from your Edge Impulse project).  
- **Wire library** (for I²C communication).

### 5. Upload the Sketch
1. Select the correct **ESP32 board** under `Tools → Board`.  
2. Select the correct **COM port** under `Tools → Port`.  
3. Click **Upload**.

### 6. View Output
- Open the **Serial Monitor** at `9600 baud`.  
- Move or shake the sensor — you’ll see live motion classification:
```
Starting inferencing in 2 seconds...
Sampling...
Predictions (DSP: 3 ms., Classification: 5 ms., Anomaly: 1 ms.):
    still: 0.02
    moving: 0.94
    shaking: 0.04
    anomaly score: 0.021
```

## Notes
- The project runs fully on-device (no internet connection required).  
- Make sure the model is trained for **accelerometer data**.  
- If you face memory allocation issues, define `-DEI_CLASSIFIER_ALLOCATION_STATIC` in your `boards.local.txt`.