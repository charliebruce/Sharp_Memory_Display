/*********************************************************************
This is a port of the Adafruit Arduino library for Due.

This code is not optimised at all! It's barely more than a proof of concept.

If you need something to run really fast:

 * SPI peripheral and DMA should be used
 * 32-bit accesses should be used where possible
 * Rotation should be done per object, not per pixel
 * Fast horizontal lines should be implemented

It can be made more robust too:

 * Optimise timing, remove hacky volatile delays that depend on clock speed


The original readme follows:


  Pick a SHARP Memory LCD up today in the adafruit shop!
  ------> http://www.adafruit.com/products/1393

These displays use SPI to communicate, 3 pins are required to  
interface

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada for Adafruit Industries.
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution
 *********************************************************************/

#include "Adafruit_SharpMem.h"

/**************************************************************************
    Sharp Memory Display Connector
    -----------------------------------------------------------------------
    Pin   Function        Notes
    ===   ==============  ===============================
      1   VIN             3.3-5.0V (into LDO supply)
      2   3V3             3.3V out
      3   GND
      4   SCLK            Serial Clock
      5   MOSI            Serial Data Input
      6   CS              Serial Chip Select
      9   EXTMODE         COM Inversion Select (Low = SW clock/serial)
      7   EXTCOMIN        External COM Inversion Signal
      8   DISP            Display On(High)/Off(Low)

 **************************************************************************/

#define SHARPMEM_BIT_WRITECMD   (0x80)
#define SHARPMEM_BIT_CLEAR      (0x20)

//Volatile dummy for small delays
volatile uint32_t d = 0;

uint8_t sharpmem_buffer[(SHARPMEM_LCDWIDTH * SHARPMEM_LCDHEIGHT) / 8];

/* ************* */
/* CONSTRUCTORS  */
/* ************* */
Adafruit_SharpMem::Adafruit_SharpMem(uint8_t clk, uint8_t mosi, uint8_t ss, uint8_t ecin) :
												Adafruit_GFX(SHARPMEM_LCDWIDTH, SHARPMEM_LCDHEIGHT) {
	_clk = clk;
	_mosi = mosi;
	_ss = ss;
	_ecin = ecin;

	//Set pin state before direction to make sure they start this way (no glitching)
	digitalWrite(_ss, HIGH);
	digitalWrite(_clk, LOW);
	digitalWrite(_ecin, LOW);
	digitalWrite(_mosi, HIGH);

	pinMode(_ss, OUTPUT);
	pinMode(_clk, OUTPUT);
	pinMode(_mosi, OUTPUT);
	pinMode(_ecin, OUTPUT);

	clkport     = portOutputRegister(digitalPinToPort(_clk));
	clkpinmask  = digitalPinToBitMask(_clk);
	dataport    = portOutputRegister(digitalPinToPort(_mosi));
	datapinmask = digitalPinToBitMask(_mosi);

}

void Adafruit_SharpMem::begin() {
	setRotation(0);
}

/* *************** */
/* PRIVATE METHODS */
/* *************** */

/**************************************************************************/
/*!
    @brief  Sends a single byte in pseudo-SPI.
 */
/**************************************************************************/
void Adafruit_SharpMem::sendbyte(uint8_t data)
{
	uint8_t i = 0;

	//LCD expects LSB first
	for (i=0; i<8; i++)
	{

		d++;d++;d++;

		//Make sure clock starts low
		*clkport &= ~clkpinmask;
		if (data & 0x80)
			*dataport |=  datapinmask;
		else
			*dataport &= ~datapinmask;

		d++;d++;d++;

		//Clock is active high
		*clkport |=  clkpinmask;
		data <<= 1;
	}

	d++;d++;d++;

	//Make sure clock ends low
	*clkport &= ~clkpinmask;
}




void Adafruit_SharpMem::sendbyteLSB(uint8_t data)
{
	uint8_t i = 0;

	//LCD expects LSB first
	for (i=0; i<8; i++)
	{

		d++;d++;d++;

		//Make sure clock starts low
		*clkport &= ~clkpinmask;

		if (data & 0x01)
			*dataport |=  datapinmask;
		else
			*dataport &= ~datapinmask;

		d++;d++;d++;

		//Clock is active high
		*clkport |=  clkpinmask;
		data >>= 1;

	}

	d++;d++;d++;

	//Make sure clock ends low
	*clkport &= ~clkpinmask;

}


/* ************** */
/* PUBLIC METHODS */
/* ************** */


/**************************************************************************/
/*! 
    @brief Write a single pixel to the image buffer

    @param[in]  x
                The x position (0 based)
    @param[in]  y
                The y position (0 based)
	@param[in]	color
				The pixel color to use
 */
/**************************************************************************/
void Adafruit_SharpMem::drawPixel(int16_t x, int16_t y, uint16_t color) 
{

	//Is the point out of range?
	if((x < 0) || (x >= _width) || (y < 0) || (y >= _height)) return;

	switch(rotation) {
	case 1:
		swap(x, y);
		x = WIDTH  - 1 - x;
		break;
	case 2:
		x = WIDTH  - 1 - x;
		y = HEIGHT - 1 - y;
		break;
	case 3:
		swap(x, y);
		y = HEIGHT - 1 - y;
		break;
	}

	//Mirror in x
	x = WIDTH - 1 - x;


	if(!color) {
		//Wipe relevant bits
		sharpmem_buffer[(y*SHARPMEM_LCDWIDTH + x) / 8] &= ~(1 << (x & 0b111));
	}
	else {
		//Set relevant bits
		sharpmem_buffer[(y*SHARPMEM_LCDWIDTH + x) / 8] |= (1 << (x & 0b111));
	}

}

/**************************************************************************/
/*! 
    @brief Gets the value of the specified pixel from the buffer

    @param[in]  x
                The x position (0 based)
    @param[in]  y
                The y position (0 based)

    @return     The pixel's color value, as defined in SHARPMEM_COLORS.
 */
/**************************************************************************/
uint8_t Adafruit_SharpMem::getPixel(uint16_t x, uint16_t y)
{

	while(1)
		;

	//DO NOT USE
}

/**************************************************************************/
/*! 
    @brief Clears the screen
 */
/**************************************************************************/
void Adafruit_SharpMem::clearDisplay() 
{


	clearFB();

	//Activate the device
	digitalWrite(_ss, HIGH);

	//Delay tsSCS
	delayMicroseconds(3);

	//Send the clear screen command rather than doing a HW refresh (quicker)
	//followed by 5 dummy bytes
	sendbyte(SHARPMEM_BIT_CLEAR);

	//Send 8 additional dummy bits
	sendbyte(0x00);

	//Delay thSCS
	delayMicroseconds(1);

	//deactivate the device
	digitalWrite(_ss, LOW);

	//Delay twSCSL
	delayMicroseconds(1);

}

void Adafruit_SharpMem::clearFB(void) {

	//Clear framebuffer black
	memset(sharpmem_buffer, 0, (SHARPMEM_LCDWIDTH * SHARPMEM_LCDHEIGHT) / 8);

}


/*
 * Prevent problems with charge buildup
 * Call on a timer interrupt at the recommended frequency 
 * as given by the datasheet for best contrast.
 */
void Adafruit_SharpMem::toggleEcHw(void) {

	digitalWrite(_ecin, HIGH);
	delayMicroseconds(3); //Min 2uS
	digitalWrite(_ecin, LOW);


}


/**************************************************************************/
/*! 
    @brief Renders the contents of the pixel buffer on the LCD
 */
/**************************************************************************/
void Adafruit_SharpMem::refresh(void) 
{
	uint16_t y, currentLine;
	uint8_t atInLine = 0;
	//Activate the device
	digitalWrite(_ss, HIGH);

	//Delay tsSCS
	delayMicroseconds(3);

	//Send the write command
	sendbyte(SHARPMEM_BIT_WRITECMD);

	//Send the address for line 1
	currentLine = 1;
	sendbyteLSB(currentLine);

	//For each byte
	for (y=0; y < (SHARPMEM_LCDHEIGHT * SHARPMEM_LCDWIDTH)/8; y++)
	{

		sendbyteLSB(sharpmem_buffer[y]);

		atInLine++;
		//If we're not at the end of line, continue to the next byte
		//12 bytes/line
		if(atInLine < (SHARPMEM_LCDWIDTH/8))
			continue;

		currentLine++;
		atInLine = 0;
		//Dummy byte to end a line
		sendbyte(0x00);

		//If there are more lines to follow, send the next line number.
		if (currentLine <= SHARPMEM_LCDHEIGHT)
		{
			sendbyteLSB(currentLine);
		}

	}

	//Send another trailing 8 dummy bits for the final line, making a total of 16 dummy bits at end of frame
	sendbyte(0x00);

	//Delay thSCS
	delayMicroseconds(1);

	//Deactivate the device
	digitalWrite(_ss, LOW);

	//A microsecond (twSCSL)
	delayMicroseconds(1);

}
