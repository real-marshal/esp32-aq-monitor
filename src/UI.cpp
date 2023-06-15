#include <UI.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <common.h>

constexpr HealthinessRange co2Range = {1000, 1500, 2000};
constexpr HealthinessRange hchoRange = {30, 100, 300};
constexpr HealthinessRange pm10Range = {15, 30, 60};
constexpr HealthinessRange pm25Range = {15, 30, 60};
constexpr HealthinessRange pm40Range = {20, 40, 80};
constexpr HealthinessRange pm100Range = {30, 60, 120};
constexpr HealthinessRange vocRange = {150, 300, 450};
constexpr HealthinessRange noxRange = {50, 150, 250};

constexpr auto SECOND_COLUMN_OFFSET = TFT_HEIGHT / 3 * 1.2;
constexpr auto THIRD_COLUMN_OFFSET = TFT_HEIGHT / 3 * 2.2;
constexpr auto NC_COLUMN_OFFSET = TFT_HEIGHT / 3 * 1.5;

template struct Measurement<float>;
template struct Measurement<int32_t>;

template void writeMeasurement<float>(const Measurement<float> &measurement, int extraSpaces);
template void writeMeasurement<int32_t>(const Measurement<int32_t> &measurement, int extraSpaces);

TFT_eSPI tft = TFT_eSPI();

template <typename T>
Measurement<T>::Measurement(String label, T value, String unit, const HealthinessRange *range)
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

uint16_t getValueColor(int value, const HealthinessRange *range)
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
void writeMeasurement(const Measurement<T> &measurement, int extraSpaces)
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

void initDisplay()
{
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
}

void renderUI()
{
  int16_t cursorY;

  tft.setTextSize(1);
  tft.setTextFont(2);

  tft.setCursor(0, 0);

  writeMeasurement(Measurement<float>("CO2", mData.co2Concentration, "ppm", &co2Range));
  writeMeasurement(Measurement<float>("HCHO", mData.hcho, "ppb", &hchoRange));

  tft.setCursor(SECOND_COLUMN_OFFSET, 0);

  writeMeasurement(Measurement<float>("T", mData.temperature, "C"));

  cursorY = tft.getCursorY();
  tft.setCursor(SECOND_COLUMN_OFFSET, cursorY);

  writeMeasurement(Measurement<int32_t>("VOC", mData.vocIndex, "", &vocRange));

  tft.println();

  tft.setCursor(THIRD_COLUMN_OFFSET, 0);

  writeMeasurement(Measurement<float>("RH", mData.humidity, "%"));

  cursorY = tft.getCursorY();
  tft.setCursor(THIRD_COLUMN_OFFSET, cursorY);

  writeMeasurement(Measurement<int32_t>("NOx", mData.noxIndex, "", &noxRange));

  tft.println();

  cursorY = tft.getCursorY();
  tft.setCursor(NC_COLUMN_OFFSET, cursorY);

  writeMeasurement(Measurement<float>("NC 0.5", mData.sps30_m.nc_0p5, "#/cm3"), 1);

  int16_t cursorYPM = tft.getCursorY();
  tft.setCursor(NC_COLUMN_OFFSET, cursorYPM);

  writeMeasurement(Measurement<float>("NC 1.0", mData.sps30_m.nc_1p0, "#/cm3"), 1);

  cursorY = tft.getCursorY();
  tft.setCursor(NC_COLUMN_OFFSET, cursorY);

  writeMeasurement(Measurement<float>("NC 2.5", mData.sps30_m.nc_2p5, "#/cm3"), 1);

  cursorY = tft.getCursorY();
  tft.setCursor(NC_COLUMN_OFFSET, cursorY);

  writeMeasurement(Measurement<float>("NC 4.0", mData.sps30_m.nc_4p0, "#/cm3"), 1);

  cursorY = tft.getCursorY();
  tft.setCursor(NC_COLUMN_OFFSET, cursorY);

  writeMeasurement(Measurement<float>("NC 10.0", mData.sps30_m.nc_10p0, "#/cm3"));

  tft.setCursor(0, cursorYPM);

  writeMeasurement(Measurement<float>("PM 1.0", mData.sps30_m.mc_1p0, "mcg/m3", &pm10Range), 1);

  cursorY = tft.getCursorY();
  tft.setCursor(0, cursorY);

  writeMeasurement(Measurement<float>("PM 2.5", mData.sps30_m.mc_2p5, "mcg/m3", &pm25Range), 1);

  cursorY = tft.getCursorY();
  tft.setCursor(0, cursorY);

  writeMeasurement(Measurement<float>("PM 4.0", mData.sps30_m.mc_4p0, "mcg/m3", &pm40Range), 1);

  cursorY = tft.getCursorY();
  tft.setCursor(0, cursorY);

  writeMeasurement(Measurement<float>("PM 10.0", mData.sps30_m.mc_10p0, "mcg/m3", &pm100Range));

  writeMeasurement(Measurement<float>("Avg particle diameter", mData.sps30_m.typical_particle_size, "nm"));
}
