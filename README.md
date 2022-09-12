# eduSignalFW
firmware for edu signal

## TODO
Project will not build with the latest ESP-IDF.
ASIO_CONCEPT is broken!
> Workaround: Remove `CMakesLists.txt` from `esp-idf/components/asio/`

### Initializing Board
Ticks in the Boxes mean that the device is connected to the bus and communication is established.
There has not been further implementation of functions. Just simple communication (e.g. check ID of device)!

#### VSPI
- [X] EKG (ADS1299)
- [X] ADC (MCP3561): Working. Display backlight and Display reset must be connected to IO Expander
- [ ] SD Card: CS line connected to ESP, Power supply for SD card needs to be checked

#### HSPI
- [ ] Display: Adapter needs to be done

#### I2C
- [X] IMU (BHI160): Works, firmware is running 
- [X] Pulsoximeter (MAX30102)
- [X] IO Extender (PCF8574): Works, output lines need pull-up
- [X] Touchscreen controller (TSC2003): SDA and SCL were swapped... fixed with handwire!