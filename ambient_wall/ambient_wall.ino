// -*- c++ -*-

#include <G35.h>

G35 g35;
uint8_t r, g, b;
uint8_t i = 204;

int8_t r_direction = -1;
int8_t g_direction = 1;
int8_t b_direction = 1;
int8_t i_direction = -1;

int maxBright = 204;

int8_t change_counter = 0;
uint8_t state = 0;

void setup() {
  g35.setup();
  
  randomSeed(analogRead(0));
  r = random(16);
  g = random(16);
  b = random(16);
}

void loop() {
  if (state == 0) {
    roll(r, r_direction, 15, 1);
    roll(g, g_direction, 15, 1);
    roll(b, b_direction, 15, 1);
    
    g35.clear(r, g, b, i);
  }
  
  if (change_counter == 99) {
    r_direction = r_direction * -1;
    g_direction = g_direction * -1;
    b_direction = b_direction * -1;
  }
  clamp(change_counter, 100, 1);
}

void roll(uint8_t &value, int8_t &direction, uint8_t max, uint8_t step) {
  value = value + (direction * step);
  if (value >= max) {
    direction = -1;
  }
  else if (value <= 0) {
    direction = 1;
  }
}

void clamp(int8_t &value, int8_t max, int8_t step) {
  value = value + step;
  if (value >= max) {
    value = 0;
  }
}

