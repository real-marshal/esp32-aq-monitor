#include <Arduino.h>
#include <TFT_eSPI.h>

// RGB565 colors
enum ValueColor
{
  COLOR_GOOD = 0xFFFF,
  COLOR_DECENT = 0xFF4E,
  COLOR_BAD = 0xFE2E,
  COLOR_AWFUL = 0xFC8E
};

struct HealthinessRange
{
  int good;
  int decent;
  int bad;
};

template <typename T>
struct Measurement
{
  Measurement(String label, T value, String unit);
  Measurement(String label, T value, String unit, const HealthinessRange *range);
  String label;
  T value;
  String unit;
  const HealthinessRange *range;
};

uint16_t getValueColor(int value, HealthinessRange *range);

template <typename T>
void writeMeasurement(TFT_eSPI &tft, const struct Measurement<T> &measurement, int extraSpaces = 0);