//G35 LED Scrolling Marquee
//  Project by John Riney and Jason Beeland
//  Concept and Engineering by John Riney
//  Code by Jason Beeland
//
//  This was a 48 hour new years project Riney and I put together in order to
//  take a functioning display to a friend's house for a new years eve party.
//  All work, start to finish was completed in that time span.  The only
//  additions after the fact are renaming a few variables for clarity and
//  documentation of the code.
//
//  I had a ton of fun working on this project with Riney.  I learned a great
//  deal, and hope that in sharing this code others will have the same fun in
//  expanding on what we've done here.
//
//  Feel free to use this code in any and all endeavors; all I ask is that if
//  you lift code for reuse from here you post your own code for others to
//  benefit from as well.
//
//  -Jason Beeland

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

//Debug levels are provided, as the debugging code in the strand update seriously slows down
//the refresh rate.
//  Debug level 0 indicates that debugging is fully off.
//  Debug level 1 isn't implimented, but would involve minor, non-invasive debugging.
//  Debug level 2 is full debugging, with all updates sending bitstreams to serial, etc.
//  Debug level 3 is full debugging, but ONLY during addressing and initialization.
volatile int debugLevel = 0;

//The width, in bulbs, of the assembled display.
const int matrixWidth = 18;
//The height, in bulbs, of the assembled display.
const int matrixHeight = 10;
//The number if strands used to form the display.
const int strandCount = 5;
//The number of bulbs on each strand.
const int strandLength = 36;
//The digital pins assigned to the strands' data buses.
const int strandPin[strandCount] = {22, 24, 26, 28, 30};

//SPECIAL NOTE - READ THIS IF NOTHING ELSE!!!!
//  The code as written assumes a few things regardnig the configuration of the light strands.
//  1.  All strands have the same number of bulbs.
//  2.  All strands are connected in the order they are addressed in the strandPin array.
//  3.  All strands are arranged in a left to right, right to left, left to right, etc, etc, etc pattern
//      down the horizontal lines of the matrix.  Below is an example of what this means.  Each bulb noted
//      in the example will be shows as "X-YY" where X is the strand (noted as A/B/C/etc and YY is the bulb
//      number on its strand:
//
//      A00  A01  A02  A03  A04  A05  A06  A07  A08  A09  A10  A11  A12  A13  A14  A15  A16  A17
//      A35  A34  A33  A32  A31  A30  A29  A28  A27  A26  A25  A24  A23  A22  A21  A20  A19  A18
//      B00  B01  B02  B03  B04  B05  B06  B07  B08  B09  B10  B11  B12  B13  B14  B15  B16  B17
//      B35  B34  B33  B32  B31  B30  B29  B28  B27  B26  B25  B24  B23  B22  B21  B20  B19  B18
//      C00  C01  C02  C03  C04  C05  C06  C07  C08  C09  C10  C11  C12  C13  C14  C15  C16  C17
//      C35  C34  C33  C32  C31  C30  C29  C28  C27  C26  C25  C24  C23  C22  C21  C20  C19  C18
//
//      The above example is the actual configuration of the display this code was originally written for,
//      but larger displays would be usable with little to no modification of this code.


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

const int blitFontHeight = 6;
const int blitFontWidth = 5;
const int blitFontLength = 40;
boolean blitFont[blitFontLength][blitFontHeight][blitFontWidth] = {
{    { false, false, false, false, false },    //0  Whitespace
     { false, false, false, false, false },
     { false, false, false, false, false },
     { false, false, false, false, false },
     { false, false, false, false, false },
     { false, false, false, false, false }    },
{    { false, true,  true,  true,  false },    //1  A
     { true,  false, false, false, true  },
     { true,  false, false, false, true  },
     { true,  true,  true,  true,  true  },
     { true,  false, false, false, true  },
     { true,  false, false, false, true  }    },
{    { true,  true,  true,  true,  false },    //2  B
     { true,  false, false, false, true  },
     { true,  false, false, false, true  },
     { true,  true,  true,  true,  false },
     { true,  false, false, false, true  },
     { true,  true,  true,  true,  false }    },
{    { false, true,  true,  true,  false },    //3  C
     { true,  false, false, false, true  },
     { true,  false, false, false, false },
     { true,  false, false, false, false  },
     { true,  false, false, false, true  },
     { false, true,  true,  true,  false }    },
{    { true,  true,  true,  true,  false },    //4  D
     { true,  false, false, false, true  },
     { true,  false, false, false, true  },
     { true,  false, false, false, true  },
     { true,  false, false, false, true  },
     { true,  true,  true,  true,  false }    },
{    { true,  true,  true,  true,  true  },    //5  E
     { true,  false, false, false, false },
     { true,  false, false, false, false },
     { true,  true,  true,  true,  false },
     { true,  false, false, false, false },
     { true,  true,  true,  true,  true  }    },
{    { true,  true,  true,  true,  true  },    //6  F
     { true,  false, false, false, false },
     { true,  true,  true,  true,  false },
     { true,  false, false, false, false },
     { true,  false, false, false, false },
     { true,  false, false, false, false }    },
{    { false, true,  true,  true,  false },    //7  G
     { true,  false, false, false, true  },
     { true,  false, false, false, false },
     { true,  false, true,  true,  false },
     { true,  false, false, false, true  },
     { false, true,  true,  true,  false }    },
{    { true,  false, false, false, true  },    //8  H
     { true,  false, false, false, true  },
     { true,  false, false, false, true  },
     { true,  true,  true,  true,  true  },
     { true,  false, false, false, true  },
     { true,  false, false, false, true  }    },
{    { true,  true,  true,  true,  true  },    //9  I
     { false, false, true,  false, false },
     { false, false, true,  false, false },
     { false, false, true,  false, false },
     { false, false, true,  false, false },
     { true,  true,  true,  true,  true  }    },
{    { true,  true,  true,  true,  true  },    //10  J
     { false, false, true,  false, false },
     { false, false, true,  false, false },
     { true,  false, true,  false, false },
     { true,  false, true,  false, false },
     { false, true,  false, false, false }    },
{    { true,  false, false, false, true  },    //11  K
     { true,  false, false, false, true  },
     { true,  false, false, true,  false },
     { true,  true,  true,  false, false },
     { true,  false, false, true,  false },
     { true,  false, false, false, true  }    },
{    { true,  false, false, false, false },    //12  L
     { true,  false, false, false, false },
     { true,  false, false, false, false },
     { true,  false, false, false, false },
     { true,  false, false, false, false },
     { true,  true,  true,  true,  true  }    },
{    { true,  false, false, false, true, },    //13  M
     { true,  true,  false, true,  true  },
     { true,  false, true,  false, true  },
     { true,  false, true,  false, true  },
     { true,  false, false, false, true  },
     { true,  false, false, false, true  }    },
{    { true,  true,  false, false, true  },    //14  N
     { true,  true,  false, false, true  },
     { true,  false, true,  false, true  },
     { true,  false, false, true,  true  },
     { true,  false, false, true,  true  },
     { true,  false, false, false, true  }    },
{    { false, true,  true,  true,  false },    //15  O
     { true,  false, false, false, true  },
     { true,  false, false, false, true  },
     { true,  false, false, false, true  },
     { true,  false, false, false, true  },
     { false, true,  true,  true,  false }    },
{    { true,  true,  true,  true,  false },    //16  P
     { true,  false, false, false, true  },
     { true,  false, false, false, true  },
     { true,  true,  true,  true,  false },
     { true,  false, false, false, false },
     { true,  false, false, false, false }    },
{    { false, true,  true,  true,  false },    //17  Q
     { true,  false, false, false, true  },
     { true,  false, false, false, true  },
     { true,  false, true,  false, true  },
     { true,  false, false, true,  true  },
     { false, true,  true,  true,  true  }    },
{    { true,  true,  true,  true,  false },    //18  R
     { true,  false, false, false, true  },
     { true,  false, false, false, true  },
     { true,  true,  true,  true,  false },
     { true,  false, false, false, true  },
     { true,  false, false, false, true  }    },
{    { false, true,  true,  true,  false },    //19  S
     { true,  false, false, false, true  },
     { false, true,  false, false, false },
     { false, false, true,  true,  false },
     { true,  false, false, false, true  },
     { false, true,  true,  true,  false }    },
{    { true,  true,  true,  true,  true  },    //20  T
     { false, false, true,  false, false },
     { false, false, true,  false, false },
     { false, false, true,  false, false },
     { false, false, true,  false, false },
     { false, false, true,  false, false }    },
{    { true,  false, false, false, true  },    //21  U
     { true,  false, false, false, true  },
     { true,  false, false, false, true  },
     { true,  false, false, false, true  },
     { true,  false, false, false, true  },
     { false, true,  true,  true,  false }    },
{    { true,  false, false, false, true  },    //22  V
     { true,  false, false, false, true  },
     { true,  false, false, false, true  },
     { true,  true,  false, true,  true  },
     { false, true,  false, true,  false },
     { false, true,  true,  true,  false }    },
{    { true,  false, false, false, true  },    //23  W
     { true,  false, false, false, true  },
     { true,  false, false, false, true  },
     { true,  false, true,  false, true  },
     { true,  false, true,  false, true  },
     { false, true,  false, true,  false }    },
{    { true,  false, false, false, true  },    //24  X
     { false, true,  false, true,  false },
     { false, false, true,  false, false },
     { false, false, true,  false, false },
     { false, true,  false, true,  false },
     { true,  false, false, false, true  }    },
{    { true,  false, false, false, true  },    //25  Y
     { true,  false, false, false, true  }, 
     { false, true,  false, true,  false },
     { false, false, true,  false, false },
     { false, false, true,  false, false },
     { false, false, true,  false, false }    },
{    { true,  true,  true,  true,  true  },    //26  Z
     { false, false, false, false, true  },
     { false, false, false, true,  false },
     { false, false, true,  false, false },
     { false, true,  false, false, false },
     { true,  true,  true,  true,  true  }    },
{    { false, true,  true,  true,  false },    //27  0
     { true,  false, false, false, true  },
     { true,  false, false, false, true  },
     { true,  false, false, false, true  },
     { true,  false, false, false, true  },
     { false, true,  true,  true,  false }    },
{    { false, false, true,  false, false },    //28  1
     { false, true,  true,  false, false },
     { false, false, true,  false, false },
     { false, false, true,  false, false },
     { false, false, true,  false, false },
     { false, true,  true,  true,  false }    },
{    { false, true,  true,  true,  false },    //29  2
     { true,  false, false, false, true  },
     { false, false, false, true,  false },
     { false, false, true,  false, false },
     { false, true,  false, false, false },
     { true,  true,  true,  true,  true  }    },
{    { false, true,  true,  true,  false },    //30  3
     { true,  false, false, false, true  },
     { false, false, false, false, true },
     { false, false, true,  true,  false },
     { true,  false, false, false, true  },
     { false, true,  true,  true,  false }    },
{    { false, false, false, true,  false },    //31  4
     { false, false, true,  true,  false },
     { false, true,  false, true,  false },
     { true,  true,  true,  true,  true },
     { false, false, false, true,  false },
     { false, false, false, true,  false }    },
{    { true,  true,  true,  true,  true  },    //32  5
     { true,  false, false, false, false },
     { true,  false, false, false, false },
     { true,  true,  true,  true,  false },
     { false, false, false, false, true  },
     { true,  true,  true,  true,  false }    },
{    { false, true,  true,  true,  false },    //33  6
     { true,  false, false, false, false },
     { true,  false, false, false, false },
     { true,  false, true,  true,  false },
     { true,  true,  false, false, true  },
     { false, true,  true,  true,  false }    },
{    { true,  true,  true,  true,  true  },    //34  7
     { false, false, false, false, true  },
     { false, false, false, true,  false },
     { false, false, true,  false, false },
     { false, true,  false, false, false },
     { true,  false, false, false, false }    },
{    { false, true,  true,  true,  false },    //35  8
     { true,  false, false, false, true  },
     { false, true,  false, true,  false },
     { false, true,  true,  true,  false },
     { true,  false, false, false, true  },
     { false, true,  true,  true,  false }    },
{    { false, true,  true,  true,  false },    //36  9
     { true,  false, false, false, true  },
     { true,  false, false, true,  true  },
     { false, true,  true,  false, true  },
     { false, false, false, false, true  },
     { false, false, false, false, true  }    },
{    { false, false, true,  true,  false },    //37  !
     { false, false, true,  true,  false },
     { false, false, true,  true,  false },
     { false, false, true,  true,  false },
     { false, false, false, false, false },
     { false, false, true,  true,  false }    },
{    { false, false, true,  false, false },    //38  :
     { false, false, true,  false, false },
     { false, false, false, false, false },
     { false, false, false, false, false },
     { false, false, true,  false, false },
     { false, false, true,  false, false }    },
{    { false, false, false, false, true  },    //39  /
     { false, false, false, true,  false },
     { false, false, true,  false, false },
     { false, false, true,  false, false },
     { false, true,  false, false, false },
     { true,  false, false, false, false }    }
};
               

              

void setup() {
  Serial.begin(9600);
  Serial.println("Device Start");
  int i = 0;
  while (i < strandCount) {
    pinMode(strandPin[i], OUTPUT);
    digitalWrite(strandPin[i], LOW);
    Serial.print("Configured strand ");
    Serial.print(i);
    Serial.print(" on output pin ");
    Serial.println(strandPin[i]);
    i++;
  }
  Serial.println("Output Pins Configured");
  
  delay(1000);
  
  addressMatrix();
  Serial.println("String Addressed");
  buildSolidDisplay(RGB_White.r, RGB_White.g, RGB_White.b, 200);
  Serial.println("Default display frame generated");
  updateStrands();
  if (debugLevel == 3) {
    debugLevel = 0;
  }
}

void loop() {
  int color_base;
  byte my_r;
  byte my_g;
  byte my_b;
  int xpos = 0;
  int ypos = 0;
  while(ypos < matrixHeight) {
    while(xpos < matrixWidth) {
      bufferMatrix[xpos][ypos].color.r = random(0, 15);
      bufferMatrix[xpos][ypos].color.g = random(0, 15);
      bufferMatrix[xpos][ypos].color.b = random(0, 15);
      bufferMatrix[xpos][ypos].i = random(0, 200);
      xpos++;
    }
    xpos = 0;
    ypos++;
  }
  updateStrands();
  while (1) {
    xpos = 0;
    ypos = 0;
    while(ypos < matrixHeight) {
      while(xpos < matrixWidth) {
        if (bufferMatrix[xpos][ypos].i > 0) {
          bufferMatrix[xpos][ypos].i--;
        }
        else {
          bufferMatrix[xpos][ypos].color.r = random(0, 15);
          bufferMatrix[xpos][ypos].color.g = random(0, 15);
          bufferMatrix[xpos][ypos].color.b = random(0, 15);
          bufferMatrix[xpos][ypos].i = random(50, 200);
        }
        xpos++;
      }
      xpos = 0;
      ypos++;
    }
    
    updateStrands();
  }
}

//Generates a full frame with each bulb being assigned a random color.
//  Not called in code as written, included merely as an example.

void buildRandomDisplay() {
  int xpos = 0;
  int ypos = 0;
  while(ypos < matrixHeight) {
    while(xpos < matrixWidth) {
      bufferMatrix[xpos][ypos].color.r = random(0, 13);
      bufferMatrix[xpos][ypos].color.g = random(0, 13);
      bufferMatrix[xpos][ypos].color.b = random(0, 13);
      bufferMatrix[xpos][ypos].i = 200;
      xpos++;
    }
    xpos = 0;
    ypos++;
  }
}

//Scrolls text across the screen.
//  displayText is the string you wish to be displayed.  Ensure all characters are included in the font.  If they
//    are not it will simply display a space in replacement of the unknown character.
//  refreshDelay is the bounding time limit in ms between steps of the text across the screen.  Note that this is
//    an lower limit bound, not an upper limit bound.  If set it low enough you will see a variance in display speed
//    as the display will refresh faster when there is less communication to be done, and slower when more bulbs
//    need to be refreshed.  Provided this is set higher than the slowest update speed in your text display sequence
//    it will gate the refresh so that it becomes smooth, even if few or even no bulbs need updating on a particular
//    refresh cycle.
//  loops is the number of times you want the text to scroll across the display before returning to the calling function.
//  r_t, g_t, b_t, and i_t represent the RGB and intensity values of the bulbs which make up the text
//  r_b, g_b, b_b, and i_b represent the RGB and intensity values of the bulbs which make up the background
//
//  Note that this does call updateStrands(), and thus will both build a frame AND display it.  As it is a loop it will
//    not return to the calling function untill it has fully completed displaying the defined text for the defined number of times.

void blitTextDisplay(String displayText, int refreshDelay, int loops, int r_t, int g_t, int b_t, int i_t, int r_b, int g_b, int b_b, int i_b) {
  int charPos = 0;
  int textPos = 0;
  int loopCount = 0;
  int textLength = displayText.length();
  unsigned long lastDispTime = 0;
  unsigned long currTime = 0;
  buildSolidDisplay(r_b, g_b, b_b, i_b);
  while((loops == 0) || (loopCount < loops)) {
    textPos = 0;
    charPos = 0;
    while (textPos < textLength) {
      currTime = millis();
      if ((lastDispTime == 0) || (currTime >= (lastDispTime + refreshDelay))) {
        blitShiftMatrixLeft();
        if (   (charPos == -1)   &&   (  skipSpace(displayText[textPos]) || skipSpace(displayText[(textPos - 1)])  )       ) {
          charPos = 0;
        }
        if(charPos == -1) {
          blitAddSpacerLine(r_b, g_b, b_b, i_b);
        } else {
          blitAddScanLine(displayText[textPos], charPos, r_t, g_t, b_t, i_t, r_b, g_b, b_b, i_b);
        }
        charPos++;
        if(charPos >= blitFontWidth) {
          charPos = -1;
          textPos++;
        }
        updateStrands();
        lastDispTime = currTime;
      }
    }
    loopCount++;
  }
}

//Boolean test to determine whether a space is needed between the characters or not.
//  As written it indicates the extra spacer line should be skipped if either of the
//  surrounding characters is a space or an exclaimation point.
//  Called by blitTextDisplay().

boolean skipSpace(char testChar) {
  switch(testChar) {
    case ' ':
    case '!':
      return true;
    default:
      return false;
  }
}

//Shifts the entire matrix one scan line to the left, freeing the last scan line for new data.  Called by blitTextDisplay().
//  Note, only acts on the frame buffer, does not communicate to bulbs.

void blitShiftMatrixLeft() {
  int xpos = 0;
  int ypos = 0;
  while(ypos < matrixHeight) {
    while(xpos < (matrixWidth - 1)) {
      bufferMatrix[xpos][ypos].color.r = bufferMatrix[(xpos + 1)][ypos].color.r;
      bufferMatrix[xpos][ypos].color.g = bufferMatrix[(xpos + 1)][ypos].color.g;
      bufferMatrix[xpos][ypos].color.b = bufferMatrix[(xpos + 1)][ypos].color.b;
      bufferMatrix[xpos][ypos].i = bufferMatrix[(xpos + 1)][ypos].i;
      xpos++;
    }
    xpos = 0;
    ypos++;
  }
}

//Generate the new scan line of the blitted text.  Called by blitTextDisplay().
//  r_t, g_t, b_t, and i_t represent the RGB and intensity values of the bulbs which make up the text
//  r_b, g_b, b_b, and i_b represent the RGB and intensity values of the bulbs which make up the background
//  Note, only acts on the frame buffer, does not communicate to bulbs.

void blitAddScanLine(char currChar, int charPos, byte r_t, byte g_t, byte b_t, byte i_t, byte r_b, byte g_b, byte b_b, byte i_b) {
  int charIndex = getFontIndex(currChar);
  int ypos = 0;
  while (ypos < blitFontHeight) {
    if (blitFont[charIndex][ypos][charPos]) {
      bufferMatrix[(matrixWidth - 1)][ypos].color.r = r_t;
      bufferMatrix[(matrixWidth - 1)][ypos].color.g = g_t;
      bufferMatrix[(matrixWidth - 1)][ypos].color.b = b_t;
      bufferMatrix[(matrixWidth - 1)][ypos].i = i_t;
    } else {
      bufferMatrix[(matrixWidth - 1)][ypos].color.r = r_b;
      bufferMatrix[(matrixWidth - 1)][ypos].color.g = g_b;
      bufferMatrix[(matrixWidth - 1)][ypos].color.b = b_b;
      bufferMatrix[(matrixWidth - 1)][ypos].i = i_b;
    }
    ypos++;
  }
}

//Matches the character the function is called with to the correct index in the font array.
//  Called by blitAddScanLine().
//
//  Could have modified case of the char then tested, but this way it is easier to modify later if
//    some or all lower case letters need to be defined in the font.

int getFontIndex(char currChar) {
  switch(currChar) {
    case ' ':
      return 0;
    case 'a':
    case 'A':
      return 1;
    case 'b':
    case 'B':
      return 2;
    case 'c':
    case 'C':
      return 3;
    case 'd':
    case 'D':
      return 4;
    case 'e':
    case 'E':
      return 5;
    case 'f':
    case 'F':
      return 6;
    case 'g':
    case 'G':
      return 7;
    case 'h':
    case 'H':
      return 8;
    case 'i':
    case 'I':
      return 9;
    case 'j':
    case 'J':
      return 10;
    case 'k':
    case 'K':
      return 11;
    case 'l':
    case 'L':
      return 12;
    case 'm':
    case 'M':
      return 13;
    case 'n':
    case 'N':
      return 14;
    case 'o':
    case 'O':
      return 15;
    case 'p':
    case 'P':
      return 16;
    case 'q':
    case 'Q':
      return 17;
    case 'r':
    case 'R':
      return 18;
    case 's':
    case 'S':
      return 19;
    case 't':
    case 'T':
      return 20;
    case 'u':
    case 'U':
      return 21;
    case 'v':
    case 'V':
      return 22;
    case 'w':
    case 'W':
      return 23;
    case 'x':
    case 'X':
      return 24;
    case 'y':
    case 'Y':
      return 25;
    case 'z':
    case 'Z':
      return 26;
    case '0':
      return 27;
    case '1':
      return 28;
    case '2':
      return 29;
    case '3':
      return 30;
    case '4':
      return 31;
    case '5':
      return 32;
    case '6':
      return 33;
    case '7':
      return 34;
    case '8':
      return 35;
    case '9':
      return 36;
    case '!':
      return 37;
    case ':':
      return 38;
    case '/':
      return 39;
    default:
      return 0;
  }
}
    
//Adds a vertical line of blank space at the right side of the array.  Intended to provide spacing between characters.
//  Called by blitTextDisplay().

void blitAddSpacerLine(byte r, byte g, byte b, byte i) {
  int ypos = 0;
  while(ypos < matrixHeight) {
    bufferMatrix[(matrixWidth - 1)][ypos].color.r = r;
    bufferMatrix[(matrixWidth - 1)][ypos].color.g = g;
    bufferMatrix[(matrixWidth - 1)][ypos].color.b = b;
    bufferMatrix[(matrixWidth - 1)][ypos].i = i;
    ypos++;
  }
}

//Generates a solid frame of the specified RGB and intensity.
//  Called by setup(). (to generate an initial solid white frame as a boot indicator)
//  Called by blitTextDisplay().  (to set the initial background when beginning text display)
//  Note, only acts on the frame buffer, does not communicate to bulbs.

void buildSolidDisplay(int r, int g, int b, int i) {
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
  
//Compares the described xy coordinates in the frame buffer to the bulb array to determine
//  whether or not the bulb needs to be updated.
//  Called by updateStrands().

boolean needsUpdate(int xpos, int ypos) {
  if(bulbMatrix[xpos][ypos].color.r != bufferMatrix[xpos][ypos].color.r) {
    return true;
  } else {
    if(bulbMatrix[xpos][ypos].color.g != bufferMatrix[xpos][ypos].color.g) {
      return true;
    } else {
      if(bulbMatrix[xpos][ypos].color.b != bufferMatrix[xpos][ypos].color.b) {
        return true;
      } else {
        if(bulbMatrix[xpos][ypos].i != bufferMatrix[xpos][ypos].i) {
          return true;
        } else {
          return false;
        }
      }
    }
  }
}

//Copies the RGB and intensity values from the frame buffer to the bulb array for the described xy coordinate.
//  Called by updateStrands().

void copyState(int xpos, int ypos) {
  bulbMatrix[xpos][ypos].color.r = bufferMatrix[xpos][ypos].color.r;
  bulbMatrix[xpos][ypos].color.g = bufferMatrix[xpos][ypos].color.g;
  bulbMatrix[xpos][ypos].color.b = bufferMatrix[xpos][ypos].color.b;
  bulbMatrix[xpos][ypos].i = bufferMatrix[xpos][ypos].i;
}

//Steps through the frame buffer, checking to see if each bulb needs updating.  If so it copies the new bulb state
//  to the bulb array and transmits the appropriate data on the appropriate data pin.
//  Called by setup() (for updated the test pattern on boot time) and blitTextDisplay(). 

void updateStrands() {
  int xpos = 0;
  int ypos = 0;
  while(ypos < matrixHeight) {
    while(xpos < matrixWidth) {
      if(needsUpdate(xpos, ypos)) {
        copyState(xpos, ypos);
        sendBulbPacket(bulbMatrix[xpos][ypos].address, bulbMatrix[xpos][ypos].pin, bulbMatrix[xpos][ypos].color.r, bulbMatrix[xpos][ypos].color.g, bulbMatrix[xpos][ypos].color.b, bulbMatrix[xpos][ypos].i);
      }
      xpos++;
    }
    xpos = 0;
    ypos++;
  }
}

//Initializes the bulb array, generates addresses for each bulb, assigns the correct data pin number for each bulb,
//  and transmits the addressing to the strands.
//  Called by setup().

void addressMatrix() {
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
      if (debugLevel > 0) {
        Serial.print("Init bulb at ");
        Serial.print(xpos);
        Serial.print(", ");
        Serial.println(ypos);
      }
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

void sendBulbPacket(byte address, int pin, byte r, byte g, byte b, byte i) {
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
  if (debugLevel > 1) {
    Serial.print("Pin: ");
    Serial.print(pin);
    Serial.print("    Address: ");
    Serial.print((int) address);
    Serial.print("    Intensity: ");
    Serial.print((int) i);
    Serial.print("    Blue: ");
    Serial.print((int) b);
    Serial.print("    Green: ");
    Serial.print((int) g);
    Serial.print("    Red: ");
    Serial.println((int) r);
    Serial.println("Created data frame:");
    streamPos = 0;
    while (streamPos < 26) {
      if (streamBuffer[streamPos]) {
        Serial.print(1);
      } else {
      Serial.print(0);
      }
      streamPos++;
    }
    Serial.println();
    Serial.println();
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

void fastDigitalWrite(uint8_t pin) {
    switch(pin) {
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
