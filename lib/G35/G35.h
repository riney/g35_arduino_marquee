#ifndef G35_h
#define G35_h
#include "Arduino.h"

class G35 {
	public:
		G35();
		~G35();
		void setup();
		void clear(uint8_t r, uint8_t g, uint8_t b, uint8_t i);
		void set(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t i);
		void setBrightness(uint8_t brightness);
		void draw();

	private:
		void addressMatrix();
		void sendBulbPacket(uint8_t address, uint8_t pin, uint8_t r, uint8_t g, uint8_t b, uint8_t i);
		void fastDigitalWrite(uint8_t pin);
};
#endif