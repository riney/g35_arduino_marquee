#ifndef G35_h
#define G35_h
#include "Arduino.h"

class G35 {
	public:
		G35();
		~G35();
		void setup();
		void clear(int r, int g, int b, int i);
		void set(int x, int y, int r, int g, int b, int i);
		void setBrightness(byte brightness);
		void draw();
	private:
		void addressMatrix();
		void sendBulbPacket(byte address, int pin, byte r, byte g, byte b, byte i);
		void fastDigitalWrite(uint8_t pin);
};
#endif