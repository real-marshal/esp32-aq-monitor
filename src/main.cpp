#include <Arduino.h>
#include <SensirionI2cScd30.h>
#include <Wire.h>
#include <sps30.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <SensirionI2CSfa3x.h>
#include <SensirionI2CSgp41.h>
#include <VOCGasIndexAlgorithm.h>
#include <NOxGasIndexAlgorithm.h>
#include <UI.h>
#include <Preferences.h>

// TODO: refactor using task scheduler

constexpr auto PREFERENCES_NAMESPACE = "default";

constexpr auto SPS30_AUTO_CLEAN_DAYS = 4;
// Never increase this value unless you want to burn the sensor
constexpr auto SGP41_CONDITIONING_SECS = 10;
constexpr auto SGP41_SAMPLING_INTERVAL = 0.1f;
constexpr auto SGP41_VOC_SAVE_STATE_INTERVAL_MS = 10 * 60 * 1000;
constexpr auto SGP41_VOC_SAVE_STATE_BEGIN_MS = 3 * 60 * 60 * 1000;
constexpr auto SGP41_VOC_SAVED_STATE_AGE_MS = 10 * 60 * 1000;
// These have a length limit of 15 chars, same with namespace
constexpr auto SGP41_VOC_STATE1_KEY = "voc_state1";
constexpr auto SGP41_VOC_STATE2_KEY = "voc_state2";
constexpr auto SGP41_VOC_STATE_TIMESTAMP_KEY = "voc_state_ts";

constexpr auto SECOND_COLUMN_OFFSET = TFT_HEIGHT / 3 * 1.2;
constexpr auto THIRD_COLUMN_OFFSET = TFT_HEIGHT / 3 * 2.2;
constexpr auto NC_COLUMN_OFFSET = TFT_HEIGHT / 3 * 1.5;

constexpr HealthinessRange co2Range = {1000, 1500, 2000};
constexpr HealthinessRange hchoRange = {30, 100, 300};
constexpr HealthinessRange pm10Range = {25, 55, 85};
constexpr HealthinessRange pm25Range = {30, 60, 90};
constexpr HealthinessRange pm40Range = {35, 65, 95};
constexpr HealthinessRange pm100Range = {50, 100, 150};
constexpr HealthinessRange vocRange = {150, 300, 450};
constexpr HealthinessRange noxRange = {50, 150, 250};

TFT_eSPI tft = TFT_eSPI();
Preferences preferences;

SensirionI2cScd30 scd30;
SensirionI2CSfa3x sfa3x;
SensirionI2CSgp41 sgp41;

VOCGasIndexAlgorithm voc_algorithm(SGP41_SAMPLING_INTERVAL);
NOxGasIndexAlgorithm nox_algorithm;

void printSensirionError(int16_t error, String message)
{
  char errorMessage[256];

  if (!error)
    return;

  errorToString(error, errorMessage, 256);

  Serial.print(message + ": ");
  Serial.println(errorMessage);
}

void setup()
{
  Serial.begin(115200);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  preferences.begin(PREFERENCES_NAMESPACE);

  int16_t error;

  // This bus is used for SCD30, SFA30 and SGP41, 18 SDA, 17 SCL (default ESP32 I2C pins)
  Wire.begin();

  // SCD30
  scd30.begin(Wire, SCD30_I2C_ADDR_61);

  // Temp offset from my personal observations
  error = scd30.setTemperatureOffset(150);
  printSensirionError(error, "SCD30 error trying to set temperature offset");

  error = scd30.startPeriodicMeasurement(0);
  printSensirionError(error, "SCD30 error trying to start measurements");

  // SFA30
  sfa3x.begin(Wire);

  error = sfa3x.startContinuousMeasurement();
  printSensirionError(error, "SFA30 error trying to start measurements");

  // SGP41
  sgp41.begin(Wire);

  uint16_t testResult;
  error = sgp41.executeSelfTest(testResult);
  printSensirionError(error, "SGP41 error trying to execute self-test");

  if (testResult != 0xD400)
  {
    Serial.print("SGP41 self-test failed with error: ");
    Serial.println(testResult);
  }

  if (millis() - preferences.getULong(SGP41_VOC_STATE_TIMESTAMP_KEY) < SGP41_VOC_SAVED_STATE_AGE_MS)
  {
    const float state1 = preferences.getFloat(SGP41_VOC_STATE1_KEY);
    const float state2 = preferences.getFloat(SGP41_VOC_STATE2_KEY);

    if (state1 != 0 && state2 != 0 && !isnan(state1) && !isnan(state2))
    {
      voc_algorithm.set_states(state1, state2);

      Serial.println("Restored VOC state from flash memory: ");
      Serial.println("State 1: " + String(state1));
      Serial.println("State 2: " + String(state2));
    }
  }

  // This bus is used for SPS30, 21 SDA, 16 SCL
  Wire1.begin(21, 16);

  while (sps30_probe() != 0)
  {
    Serial.println("SPS30 probing failed");
    delay(500);
  }

  error = sps30_set_fan_auto_cleaning_interval_days(SPS30_AUTO_CLEAN_DAYS);
  printSensirionError(error, "SPS30 error setting the auto-clean interval");

  error = sps30_start_measurement();
  printSensirionError(error, "SPS30 error starting measurements");
}

void loop()
{
  int16_t error;
  uint16_t dataReady = 0;

  static ulong initMillis = 0;
  static ulong vocSaveMillis = 0;

  if (initMillis == 0)
  {
    initMillis = millis();
  }

  if (vocSaveMillis == 0)
  {
    vocSaveMillis = millis();
  }

  // SCD30
  static float co2Concentration = 0.0;
  static float temperature = 0.0;
  static float humidity = 0.0;

  error = scd30.getDataReady(dataReady);
  printSensirionError(error, "SCD30 data readiness check error");

  if (dataReady == 1)
  {
    error = scd30.readMeasurementData(co2Concentration, temperature, humidity);
    printSensirionError(error, "SCD30 reading measurement data error");
  }

  // SPS30
  static sps30_measurement sps30_m;

  error = sps30_read_data_ready(&dataReady);
  printSensirionError(error, "SPS30 data readiness check error");

  if (dataReady == 1)
  {
    error = sps30_read_measurement(&sps30_m);
    printSensirionError(error, "SPS30 reading measurement data error");
  }

  // SFA30
  static int16_t hcho = 0;
  static int16_t humidity_sfa = 0;
  static int16_t temperature_sfa = 0;

  error = sfa3x.readMeasuredValues(hcho, humidity_sfa, temperature_sfa);
  printSensirionError(error, "SFA30 reading measurement data error");

  // SGP41
  static uint16_t srawVoc = 0;
  static uint16_t srawNox = 0;
  static uint16_t rhCompensation = uint16_t(humidity) * 65535 / 100;
  static uint16_t tCompensation = (uint16_t(temperature) + 45) * 65535 / 175;
  static int32_t vocIndex = 0;
  static int32_t noxIndex = 0;

  if (millis() - initMillis < SGP41_CONDITIONING_SECS * 1000)
  {
    error = sgp41.executeConditioning(rhCompensation, tCompensation, srawVoc);
    printSensirionError(error, "SGP41 conditioning error");
  }
  else
  {
    error = sgp41.measureRawSignals(rhCompensation, tCompensation, srawVoc, srawNox);
    printSensirionError(error, "SGP41 reading measurement data error");

    if (!error)
    {
      vocIndex = voc_algorithm.process(srawVoc);
      noxIndex = nox_algorithm.process(srawNox);
    }
  }

  if (millis() - initMillis >= SGP41_VOC_SAVE_STATE_BEGIN_MS && millis() - vocSaveMillis > SGP41_VOC_SAVE_STATE_INTERVAL_MS)
  {
    float state1, state2;
    size_t ret = 1;

    voc_algorithm.get_states(state1, state2);

    ret &= preferences.putFloat(SGP41_VOC_STATE1_KEY, state1);
    ret &= preferences.putFloat(SGP41_VOC_STATE2_KEY, state2);
    ret &= preferences.putULong(SGP41_VOC_STATE_TIMESTAMP_KEY, millis());

    Serial.println(ret == 0 ? "VOC state save failed" : "VOC state saved");

    vocSaveMillis = millis();
  }

  // UI rendering

  int16_t cursorY;

  tft.setTextSize(1);
  tft.setTextFont(2);

  tft.setCursor(0, 0);

  writeMeasurement(tft, Measurement<float>("CO2", co2Concentration, "ppm", &co2Range));
  writeMeasurement(tft, Measurement<float>("HCHO", hcho / 5.0f, "ppb", &hchoRange));

  tft.setCursor(SECOND_COLUMN_OFFSET, 0);

  writeMeasurement(tft, Measurement<float>("T", temperature, "C"));

  cursorY = tft.getCursorY();
  tft.setCursor(SECOND_COLUMN_OFFSET, cursorY);

  writeMeasurement(tft, Measurement<int32_t>("VOC", vocIndex, "", &vocRange));

  tft.println();

  tft.setCursor(THIRD_COLUMN_OFFSET, 0);

  writeMeasurement(tft, Measurement<float>("RH", humidity, "%"));

  cursorY = tft.getCursorY();
  tft.setCursor(THIRD_COLUMN_OFFSET, cursorY);

  writeMeasurement(tft, Measurement<int32_t>("NOx", noxIndex, "", &noxRange));

  tft.println();

  cursorY = tft.getCursorY();
  tft.setCursor(NC_COLUMN_OFFSET, cursorY);

  writeMeasurement(tft, Measurement<float>("NC 0.5", sps30_m.nc_0p5, "#/cm3"), 1);

  int16_t cursorYPM = tft.getCursorY();
  tft.setCursor(NC_COLUMN_OFFSET, cursorYPM);

  writeMeasurement(tft, Measurement<float>("NC 1.0", sps30_m.nc_1p0, "#/cm3"), 1);

  cursorY = tft.getCursorY();
  tft.setCursor(NC_COLUMN_OFFSET, cursorY);

  writeMeasurement(tft, Measurement<float>("NC 2.5", sps30_m.nc_2p5, "#/cm3"), 1);

  cursorY = tft.getCursorY();
  tft.setCursor(NC_COLUMN_OFFSET, cursorY);

  writeMeasurement(tft, Measurement<float>("NC 4.0", sps30_m.nc_4p0, "#/cm3"), 1);

  cursorY = tft.getCursorY();
  tft.setCursor(NC_COLUMN_OFFSET, cursorY);

  writeMeasurement(tft, Measurement<float>("NC 10.0", sps30_m.nc_10p0, "#/cm3"));

  tft.setCursor(0, cursorYPM);

  writeMeasurement(tft, Measurement<float>("PM 1.0", sps30_m.mc_1p0, "mcg/m3", &pm10Range), 1);

  cursorY = tft.getCursorY();
  tft.setCursor(0, cursorY);

  writeMeasurement(tft, Measurement<float>("PM 2.5", sps30_m.mc_2p5, "mcg/m3", &pm25Range), 1);

  cursorY = tft.getCursorY();
  tft.setCursor(0, cursorY);

  writeMeasurement(tft, Measurement<float>("PM 4.0", sps30_m.mc_4p0, "mcg/m3", &pm40Range), 1);

  cursorY = tft.getCursorY();
  tft.setCursor(0, cursorY);

  writeMeasurement(tft, Measurement<float>("PM 10.0", sps30_m.mc_10p0, "mcg/m3", &pm100Range));

  writeMeasurement(tft, Measurement<float>("Avg particle diameter", sps30_m.typical_particle_size, "nm"));

  // All sensors require a delay before checking their readiness status
  delay(100);
}