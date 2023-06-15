#pragma once

// T, RH, CO2
void scd30Init();
void scd30Measure();

// PM, NC
void sps30Init();
void sps30Measure();

// HCHO
void sfa30Init();
void sfa30Measure();

// VOC, NOx
void sgp41Init();
void sgp41Conditioning();
void sgp41Measure();
void sgp41SaveState();
