## Air quality monitor using ESP32-S3 (with LilyGO T-Display-S3 devboard)

**Before building don't forget to choose the correct display driver in TFT_eSPI/User_Setup_Select.h**

Currently connected sensors:

- SCD30 for CO2, T and RH
- SPS30 for PM1-10
- SFA30 for HCHO (formaldehydes)
- SGP41 for VOC and NOx (relative values only)
- ZE15CO for CO
- VEML6075 for UV data

Note that the parameters for VOX algorithm are supposed to have age of only 10 mins, so the device shouldn't be powered off for longer than that period (or you need to add a battery powered RTC like DS3231).

About the circuit:

- SCD30 has 45k Ohm internal pull-up resistor, SGP41 - 10k, SFA30 - none, this gives about ~8k in the end for the whole I2C bus which is perfectly fine
- 10K pull-up resistors are connected to SPS30's SCL and SDA lines
- 2 I2C buses are used because I was not sure if and how sensors with an internal pull-up resistor can be used with a sensor that requires an external pull-up resistor (SPS30). But now I think I'll add it to the first bus soon and get rid of the external resistors
