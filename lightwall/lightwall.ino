// -*- c++ -*-
//
// Copyright 2010 Ovidiu Predescu <ovidiu@gmail.com>
// Date: June 2010
// Updated: 08-JAN-2012 for Arduno IDE 1.0 by <Hardcore@hardcoreforensics.com>
//

#include <pins_arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <Flash.h>
#include <SD.h>
#include <TinyWebServer.h>

/****************VALUES YOU CHANGE*************/
// pin 4 is the SPI select pin for the SDcard
const int SD_CS = 4;

// pin 10 is the SPI select pin for the Ethernet
const int ETHER_CS = 10;

// Don't forget to modify the IP to an available one on your home network
byte ip[] = { 192, 168, 1, 250 };
/*********************************************/

static uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

/*************************************
 HTTP ROUTING
**************************************/
boolean index_handler(TinyWebServer& web_server);
boolean status_handler(TinyWebServer& web_server);
boolean file_handler(TinyWebServer& web_server);
boolean set_handler(TinyWebServer& web_server);
boolean clear_handler(TinyWebServer& web_server);

TinyWebServer::PathHandler handlers[] = {
  { "/",             TinyWebServer::GET, &index_handler },
  { "/status",       TinyWebServer::GET, &status_handler },
  { "/public/" "*",  TinyWebServer::GET, &file_handler },
  { "/upload/" "*",  TinyWebServer::PUT, &TinyWebPutHandler::put_handler },
  { "/s/" "*",       TinyWebServer::GET, &set_handler },
  { "/c",       TinyWebServer::GET, &clear_handler },
  {NULL}
};

/*************************************
 HTTP HANDLERS
**************************************/
boolean index_handler(TinyWebServer& web_server) {
  send_file_name(web_server, "index.htm");
  return true;
}

boolean status_handler(TinyWebServer& web_server) {
  web_server.send_error_code(200);
  web_server.end_headers();
  web_server << F("<html><body>status wooooooooo</body></html>\n");
  return true;
}

SdFile root;
SdFile file;
boolean has_filesystem = true;
Sd2Card card;
SdVolume volume;

void send_file_name(TinyWebServer& web_server, const char* filename) {
  if (!filename) {
    web_server.send_error_code(404);
    web_server << F("Could not parse URL");
  }
  else {
    TinyWebServer::MimeType mime_type = TinyWebServer::get_mime_type_from_filename(filename);
    web_server.send_error_code(200);
    web_server.send_content_type(mime_type);
    web_server.end_headers();
    if (file.open(&root, filename, O_READ)) {
      Serial << F("Read file "); Serial.println(filename);
      web_server.send_file(file);
      file.close();
    }
    else {
      web_server << F("Could not find file: ") << filename << "\n";
    }
  }
}

boolean file_handler(TinyWebServer& web_server) {
  char* filename = TinyWebServer::get_file_from_path(web_server.get_path());
  send_file_name(web_server, filename);
  free(filename);
  return true;
}

void file_uploader_handler(TinyWebServer& web_server, TinyWebPutHandler::PutAction action, char* buffer, int size) {
  static uint32_t start_time;
  static uint32_t total_size;

  switch (action) {
  case TinyWebPutHandler::START:
    start_time = millis();
    total_size = 0;
    if (!file.isOpen()) {
      // File is not opened, create it. First obtain the desired name
      // from the request path.
      char* fname = web_server.get_file_from_path(web_server.get_path());
      if (fname) {
        Serial << F("Creating ") << fname << "\n";
        file.open(&root, fname, O_CREAT | O_WRITE | O_TRUNC);
        free(fname);
      }
    }
    break;

  case TinyWebPutHandler::WRITE:
    if (file.isOpen()) {
      file.write(buffer, size);
      total_size += size;
    }
    break;

  case TinyWebPutHandler::END:
    file.sync();
    Serial << F("Wrote ") << file.fileSize() << F(" bytes in ") << millis() - start_time << F(" millis (received ")
           << total_size << F(" bytes)\n");
    file.close();
  }
}


boolean set_handler(TinyWebServer& web_server) {
  char* num = web_server.get_file_from_path(web_server.get_path());
  int x, y, r, g, b, i;
  sscanf(num, "%d,%d,%d,%d,%d", &x, &y, &r, &g, &b, &i);
 
  web_server.send_error_code(200);
  web_server.end_headers();
  
  
  return true;
}


boolean clear_handler(TinyWebServer& web_server) {
  web_server.send_error_code(200);
  web_server.end_headers();
  web_server << F("<html><body>clear</body></html>\n");
  return true;
}

boolean has_ip_address = false;
const char* headers[] = {
  "Content-Length",
  NULL
};

TinyWebServer web = TinyWebServer(handlers, headers);

/*************************************
 DERPY UTILS
**************************************/
const char* ip_to_str(const uint8_t* ipAddr) {
  static char buf[16];
  sprintf(buf, "%d.%d.%d.%d\0", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);
  return buf;
}

const char* mac_to_str(const uint8_t* mac) {
  static char buf[24];
  sprintf(buf, "%X:%X:%X:%X:%X:%X\0", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] );
  return buf;
}

/*************************************
 G35 INIT
**************************************/
void setup_lights() {
  Serial << F("Starting lights, ");
  setup_g35();
  buildSolidDisplay(13, 13, 13, 200);
}

void setup_sd() {
  // initialize the SD card.
  Serial << F("SD card, ");
  // pass over the speed and Chip select for the SD card
  if (!card.init(SPI_FULL_SPEED, SD_CS)) {
    Serial << F("card failed\n");
    has_filesystem = false;
  }
  // initialize a FAT volume.
  if (!volume.init(&card)) {
    Serial << F("vol.init failed!\n");
    has_filesystem = false;
  }
  if (!root.openRoot(&volume)) {
    Serial << F("openRoot failed");
    has_filesystem = false;
  }
}

/*************************************
 WEB SERVER INIT
**************************************/
void setup_server() {
  pinMode(SS_PIN, OUTPUT);	// set the SS pin as an output
                                // (necessary to keep the board as
                                // master and not SPI slave)
  digitalWrite(SS_PIN, HIGH);	// and ensure SS is high

  // Ensure we are in a consistent state after power-up or a reset
  // button These pins are standard for the Arduino w5100 Rev 3
  // ethernet board They may need to be re-jigged for different boards
  pinMode(ETHER_CS, OUTPUT);	// Set the CS pin as an output
  digitalWrite(ETHER_CS, HIGH);	// Turn off the W5100 chip! (wait for
                                // configuration)
  pinMode(SD_CS, OUTPUT);	// Set the SDcard CS pin as an output
  digitalWrite(SD_CS, HIGH);	// Turn off the SD card! (wait for
                                // configuration)

  // Initialize the Ethernet.
  Serial << F("ethernet, ");
  Ethernet.begin(mac, ip);

  // Start the web server.
  Serial << F("HTTP.\n");
  if (has_filesystem) {
    TinyWebPutHandler::put_handler_fn = file_uploader_handler;
  }
  web.begin();

  Serial << F("Listening on ") << ip_to_str(ip) << F("\n");

}

void setup() {
  Serial.begin(115200);
  Serial << F("lightwall (c)2012 riney, ") << FreeRam() << F(" BASIC bytes free\n");
  Serial << F("MAC ") << mac_to_str(mac) << F("\n");
  
  setup_lights();
  setup_sd();
  setup_server();
}

void loop() {
  web.process();
}

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

void setup_g35() {
  int i = 0;
  while (i < strandCount) {
    pinMode(strandPin[i], OUTPUT);
    digitalWrite(strandPin[i], LOW);
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

void loop_g35() {
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

