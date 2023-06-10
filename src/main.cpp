#include <Arduino.h>
#include <SensirionI2cScd30.h>
#include <Wire.h>
#include <sps30.h>
#include <TFT_eSPI.h>
#include <SPI.h>

// TODO: refactor this awful code & add color ranges for values

TFT_eSPI tft = TFT_eSPI();

SensirionI2cScd30 scd30;

static char errorMessage[128];
static int16_t error;

uint8_t auto_clean_days = 4;
uint32_t auto_clean;

void setup()
{
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(0);

  Serial.begin(115200);

  // This bus is used for SCD30, 18 SDA, 17 SCL (default I2C pins)
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
  // Want to try it out
  error = scd30.activateAutoCalibration(1);
  if (error != NO_ERROR)
  {
    Serial.print("Error trying to activate ASC: ");
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

  // This bus is used for SPS30, 21 SDA, 16 SCL
  Wire1.begin(21, 16);

  while (sps30_probe() != 0)
  {
    Serial.println("SPS30 probing failed");
    delay(500);
  }

  error = sps30_set_fan_auto_cleaning_interval_days(auto_clean_days);
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

struct sps30_measurement sps30_m;

void loop()
{
  uint16_t dataReady = 0;

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

  tft.println();

  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.print("                        NC 0.5:  ");
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
  tft.print(" NC 10.0: ");
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