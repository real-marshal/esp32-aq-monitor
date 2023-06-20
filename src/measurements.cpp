#include <measurements.h>
#include <common.h>
#include <SensirionI2cScd30.h>
#include <SensirionI2CSfa3x.h>
#include <SensirionI2CSgp41.h>
#include <VOCGasIndexAlgorithm.h>
#include <NOxGasIndexAlgorithm.h>
#include <constants.h>

SensirionI2cScd30 scd30;
SensirionI2CSfa3x sfa3x;
SensirionI2CSgp41 sgp41;

VOCGasIndexAlgorithm voc_algorithm(SGP41_SAMPLING_INTERVAL_SECS);
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

void scd30Init()
{
  int16_t error;

  scd30.begin(Wire, SCD30_I2C_ADDR_61);

  // Temp offset from my personal observations
  error = scd30.setTemperatureOffset(200);
  printSensirionError(error, "SCD30 error trying to set temperature offset");

  error = scd30.startPeriodicMeasurement(0);
  printSensirionError(error, "SCD30 error trying to start measurements");
}

void scd30Measure()
{
  int16_t error;
  uint16_t dataReady = 0;

  error = scd30.getDataReady(dataReady);
  printSensirionError(error, "SCD30 data readiness check error");

  if (dataReady == 1)
  {
    error = scd30.readMeasurementData(mData.co2Concentration, mData.temperature, mData.humidity);
    printSensirionError(error, "SCD30 reading measurement data error");
  }
}

void sps30Init()
{
  int16_t error;

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

void sps30Measure()
{
  int16_t error;
  uint16_t dataReady = 0;

  error = sps30_read_data_ready(&dataReady);
  printSensirionError(error, "SPS30 data readiness check error");

  if (dataReady == 1)
  {
    error = sps30_read_measurement(&mData.sps30_m);
    printSensirionError(error, "SPS30 reading measurement data error");
  }
}

void sfa30Init()
{
  int16_t error;
  sfa3x.begin(Wire);

  error = sfa3x.startContinuousMeasurement();
  printSensirionError(error, "SFA30 error trying to start measurements");
}

void sfa30Measure()
{
  int16_t error;
  static int16_t hcho = 0;
  static int16_t humidity_sfa = 0;
  static int16_t temperature_sfa = 0;

  error = sfa3x.readMeasuredValues(hcho, humidity_sfa, temperature_sfa);
  printSensirionError(error, "SFA30 reading measurement data error");

  mData.hcho = hcho / 5.0f;
}

void sgp41Init()
{
  int16_t error;

  sgp41.begin(Wire);

  uint16_t testResult;
  error = sgp41.executeSelfTest(testResult);
  printSensirionError(error, "SGP41 error trying to execute self-test");

  if (testResult != 0xD400)
  {
    Serial.print("SGP41 self-test failed with error: ");
    Serial.println(testResult);
  }

  const float state1 = preferences.getFloat(SGP41_VOC_STATE1_KEY);
  const float state2 = preferences.getFloat(SGP41_VOC_STATE2_KEY);

  if (!isnan(state1) && !isnan(state2))
  {
    voc_algorithm.set_states(state1, state2);

    Serial.println("Restored VOC state from flash memory: ");
    Serial.println("State 1: " + String(state1));
    Serial.println("State 2: " + String(state2));
  }
}

void sgp41Conditioning()
{
  int16_t error;
  uint16_t srawVoc = 0;

  uint16_t rhCompensation = uint16_t(mData.humidity) * 65535 / 100;
  uint16_t tCompensation = (uint16_t(mData.temperature) + 45) * 65535 / 175;

  error = sgp41.executeConditioning(rhCompensation, tCompensation, srawVoc);
  printSensirionError(error, "SGP41 conditioning error");
}

void sgp41Measure()
{
  int16_t error;

  static uint16_t srawVoc = 0;
  static uint16_t srawNox = 0;
  static uint16_t rhCompensation = uint16_t(mData.humidity) * 65535 / 100;
  static uint16_t tCompensation = (uint16_t(mData.temperature) + 45) * 65535 / 175;

  error = sgp41.measureRawSignals(rhCompensation, tCompensation, srawVoc, srawNox);
  printSensirionError(error, "SGP41 reading measurement data error");

  if (!error)
  {
    mData.vocIndex = voc_algorithm.process(srawVoc);
    mData.noxIndex = nox_algorithm.process(srawNox);
  }
}

void sgp41SaveState()
{
  float state1, state2;

  voc_algorithm.get_states(state1, state2);

  Serial.println(
      preferences.putFloat(SGP41_VOC_STATE1_KEY, state1) && preferences.putFloat(SGP41_VOC_STATE2_KEY, state2)
          ? "VOC state saved"
          : "VOC state save failed");
}
