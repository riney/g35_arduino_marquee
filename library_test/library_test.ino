// -*- c++ -*-

/* Quick test of how to use the G35 lib. */
#include <G35.h>
#include <Flash.h>

G35 g35;

void setup() {
  g35.setup();
  Serial.begin(115200);
  g35.clear(0, 13, 0, 100);
  g35.draw();
}

void loop() {
}

