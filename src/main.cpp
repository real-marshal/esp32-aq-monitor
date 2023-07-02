#include <Arduino.h>
#include <TaskScheduler.h>
#include <UI.h>
#include <Wire.h>
#include <common.h>
#include <constants.h>
#include <measurements.h>

// TODO: integrate LVGL and refactor UI

Scheduler scheduler;

Task scd30MeasureTask(100,
                      TASK_FOREVER,
                      &Measurements::scd30Measure,
                      &scheduler,
                      true);
Task sps30MeasureTask(100,
                      TASK_FOREVER,
                      &Measurements::sps30Measure,
                      &scheduler,
                      true);
Task sfa30MeasureTask(100,
                      TASK_FOREVER,
                      &Measurements::sfa30Measure,
                      &scheduler,
                      true);
Task sgp41ConditioningTask(1000,
                           SGP41_CONDITIONING_SECS,
                           &Measurements::sgp41Conditioning,
                           &scheduler,
                           true);
Task sgp41MeasureTask(SGP41_SAMPLING_INTERVAL_SECS * 1000,
                      TASK_FOREVER,
                      &Measurements::sgp41Measure,
                      &scheduler);
Task sgp41SaveStateTask(SGP41_VOC_SAVE_STATE_INTERVAL_MS,
                        TASK_FOREVER,
                        &Measurements::sgp41SaveState,
                        &scheduler);
Task veml6075MeasureTask(400,
                         TASK_FOREVER,
                         &Measurements::veml6075Measure,
                         &scheduler,
                         true);
Task ze15coMeasureTask(1000,
                       TASK_FOREVER,
                       &Measurements::ze15coMeasure,
                       &scheduler,
                       true);
Task renderUITask(100, TASK_FOREVER, &UI::renderUI, &scheduler, true);

void setup() {
  Serial.begin(115200);

  preferences.begin(PREFERENCES_NAMESPACE);

  // This bus is used for SCD30, SFA30 and SGP41, 18 SDA, 17 SCL (default ESP32
  // I2C pins)
  Wire.begin();

  // This bus is used for SPS30, 21 SDA, 16 SCL
  Wire1.begin(21, 16);

  Measurements::scd30Init();

  Measurements::sfa30Init();

  Measurements::sgp41Init();

  Measurements::sps30Init();

  Measurements::veml6075Init();

  Measurements::ze15coInit();

  UI::initDisplay();

  sgp41ConditioningTask.setOnDisable([]() { sgp41MeasureTask.enable(); });

  sgp41SaveStateTask.enableDelayed();
}

void loop() {
  scheduler.execute();
}