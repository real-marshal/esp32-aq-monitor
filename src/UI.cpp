#include <UI.h>

template <typename T>
Measurement<T>::Measurement(String label, T value, String unit)
{
  this->label = label;
  this->value = value;
  this->unit = unit;
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
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print(measurement.value);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.println(measurement.unit);
}

template struct Measurement<float>;
template struct Measurement<int8_t>;

template void writeMeasurement<float>(TFT_eSPI &tft, const Measurement<float> &measurement, int extraSpaces);
template void writeMeasurement<int8_t>(TFT_eSPI &tft, const Measurement<int8_t> &measurement, int extraSpaces);
