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
- [ ] ADC (MCP3561): CS line connected to bit 1 of expander, needs to be tested
- [ ] SD Card: CS line connected to bit 2 of expander, needs to be tested

#### HSPI
- [ ] Display: Adapter needs to be done

#### I2C
- [X] IMU (BHI160): Works, firmware is running 
- [X] Pulsoximeter (MAX30102)
- [X] IO Extender (PCF8574): Works, needs to be connected to CS lines
- [X] Touchscreen controller (TSC2003): SDA and SCL were swapped... fixed with handwire!