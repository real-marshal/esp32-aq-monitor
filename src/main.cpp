#include <Arduino.h>
#include <Wire.h>
#include <UI.h>
#include <common.h>
#include <measurements.h>
#include <TaskScheduler.h>
#include <constants.h>

// TODO: integrate LVGL and refactor UI

Scheduler scheduler;

Task scd30MeasureTask(100, TASK_FOREVER, &scd30Measure, &scheduler, true);
Task sps30MeasureTask(100, TASK_FOREVER, &sps30Measure, &scheduler, true);
Task sfa30MeasureTask(100, TASK_FOREVER, &sfa30Measure, &scheduler, true);
Task sgp41ConditioningTask(1000, SGP41_CONDITIONING_SECS, &sgp41Conditioning, &scheduler, true);
Task sgp41MeasureTask(SGP41_SAMPLING_INTERVAL_SECS * 1000, TASK_FOREVER, &sgp41Measure, &scheduler);
Task sgp41SaveStateTask(SGP41_VOC_SAVE_STATE_INTERVAL_MS, TASK_FOREVER, &sgp41SaveState, &scheduler);
Task renderUITask(100, TASK_FOREVER, &renderUI, &scheduler, true);

void setup()
{
  Serial.begin(115200);

  preferences.begin(PREFERENCES_NAMESPACE);

  // This bus is used for SCD30, SFA30 and SGP41, 18 SDA, 17 SCL (default ESP32 I2C pins)
  Wire.begin();

  // This bus is used for SPS30, 21 SDA, 16 SCL
  Wire1.begin(21, 16);

  scd30Init();

  sfa30Init();

  sgp41Init();

  sps30Init();

  initDisplay();

  sgp41ConditioningTask.setOnDisable([]()
                                     { sgp41MeasureTask.enable(); });

  sgp41SaveStateTask.enableDelayed();
}

void loop()
{
  scheduler.execute();
}