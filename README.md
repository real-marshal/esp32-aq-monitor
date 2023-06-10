## Air quality monitor based on LilyGO T-Display-S3 board (ESP32 MCU)

**Before building don't forget to choose the correct display driver in TFT_eSPI/User_Setup_Select.h**

Currently connected sensors:

- SCD30 for CO2, T and RH
- SPS30 for PM1-10
- SFA30 for HCHO (formaldehydes)
- SGP41 for VOC and NOx (relative values only)

About the circuit:

- 10K pull-up resistors are connected to SPS30's SCL and SDA lines
- 2 I2C buses are used because I'm not sure if and how sensors with an internal pull-up resistor can be used with a sensor that requires an external pull-up resistor (SPS30).

| Pin | Usage         |
| --- | ------------- |
| 18  | SDA for SCD30 |
| 17  | SCL for SCD30 |
| 21  | SDA for SPS30 |
| 16  | SCL for SPS30 |
