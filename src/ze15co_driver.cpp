#include <ze15co_driver.h>

constexpr auto MESSAGE_SIZE = 9;
constexpr uint8_t READ_COMMAND[MESSAGE_SIZE] = {0xFF, 0x01, 0x86, 0x00, 0x00,
                                                0x00, 0x00, 0x00, 0x79};

ZE15CO::ZE15CO() {}

void ZE15CO::begin(HardwareSerial& serial) {
  this->serial = &serial;
}

ze15coError ZE15CO::readCO(float& value) {
  static uint8_t response[MESSAGE_SIZE];

  size_t bytesWritten = serial->write(READ_COMMAND, MESSAGE_SIZE);

  if (!bytesWritten) {
    return ze15coError::WRITE_ERROR;
  }

  serial->flush();

  if (serial->available() < MESSAGE_SIZE) {
    return ze15coError::NOT_AVAILABLE;
  }

  serial->readBytes(response, MESSAGE_SIZE);

  if (!verifyChecksum(response)) {
    return ze15coError::CHECKSUM_MISMATCH;
  }

  if (response[2] >> 7) {
    return ze15coError::SENSOR_FAILURE;
  }

  value = ((response[2] & ((1 << 5) - 1)) * 256 + response[3]) * 0.1;

  return NOT_ERROR;
}

bool ZE15CO::verifyChecksum(uint8_t message[]) {
  uint8_t sum = 0;

  // sum all bytes except first and last
  for (int i = 1; i < MESSAGE_SIZE - 1; i++) {
    sum += message[i];
  }

  return (~sum + 1 + 256) == message[MESSAGE_SIZE - 1];
}