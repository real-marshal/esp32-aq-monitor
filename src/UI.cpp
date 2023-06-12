#include <UI.h>

template <typename T>
Measurement<T>::Measurement(String label, T value, String unit, HealthinessRange *range)
{
  this->label = label;
  this->value = value;
  this->unit = unit;
  this->range = range;
}

template <typename T>
Measurement<T>::Measurement(String label, T value, String unit)
{
  this->label = label;
  this->value = value;
  this->unit = unit;
  this->range = NULL;
}

uint16_t getValueColor(int value, HealthinessRange *range)
{
  if (range == NULL || value < range->good)
    return COLOR_GOOD;
  if (value < range->decent)
    return COLOR_DECENT;
  if (value < range->bad)
    return COLOR_BAD;

  return COLOR_AWFUL;
}

template <typename T>
void writeMeasurement(TFT_eSPI &tft, const Measurement<T> &measurement, int extraSpaces)
{
  String labelSuffix = ": ";

  for (int i = 0; i < extraSpaces; i++)
  {
    labelSuffix += " ";
  }

  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.print(measurement.label + labelSuffix);
  tft.setTextColor(getValueColor(round(measurement.value), measurement.range), TFT_BLACK);
  tft.setTextPadding(tft.textWidth("9999", 2));
  tft.print(measurement.value);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setTextPadding(tft.textWidth(measurement.unit + " ", 2));
  tft.println(measurement.unit + " ");
}

template struct Measurement<float>;
template struct Measurement<int32_t>;

template void writeMeasurement<float>(TFT_eSPI &tft, const Measurement<float> &measurement, int extraSpaces);
template void writeMeasurement<int32_t>(TFT_eSPI &tft, const Measurement<int32_t> &measurement, int extraSpaces);
