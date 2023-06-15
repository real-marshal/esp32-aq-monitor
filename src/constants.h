#pragma once

constexpr auto PREFERENCES_NAMESPACE = "default";

constexpr auto SPS30_AUTO_CLEAN_DAYS = 4;

// Never increase this value unless you want to burn the sensor
constexpr auto SGP41_CONDITIONING_SECS = 10;
constexpr auto SGP41_SAMPLING_INTERVAL_SECS = 1;
constexpr auto SGP41_VOC_SAVE_STATE_INTERVAL_MS = 10 * 60 * 1000;
constexpr auto SGP41_VOC_SAVED_STATE_AGE_MS = 10 * 60 * 1000;
// These have a length limit of 15 chars, same with namespace
constexpr auto SGP41_VOC_STATE1_KEY = "voc_state1";
constexpr auto SGP41_VOC_STATE2_KEY = "voc_state2";