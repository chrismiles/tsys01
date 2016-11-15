/******************************************************************************
tsys01spi.cpp
TSYS01 SPI interface to Raspberry Pi
Chris Miles
Nov 13, 2016

The board was connected as follows:
(Raspberry Pi)(TSYS01)
GND  -> GND
3.3V -> Vcc
CE1  -> SS (Shift Select)
SCK  -> SCK 
MOSI -> MOSI
MISO -> MISO

To build this file, I use the command:
>  g++ tsys01spi.cpp -lwiringPi -o tsys01spi

This code is beerware; if you see me at the
local, and you've found my code helpful, please buy me a beer!

Distributed as-is; no warranty is given.
******************************************************************************/

#include <iostream>
#include <errno.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <unistd.h>
#include <inttypes.h> // for uint16_t
#include <math.h>

using namespace std;

// channel is the wiringPi name for the chip select (or chip enable) pin.
// Set this to 0 or 1, depending on how it's connected.
static const int CHANNEL = 1;

// TSYS01 Commands
static const int CMD_RESET			= 0x1E;
static const int CMD_START_ADC_CONVERSION	= 0x48;
static const int CMD_READ_ADC_RESULT		= 0x00;
static const int CMD_READ_PROM_ADDR_0		= 0xA0;
static const int CMD_READ_PROM_ADDR_1		= 0xA2;
static const int CMD_READ_PROM_ADDR_2		= 0xA4;
static const int CMD_READ_PROM_ADDR_3		= 0xA6;
static const int CMD_READ_PROM_ADDR_4		= 0xA8;
static const int CMD_READ_PROM_ADDR_5		= 0xAA;
static const int CMD_READ_PROM_ADDR_6		= 0xAC;
static const int CMD_READ_PROM_ADDR_7		= 0xAE;

static uint16_t k0, k1, k2, k3, k4;

void readCalibrationParameters()
{
   unsigned char buffer[100];

   cout << "Reading Calibration Parameters" << endl;

   buffer[0] = CMD_READ_PROM_ADDR_1;
   buffer[1] = 0;
   buffer[2] = 0;
   wiringPiSPIDataRW(CHANNEL, buffer, 3);
   //cout << " buffer[1] = " << hex << (int)buffer[1] << " buffer[2] = " << hex << (int)buffer[2] << endl;
   k4 = buffer[1] * 256 + buffer[2];

   buffer[0] = CMD_READ_PROM_ADDR_2;
   buffer[1] = 0;
   buffer[2] = 0;
   wiringPiSPIDataRW(CHANNEL, buffer, 3);
   k3 = buffer[1] * 256 + buffer[2];

   buffer[0] = CMD_READ_PROM_ADDR_3;
   buffer[1] = 0;
   buffer[2] = 0;
   wiringPiSPIDataRW(CHANNEL, buffer, 3);
   k2 = buffer[1] * 256 + buffer[2];

   buffer[0] = CMD_READ_PROM_ADDR_4;
   buffer[1] = 0;
   buffer[2] = 0;
   wiringPiSPIDataRW(CHANNEL, buffer, 3);
   k1 = buffer[1] * 256 + buffer[2];

   buffer[0] = CMD_READ_PROM_ADDR_5;
   buffer[1] = 0;
   buffer[2] = 0;
   wiringPiSPIDataRW(CHANNEL, buffer, 3);
   k0 = buffer[1] * 256 + buffer[2];

   cout << " k4 =  " << dec << k4 << endl;
   cout << " k3 =  " << k3 << endl;
   cout << " k2 =  " << k2 << endl;
   cout << " k1 =  " << k1 << endl;
   cout << " k0 =  " << k0 << endl;
}

double readTemperature() {
   unsigned char buffer[100];

   // Start ADC conversion
   //cout << "Starting ADC conversion (0x48)" << endl;
   buffer[0] = CMD_START_ADC_CONVERSION;
   wiringPiSPIDataRW(CHANNEL, buffer, 1);

   delay(10); // milliseconds

   // Read ADC result
   //cout << "Reading ADC result (0x00)" << endl;
   buffer[0] = CMD_READ_ADC_RESULT;
   buffer[1] = 0;
   buffer[2] = 0;
   buffer[3] = 0;
   wiringPiSPIDataRW(CHANNEL, buffer, 4);

   //cout << " Result: " << hex << (int)buffer[1] << (int)buffer[2] << (int)buffer[3] << endl;

   int adc24 = buffer[1] * 65536 + buffer[2] * 256 + buffer[3];
   double adc16 = (float)adc24 / 256;

   //cout << " adc24 = " << dec << adc24 << ", adc16 = " << adc16 << endl;

   double t = (-2.0) * (double)k4 * exp10(-21) * pow(adc16, 4.0) +
                4.0  * (double)k3 * exp10(-16) * pow(adc16, 3.0) +
              (-2.0) * (double)k2 * exp10(-11) * pow(adc16, 2.0) +
                       (double)k1 * exp10(-6)  * adc16 +
              (-1.5) * (double)k0 * exp10(-2);

   return t;
}

void initialize() {
   // Configure the interface.
   // CHANNEL insicates chip select,
   // 500000 indicates bus speed.
   int fd = wiringPiSPISetup(CHANNEL, 500000);

   cout << "Init result: " << fd << endl;
}

void reset() {
   unsigned char buffer[100];

   cout << "Sending reset (0x1E)" << endl;
   buffer[0] = CMD_RESET;
   wiringPiSPIDataRW(CHANNEL, buffer, 1);

   delay(4); // milliseconds
}

int main()
{
   initialize();
   reset();
   readCalibrationParameters();

   do {
      double t = readTemperature();
      cout << " T: " << dec << t << "°C" << endl;
      sleep(1);
   } while(1);
}

