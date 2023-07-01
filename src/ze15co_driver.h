#pragma once

#include <Arduino.h>

enum ze15coError : uint8_t {
  NOT_ERROR,
  CHECKSUM_MISMATCH,
  SENSOR_FAILURE,
  NOT_AVAILABLE
};

class ZE15CO {
 public:
  ZE15CO();

  void begin(HardwareSerial& serial);

  ze15coError readCO(float& value);

 private:
  HardwareSerial* serial = nullptr;

  bool verifyChecksum(uint8_t message[]);
};