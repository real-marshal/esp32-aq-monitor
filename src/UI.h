#include <Arduino.h>
#include <TFT_eSPI.h>

template <typename T>
struct Measurement
{
  Measurement(String label, T value, String unit);
  String label;
  T value;
  String unit;
};

template <typename T>
void writeMeasurement(TFT_eSPI &tft, const struct Measurement<T> &measurement, int extraSpaces = 0);