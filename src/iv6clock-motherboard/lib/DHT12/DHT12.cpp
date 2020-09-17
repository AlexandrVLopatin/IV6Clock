#include <DHT12.h>

#define DHT12_ADDRESS  ((uint8_t)0x5C)

#ifdef ESP8266
void DHT12::begin(uint8_t sda, uint8_t scl) {
  Wire.begin(sda, scl);
}
#endif

void DHT12::begin() {
  Wire.begin();
}

int8_t DHT12::read() {
  /*
    READ SENSOR
  */
  int status = readSensor();
  if (status < 0) {
    return status;
  }
  /*
    CONVERT AND STORE
  */
  humidity = bits[0] + bits[1] * 0.1;
  temperature = (bits[2] & 0x7F) + bits[3] * 0.1;
  if (bits[2] & 0x80) {
    temperature = -temperature;
  }

  /*
    TEST CHECKSUM
  */
  uint8_t checksum = bits[0] + bits[1] + bits[2] + bits[3];
  if (bits[4] != checksum) {
    return DHT12_ERROR_CHECKSUM;
  }

  return DHT12_OK;
}

int DHT12::readSensor() {
  /*
    GET CONNECTION
  */
  Wire.beginTransmission(DHT12_ADDRESS);
  Wire.write(0);
  int rv = Wire.endTransmission();
  if (rv < 0) {
    return rv;
  }

  /*
    GET DATA
  */
  int bytes = Wire.requestFrom(DHT12_ADDRESS, (uint8_t)5);
  if (bytes == 0) {
    return DHT12_ERROR_CONNECT;
  }
  if (bytes < (uint8_t)5) {
    return DHT12_MISSING_BYTES;
  }
  for (int i = 0; i < bytes; i++)   {
    bits[i] = Wire.read();
  }
  return bytes;
}

float DHT12::getHumidity() {
  return humidity;
}

float DHT12::getTemperature() {
  return temperature;
}