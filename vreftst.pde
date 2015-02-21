// compare VREF ADC channel 39 to Vcc
//  datasheet says  1.195  min/max 1.1915/1.1977 v

void setup() {
  analogReference(DEFAULT);
  analogReadResolution(12);
  analogReadAveraging(32);
}

void loop() {
  int mv;
  mv = 1195 * 4096 / analogRead(39);
  Serial.println(mv);
  delay(2000);
}
