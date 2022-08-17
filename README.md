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
- [ ] ADC (MCP3561): Needs new SPI CS line because Pins of ESP are not connected, waiting for IO Extender
- [ ] SD Card: Needs new SPI CS line because Pins of ESP are not connected, waiting for IO Extender

#### HSPI
- [ ] Display: Adapter needs to be done

#### I2C
- [ ] IMU (BHI160): Firmware flash is needed
- [X] Pulsoximeter (MAX30102)
- [X] IO Extender (PCF8574)
- [ ] Touchscreen controller (TSC2003): Defective IC? Wrong Address?