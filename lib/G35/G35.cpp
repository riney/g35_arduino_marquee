#include "G35.h"
#include <pins_arduino.h>

/* g35 shash */
struct rgb_value {
  byte r;
  byte g;
  byte b;
};

struct bulb {
  byte address;
  int pin;
  rgb_value color;
  byte i;
};

/*  Dimensions of the matrix (in bulbs) */
const int matrixWidth = 18;
const int matrixHeight = 10;

/*  The number of strands used to form the matrix. */
const int strandCount = 5;

/*  The number of bulbs on each strand. */
const int strandLength = 36;

/* The digital pins assigned to the strand data lines. */
const int strandPin[strandCount] = { 22, 24, 26, 28, 30 };

/*
    SPECIAL NOTE - READ THIS IF NOTHING ELSE!!!!
    The code as written assumes a few things regarding the configuration of the light strands.
    1.  All strands have the same number of bulbs.
    2.  All strands are connected in the order they are addressed in the strandPin array.
    3.  All strands are arranged in a left to right, right to left, left to right, etc, etc, etc pattern
       down the horizontal lines of the matrix.  Below is an example of what this means.  Each bulb noted
       in the example will be shows as "X-YY" where X is the strand (noted as A/B/C/etc and YY is the bulb
       number on its strand:

       A00  A01  A02  A03  A04  A05  A06  A07  A08  A09  A10  A11  A12  A13  A14  A15  A16  A17
       A35  A34  A33  A32  A31  A30  A29  A28  A27  A26  A25  A24  A23  A22  A21  A20  A19  A18
       B00  B01  B02  B03  B04  B05  B06  B07  B08  B09  B10  B11  B12  B13  B14  B15  B16  B17
       B35  B34  B33  B32  B31  B30  B29  B28  B27  B26  B25  B24  B23  B22  B21  B20  B19  B18
       C00  C01  C02  C03  C04  C05  C06  C07  C08  C09  C10  C11  C12  C13  C14  C15  C16  C17
       C35  C34  C33  C32  C31  C30  C29  C28  C27  C26  C25  C24  C23  C22  C21  C20  C19  C18

       The above example is the actual configuration of the display this code was originally written for,
       but larger displays would be usable with little to no modification of this code.
*/

//The bulb matrix is intended to store the address and pin data for each bulb, but also
//  to store the current RGB and intensity values we expect the bulb to already contain.
volatile bulb bulbMatrix[matrixWidth][matrixHeight];

//The buffer matrix is a frame buffer which is populated via some routine, then updateStrands()
//  is called to actually commit the changes to the bulb matrix and trasmit the changes to the
//  bulbs which require an update.
volatile bulb bufferMatrix[matrixWidth][matrixHeight];

//Just a few colors pre-defined for simple use later.  Only white is used in the code as written.
const rgb_value RGB_Red = {15, 0, 0};
const rgb_value RGB_Blue = {0, 0, 15};
const rgb_value RGB_Green = {0, 15, 0};
const rgb_value RGB_White = {13, 13, 13};              

G35::~G35() {}

G35::G35() {
}

void G35::setup() {
    int i = 0;
    while (i < strandCount) {
        pinMode(strandPin[i], OUTPUT);
        digitalWrite(strandPin[i], LOW);
        i++;
    }

    delay(1000);
    G35::addressMatrix();
}

//Generates a solid frame of the specified RGB and intensity.
//  Called by setup(). (to generate an initial solid white frame as a boot indicator)
//  Called by blitTextDisplay().  (to set the initial background when beginning text display)
//  Note, only acts on the frame buffer, does not communicate to bulbs.

void G35::clear(int r, int g, int b, int i) {
  int xpos = 0;
  int ypos = 0;
  while(ypos < matrixHeight) {
    while(xpos < matrixWidth) {
      bufferMatrix[xpos][ypos].color.r = r;
      bufferMatrix[xpos][ypos].color.g = g;
      bufferMatrix[xpos][ypos].color.b = b;
      bufferMatrix[xpos][ypos].i = i;
      xpos++;
    }
    xpos = 0;
    ypos++;
  }
}
    
void G35::setBrightness(byte brightness) {
  int xpos = 0;
  int ypos = 0;
  while(ypos < matrixHeight) {
    while(xpos < matrixWidth) {
      bufferMatrix[xpos][ypos].i = brightness;
      xpos++;
    }
    xpos = 0;
    ypos++;
  }
}

//Steps through the frame buffer, checking to see if each bulb needs updating.  If so it copies the new bulb state
//  to the bulb array and transmits the appropriate data on the appropriate data pin.
//  Called by setup() (for updated the test pattern on boot time) and blitTextDisplay(). 

void G35::draw() {
  int xpos = 0;
  int ypos = 0;
  while(ypos < matrixHeight) {
    while(xpos < matrixWidth) {
      bulbMatrix[xpos][ypos].color.r = bufferMatrix[xpos][ypos].color.r;
      bulbMatrix[xpos][ypos].color.g = bufferMatrix[xpos][ypos].color.g;
      bulbMatrix[xpos][ypos].color.b = bufferMatrix[xpos][ypos].color.b;
      bulbMatrix[xpos][ypos].i = bufferMatrix[xpos][ypos].i;
      sendBulbPacket(bulbMatrix[xpos][ypos].address, bulbMatrix[xpos][ypos].pin, bulbMatrix[xpos][ypos].color.r, bulbMatrix[xpos][ypos].color.g, bulbMatrix[xpos][ypos].color.b, bulbMatrix[xpos][ypos].i);
      xpos++;
    }
    xpos = 0;
    ypos++;
  }
}

//Initializes the bulb array, generates addresses for each bulb, assigns the correct data pin number for each bulb,
//  and transmits the addressing to the strands.
//  Called by setup().

void G35::addressMatrix() {
  int addr;
  int xpos = 0;
  int ypos = 0;
  int currentStrand = 0;
  while (currentStrand < strandCount) {
    addr = 0;
    while (addr < strandLength) {
      bulbMatrix[xpos][ypos].address = addr;
      bulbMatrix[xpos][ypos].pin = strandPin[currentStrand];
      bulbMatrix[xpos][ypos].color.r = 0;
      bulbMatrix[xpos][ypos].color.g = 0;
      bulbMatrix[xpos][ypos].color.b = 0;
      bulbMatrix[xpos][ypos].i = 0;

      sendBulbPacket(bulbMatrix[xpos][ypos].address, bulbMatrix[xpos][ypos].pin, bulbMatrix[xpos][ypos].color.r, bulbMatrix[xpos][ypos].color.g, bulbMatrix[xpos][ypos].color.b, bulbMatrix[xpos][ypos].i);
      delay(1);
      if ((ypos & 1) == 0) {
        if (xpos < (matrixWidth - 1)) {
          xpos++;
        } else {
          ypos++;
        }
      } else {
        if (xpos > 0) {
          xpos--;
        } else {
          ypos++;
        }
      }
      addr++;
    }
    currentStrand++;
  }
}

//Generates data stream from defined values and then transmits that data to the defined bulb.
//  Called by addressMatrix() and updateStrands().

void G35::sendBulbPacket(byte address, int pin, byte r, byte g, byte b, byte i) {
  boolean streamBuffer[26];
  int streamPos = 0;
  int bitPos;
  int currentData;
  //Simple state machine to step through the data to set up a stream buffer we will use to quickly transmit the data.
  //Note that you _could_ roll the bit transmission into this also, but timing would become trickier, as you would need
  //to time the operations between each bit shift so as to not overextend between shifts.  Building the stream and then
  //transmitting it in a tight loop was a much simpler solution in the development timeframe available.
  while (streamPos < 26) {
    switch (streamPos) {
      case 0:
        bitPos = 6;
        currentData = address;
        break;
      case 6:
        bitPos = 8;
        currentData = i;
        break;
      case 14:
        bitPos = 4;
        currentData = b;
        break;
      case 18:
        bitPos = 4;
        currentData = g;
        break;
      case 22:
        bitPos = 4;
        currentData = r;
        break;
      default:
        break;
    } 
    if ((currentData & (1 << (bitPos - 1))) != 0) {
      streamBuffer[streamPos] = true;
    } else {
      streamBuffer[streamPos] = false;
    }
    bitPos--;
    streamPos++;
  }

  //send start bit
  fastDigitalWrite(pin);
  delayMicroseconds(8);
  streamPos = 0;
  while (streamPos < 26) {
    if (streamBuffer[streamPos]) {
      //send a 1
      fastDigitalWrite(pin);
      delayMicroseconds(19);
      fastDigitalWrite(pin);
      delayMicroseconds(8);
    } else {
      //send a 0
      fastDigitalWrite(pin);
      delayMicroseconds(9);
      fastDigitalWrite(pin);
      delayMicroseconds(18);
    }
    streamPos++;
  }
  fastDigitalWrite(pin);
}

//Function designed to flip the described output pin.  On to off, off to on.
//  Note that this is written for an arduino mega, with pins 22 through 29.
//    easily expandable, or convertable to a different ard type, you just
//    have to look up which port register correlates to which pin.  I recommend
//    staying away from whichever register on your ard contains the pins for the
//    primary onboard serial, as you can muff the ability to reload/debug if you
//    start erroneously setting the state of those registers.
//
//  This was added because digitalWrite was taking too long to execute for the
//    timing that needed to be accomplished.  It was also sometimes executing faster
//    and sometimes slower (4-5 us) which made timing a real pain.  This method is
//    VERY predictable in timing, and executes extremely fast.  You just have to be
//    careful with it.
//
//  A simple flip was easiest in this case, as all commo done in updateStrands() is
//    a fixed number of state changes.  This it is completely predictable in terms of
//    knowing the state of the output upon exit/iteration.  In a more complex, but
//    still timing sensitive application it would probably be better to make two functions,
//    one to explicitly set the pin on, the other to explicitly set the pin off.  Then
//    you would have all the speed, but less ambiguity in a complex environment.

void G35::fastDigitalWrite(uint8_t pin) {
    switch(pin) {
        case 8:
            PORTB = (PORTB ^ B00000001);
            break;
        case 9:
            PORTB = (PORTB ^ B00000010);
            break;
        case 10:
            PORTB = (PORTB ^ B00000100);
            break;
        case 11:
            PORTB = (PORTB ^ B00001000);
            break;
        case 12:
            PORTB = (PORTB ^ B00010000);
            break;
        case 13:
            PORTB = (PORTB ^ B00100000);
            break;

        #ifdef PORTA
        case 22:
            PORTA = (PORTA ^ B00000001);
            break;
        case 23:
            PORTA = (PORTA ^ B00000010);
            break;
        case 24:
            PORTA = (PORTA ^ B00000100);
            break;
        case 25:
            PORTA = (PORTA ^ B00001000);
            break;
        case 26:
            PORTA = (PORTA ^ B00010000);
            break;
        case 27:
            PORTA = (PORTA ^ B00100000);
            break;
        case 28:
            PORTA = (PORTA ^ B01000000);
            break;
        case 29:
            PORTA = (PORTA ^ B10000000);
            break;
        #endif

        case 30:
            PORTC = (PORTC ^ B10000000);
            break;
        case 31:
            PORTC = (PORTC ^ B01000000);
            break;
        case 32:
            PORTC = (PORTC ^ B00100000);
            break;
        default:
            break;
    }
}
