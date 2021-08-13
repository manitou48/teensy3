//capture pin  https://www.pjrc.com/teensy/td_libs_FreqMeasure.html

#include <FreqMeasure.h>

static double ticks = 0, freq = FreqMeasure.countToFrequency(1);
static uint32_t cnt;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  FreqMeasure.begin();
  Serial.print(freq); Serial.println(" Hz");
}

void loop() {
  if (FreqMeasure.available()) {
    ticks =  FreqMeasure.read();
    cnt++;
    Serial.print(cnt); Serial.print(" ");
    Serial.print(ticks); Serial.print(" ");
    Serial.print((ticks - freq) / (freq / 1000000)); Serial.println("  ppm");
  }
}
