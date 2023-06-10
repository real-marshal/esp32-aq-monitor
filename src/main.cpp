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

// TODO: refactor this awful code & add color ranges for values

#define SPS30_AUTO_CLEAN_DAYS 4
// Never increase this value unless you want to burn the sensor
#define SGP41_CONDITIONING_SECS 10
#define SGP41_SAMPLING_INTERVAL 0.1f

TFT_eSPI tft = TFT_eSPI();

SensirionI2cScd30 scd30;
SensirionI2CSfa3x sfa3x;
SensirionI2CSgp41 sgp41;

VOCGasIndexAlgorithm voc_algorithm(SGP41_SAMPLING_INTERVAL);
NOxGasIndexAlgorithm nox_algorithm;

static char errorMessage[256];
static int16_t error;

void setup()
{
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(0);

  Serial.begin(115200);

  // This bus is used for SCD30, SFA30 and SGP41, 18 SDA, 17 SCL (default I2C pins)
  Wire.begin();
  scd30.begin(Wire, SCD30_I2C_ADDR_61);

  // No idea if these are needed, but they were in the example so...
  scd30.stopPeriodicMeasurement();
  scd30.softReset();

  // Temp offset from my personal observations
  error = scd30.setTemperatureOffset(150);
  if (error != NO_ERROR)
  {
    Serial.print("Error trying to set temperature offset: ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
    return;
  }
  // First tests show no need in this function (plus it has some painful requirements)
  error = scd30.activateAutoCalibration(0);
  if (error != NO_ERROR)
  {
    Serial.print("Error trying to (dis-)activate ASC: ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
    return;
  }

  error = scd30.startPeriodicMeasurement(0);
  if (error != NO_ERROR)
  {
    Serial.print("Error trying to execute startPeriodicMeasurement(): ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
    return;
  }

  sfa3x.begin(Wire);

  error = sfa3x.startContinuousMeasurement();
  if (error)
  {
    Serial.print("Error trying to execute startContinuousMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  sgp41.begin(Wire);

  uint16_t testResult;
  error = sgp41.executeSelfTest(testResult);
  if (error)
  {
    Serial.print("Error trying to execute executeSelfTest(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }
  else if (testResult != 0xD400)
  {
    Serial.print("executeSelfTest failed with error: ");
    Serial.println(testResult);
  }

  // This bus is used for SPS30, 21 SDA, 16 SCL
  Wire1.begin(21, 16);

  while (sps30_probe() != 0)
  {
    Serial.println("SPS30 probing failed");
    delay(500);
  }

  error = sps30_set_fan_auto_cleaning_interval_days(SPS30_AUTO_CLEAN_DAYS);
  if (error)
  {
    Serial.print("SPS30 error setting the auto-clean interval: ");
    Serial.println(error);
  }

  error = sps30_start_measurement();
  if (error < 0)
  {
    Serial.println("SPS30 error starting measurement");
  }
}

float co2Concentration = 0.0;
float temperature = 0.0;
float humidity = 0.0;

int16_t hcho;
int16_t humidity_sfa;
int16_t temperature_sfa;

uint16_t defaultRh = 0x8000;
uint16_t defaultT = 0x6666;
uint16_t srawVoc = 0;
uint16_t srawNox = 0;
int32_t voc_index = 0;
int32_t nox_index = 0;

struct sps30_measurement sps30_m;

ulong initMillis = 0;

void loop()
{
  uint16_t dataReady = 0;

  if (initMillis == 0)
  {
    initMillis = millis();
  }

  // SCD30
  error = scd30.getDataReady(dataReady);
  if (error != NO_ERROR)
  {
    Serial.print("SCD30 data readiness check error: ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
  }
  else if (dataReady == 1)
  {
    error = scd30.readMeasurementData(co2Concentration, temperature, humidity);

    if (error != NO_ERROR)
    {
      Serial.print("SCD30 reading measurement data error: ");
      errorToString(error, errorMessage, sizeof errorMessage);
      Serial.println(errorMessage);
    }
  }

  // SPS30
  error = sps30_read_data_ready(&dataReady);
  if (dataReady < 0)
  {
    Serial.print("SPS30 data readiness check error: ");
    Serial.println(error);
  }
  else if (dataReady == 1)
  {
    error = sps30_read_measurement(&sps30_m);
    if (error < 0)
    {
      Serial.println("SPS30 reading measurement data error: ");
    }
  }

  // SFA30
  error = sfa3x.readMeasuredValues(hcho, humidity_sfa, temperature_sfa);
  if (error)
  {
    Serial.print("SFA30 reading measurement data error: ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  // SGP41
  defaultRh = uint16_t(humidity) * 65535 / 100;
  defaultT = (uint16_t(temperature) + 45) * 65535 / 175;

  if (millis() - initMillis < SGP41_CONDITIONING_SECS * 1000)
  {
    error = sgp41.executeConditioning(defaultRh, defaultT, srawVoc);
  }
  else
  {
    error = sgp41.measureRawSignals(defaultRh, defaultT, srawVoc, srawNox);
  }

  if (error)
  {
    Serial.print("SGP41 error: ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }
  else
  {
    voc_index = voc_algorithm.process(srawVoc);
    nox_index = nox_algorithm.process(srawNox);
  }

  tft.setCursor(0, 0, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextFont(2);

  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.print("CO2: ");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print(co2Concentration);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.print("ppm    T: ");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print(temperature);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.print("deg    RH: ");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print(humidity);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.println("%");

  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.print("HCHO: ");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print(hcho / 5.0f);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.print("ppb   VOC: ");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print(voc_index);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.print("u   NOx: ");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print(nox_index);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.println("u");

  tft.println();

  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.print("                          NC 0.5:  ");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print(sps30_m.nc_0p5);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.println("#/cm3");

  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.print("PM 1.0:  ");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print(sps30_m.mc_1p0);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.print("mcg/m3");
  tft.print("  NC 1.0:  ");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print(sps30_m.nc_1p0);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.println("#/cm3");

  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.print("PM 2.5:  ");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print(sps30_m.mc_2p5);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.print("mcg/m3");
  tft.print("  NC 2.5:  ");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print(sps30_m.nc_2p5);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.println("#/cm3");

  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.print("PM 4.0:  ");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print(sps30_m.mc_4p0);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.print("mcg/m3");
  tft.print("  NC 4.0:  ");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print(sps30_m.nc_4p0);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.println("#/cm3");

  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.print("PM 10.0: ");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print(sps30_m.mc_10p0);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.print("mcg/m3");
  tft.print("  NC 10.0: ");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print(sps30_m.nc_10p0);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.println("#/cm3");

  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.print("Avg particle diameter: ");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print(sps30_m.typical_particle_size);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.println("nm");

  // All sensors require a delay before checking their readiness status
  delay(100);
}