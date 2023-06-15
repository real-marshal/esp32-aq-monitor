#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <sps30.h>

struct MeasurementData
{
  float co2Concentration = 0.0;
  float temperature = 0.0;
  float humidity = 0.0;

  sps30_measurement sps30_m;

  float hcho = 0.0;

  int32_t vocIndex = 0;
  int32_t noxIndex = 0;
};

extern MeasurementData mData;
extern Preferences preferences;
