// -*- c++ -*-

#include <G35.h>

G35 g35;

int maxBright = 204;

void setup() {
  g35.setup();
  Serial.begin(115200);
  g35.clear(13, 13, 13, 100);
  randomSeed(analogRead(0));
}

void loop() {
  if (random(2)) {
    g35.set(random(18), random(10), 13, 13, 13, maxBright);
  }
  else {
    g35.set(random(18), random(10), 13, 13, 13, 10);
  }
}

