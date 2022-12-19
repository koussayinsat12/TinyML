/* Edge Impulse ingestion SDK
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

/* Includes ---------------------------------------------------------------- */

#include <Wire.h>
#include <Motion_detection_inferencing.h>
// I2C ADDRESS
#define MMA_ADDRESS   0x4C

//HTS Registers Addresses
#define MMA_MODE_REG       0x07
#define MMA_OUTX_REG       0x00
#define MMA_OUTY_REG       0x01
#define MMA_OUTZ_REG       0x02


//I2C0 PINS
#define SDA_Pin 21
#define SCL_Pin 22
#define MAX_ACCEPTED_RANGE  2.0f  
TwoWire ESP32Wemo_I2C = TwoWire(0);// starting 03/2022, models are generated setting range to +-2, but this example use Arudino library which set range to +-4g. If you are using an older model, ignore this value and use 4.0f instead

/*
 ** NOTE: If you run into TFLite arena allocation issue.
 **
 ** This may be due to may dynamic memory fragmentation.
 ** Try defining "-DEI_CLASSIFIER_ALLOCATION_STATIC" in boards.local.txt (create
 ** if it doesn't exist) and copy this file to
 ** `+`.
 **
 ** See
 ** (https://support.arduino.cc/hc/en-us/articles/360012076960-Where-are-the-installed-cores-located-)
 ** to find where Arduino installs cores on your machine.
 **
 ** If the problem persists then there's not enough memory for this model and application.
 */

/* Private variables ------------------------------------------------------- */
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal

/**
* @brief      Arduino setup function
*/
uint8_t readI2cReg(uint8_t RegAddr){    
          ESP32Wemo_I2C.beginTransmission(MMA_ADDRESS);
          ESP32Wemo_I2C.write(RegAddr);
           if (ESP32Wemo_I2C.endTransmission(false)){ //if !=0
                Serial.println ("Problem writing without stop"); 
                exit(0);
            }
        ESP32Wemo_I2C.requestFrom(MMA_ADDRESS,0x01);
        return(ESP32Wemo_I2C.read());
   }
   void writeI2cReg(uint8_t RegAddr, uint8_t Value){
    
          ESP32Wemo_I2C.beginTransmission(MMA_ADDRESS);
          ESP32Wemo_I2C.write(RegAddr);
          ESP32Wemo_I2C.write(Value);
          if (ESP32Wemo_I2C.endTransmission(true)!=0){
            Serial.println ("problem writing to I2C device");
            exit(0);
          }
   }
void setup()
{    
    // put your setup code here, to run once:
    Serial.begin(9600);
    
     ESP32Wemo_I2C.begin(SDA_Pin, SCL_Pin, 400000);
      
     // enable MMA7760
         writeI2cReg(MMA_MODE_REG, 0x01);
    // comment out the below line to cancel the wait for USB connection (needed for native USB)
    while (!Serial);
    Serial.println("Edge Impulse Inferencing Demo");

    if (EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME != 3) {
        ei_printf("ERR: EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME should be equal to 3 (the 3 sensor axes)\n");
        return;
    }
}

/**
 * @brief Return the sign of the number
 * 
 * @param number 
 * @return int 1 if positive (or 0) -1 if negative
 */
float ei_get_sign(float number) {
    return (number >= 0.0) ? 1.0 : -1.0;
}

/**
* @brief      Get data and run inferencing
*
* @param[in]  debug  Get debug info if true
*/
void loop()
{
int8_t outX = (readI2cReg(MMA_OUTX_REG) <<2) ;
 float accelX = ((outX/4)*1.5/32);
 int8_t outY = (readI2cReg(MMA_OUTY_REG)<<2)  ;
 float accelY = ((outY/4)*1.5/32);
 int8_t outZ = (readI2cReg(MMA_OUTZ_REG)<<2) ;
  float accelZ = ((outZ/4)*1.5/32);
  Serial.println(accelX);
    ei_printf("\nStarting inferencing in 2 seconds...\n");

    delay(2000);

    ei_printf("Sampling...\n");

    // Allocate a buffer here for the values we'll read from the IMU
    float buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE] = { 0 };

    for (size_t ix = 0; ix < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; ix += 3) {
        // Determine the next tick (and then sleep later)
        uint64_t next_tick = micros() + (EI_CLASSIFIER_INTERVAL_MS * 1000);

        buffer[ix + 0]= accelX;
        buffer[ix + 1] = accelY;
        buffer[ix + 2] = accelZ ;

        for (int i = 0; i < 3; i++) {
            if (fabs(buffer[ix + i]) > MAX_ACCEPTED_RANGE) {
                buffer[ix + i] = ei_get_sign(buffer[ix + i]) * MAX_ACCEPTED_RANGE;
            }
        }


        delayMicroseconds(next_tick - micros());
    }

    // Turn the raw buffer in a signal which we can the classify
    signal_t signal;
    int err = numpy::signal_from_buffer(buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
    if (err != 0) {
        ei_printf("Failed to create signal from buffer (%d)\n", err);
        return;
    }

    // Run the classifier
    ei_impulse_result_t result = { 0 };

    err = run_classifier(&signal, &result, debug_nn);
    if (err != EI_IMPULSE_OK) {
        ei_printf("ERR: Failed to run classifier (%d)\n", err);
        return;
    }

    // print the predictions
    ei_printf("Predictions ");
    ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)",
        result.timing.dsp, result.timing.classification, result.timing.anomaly);
    ei_printf(": \n");
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        ei_printf("    %s: %.5f\n", result.classification[ix].label, result.classification[ix].value);
    }
#if EI_CLASSIFIER_HAS_ANOMALY == 1
    ei_printf("    anomaly score: %.3f\n", result.anomaly);
#endif
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_ACCELEROMETER
#error "Invalid model for current sensor"
#endif
