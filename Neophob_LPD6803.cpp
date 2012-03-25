/*********************************************************************************/
// Example to control LPD6803-based RGB LED Modules in a strand or strip via SPI
// by Michael Vogt / http://pixelinvaders.ch
// This Library is basically a copy and paste work and relies on work 
// of Adafruit-WS2801-Library and FastSPI Library 
/*********************************************************************************/

#include <TimerOne.h>
#include "SPI.h"
#include "Neophob_LPD6803.h"

//some local variables, ised in isr
static uint8_t isDirty;
static uint16_t prettyUglyCopyOfNumPixels;
static uint16_t *pData;
static uint16_t *pDataStart;
volatile unsigned char nState=1;

// Constructor for use with hardware SPI (specific clock/data pins):
Neophob_LPD6803::Neophob_LPD6803(uint16_t n) {
  prettyUglyCopyOfNumPixels = n;  
  numLEDs = n;  
  pixels = (uint16_t *)malloc(numLEDs);
  pDataStart = pixels;
  isDirty = 0;    
  cpumax = 70;
  
  //clear buffer
  for (int i=0; i < numLEDs; i++) {
    setPixelColor(i,0);
  }
}
/*the SPI data register (SPDR) holds the byte which is about to be shifted out the MOSI line */
#define SPI_LOAD_BYTE(data) SPDR=data
/* Wait until last bytes is transmitted. */
#define SPI_WAIT_TILL_TRANSMITED while(!(SPSR & (1<<SPIF)))


//Interrupt routine.
//Frequency was set in setup(). Called once for every bit of data sent
//In your code, set global Sendmode to 0 to re-send the data to the pixels
//Otherwise it will just send clocks.
static void isr() {
  static uint16_t indx=0;
  
  if(nState==1) {//send clock and check for color update (isDirty)
    //SPI_A(0); //maybe, move at the end of the startSPI() method
    if (isDirty==1) { //must we update the pixel value
      nState = 0;
      isDirty = 0;
      SPI_LOAD_BYTE(0);
      SPI_WAIT_TILL_TRANSMITED; 
      indx = 0;
      pData = pDataStart; //reset index
      return;
    }
    SPI_LOAD_BYTE(0);
    SPI_WAIT_TILL_TRANSMITED;
    return;
  }
  else {                        //feed out pixelbuffer    
    register uint16_t command;
    command = *(pData++);       //get current pixel
    SPI_LOAD_BYTE( (command>>8) & 0xFF);
    SPI_WAIT_TILL_TRANSMITED;                      //send 8bits
    
    SPI_LOAD_BYTE( command & 0xFF);
    SPI_WAIT_TILL_TRANSMITED;                      //send 8bits again
    
    indx++;                     //are we done?
    if(indx >= prettyUglyCopyOfNumPixels) { 
      nState = 1;
    }

    return;
  } 
}


// Activate hard/soft SPI as appropriate:
void Neophob_LPD6803::begin(void) {
  startSPI();

  setCPUmax(cpumax);
  Timer1.attachInterrupt(isr);
}

void Neophob_LPD6803::setCPUmax(uint8_t max) {
  cpumax = max;

  // each clock out takes 20 microseconds max
  long time = 100;
  time *= 20;   // 20 microseconds per
  time /= max;    // how long between timers
  Timer1.initialize(time);
}


// Enable SPI hardware and set up protocol details:
void Neophob_LPD6803::startSPI(void) {
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
//  SPI.setClockDivider(SPI_CLOCK_DIV8);  // 2 MHz
 SPI.setClockDivider(SPI_CLOCK_DIV16);  // 1 MHz  
// SPI.setClockDivider(SPI_CLOCK_DIV32);  // 0.5 MHz  
//  SPI.setClockDivider(SPI_CLOCK_DIV64);  // 0.25 MHz  
  // LPD6803 can handle a data/PWM clock of up to 25 MHz, and 50 Ohm
  // resistors on SPI lines for impedance matching.  In practice and
  // at short distances, 2 MHz seemed to work reliably enough without
  // resistors, and 4 MHz was possible with a 220 Ohm resistor on the
  // SPI clock line only.  Your mileage may vary.  Experiment!
  
  //SPI_A(0); //maybe, move at the end of the startSPI() method
}

uint16_t Neophob_LPD6803::numPixels(void) {
  return numLEDs;
}


void Neophob_LPD6803::show(void) {
  isDirty=1; //flag to trigger redraw
}


void Neophob_LPD6803::setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
  if (n > numLEDs) return;
  
    /* As a modest alternative to full double-buffering, the setPixel()
	   function blocks until the serial output interrupt has moved past
	   the pixel being modified here.  If the animation-rendering loop
	   functions in reverse (high to low pixel index), then the two can
	   operate together relatively efficiently with only minimal blocking
	   and no second pixel buffer required. */
	while(nState==0); 

  uint16_t data = g & 0x1F;
  data <<= 5;
  data |= b & 0x1F;
  data <<= 5;
  data |= r & 0x1F;
  data |= 0x8000;

  pixels[n] = data;
}

//---
void Neophob_LPD6803::setPixelColor(uint16_t n, uint16_t c) {
  if (n > numLEDs) return;

    /* As a modest alternative to full double-buffering, the setPixel()
     function blocks until the serial output interrupt has moved past
	   the pixel being modified here.  If the animation-rendering loop
	   functions in reverse (high to low pixel index), then the two can
	   operate together relatively efficiently with only minimal blocking
	   and no second pixel buffer required. */
	while(nState==0); 

  pixels[n] = 0x8000 | c;
}


