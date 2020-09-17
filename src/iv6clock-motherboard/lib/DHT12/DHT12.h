#ifndef DHT12_H
#define DHT12_H

#include <Wire.h>
#include <Arduino.h>

#define DHT12_OK              (int8_t)0
#define DHT12_ERROR_CHECKSUM  (int8_t)-10
#define DHT12_ERROR_CONNECT   (int8_t)-11
#define DHT12_MISSING_BYTES   (int8_t)-12

class DHT12 {
  public:
#ifdef ESP8266
    /*
      Init Sensor
    */
    void begin(uint8_t sda, uint8_t scl);
#endif
    /*
      Init Sensor
    */
    void begin();
    /*
      Read raw data
    */
    int8_t read();
    /*
      Get Humidity
    */
    float getHumidity();
    /*
      Get Temperature
    */
    float getTemperature();

  private:
    /*
      Negative is error in communication
      5 is OK
      0..4 is too few bytes.
    */
    int readSensor();

  private:
    float humidity;
    float temperature;
    uint8_t bits[5];
};

#endif
