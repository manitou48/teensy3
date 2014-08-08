void setup() {
  analogReference(EXTERNAL);
  analogReadResolution(12);
  analogReadAveraging(32);
}

void loop() {
  int mv;
  mv = 1195 * 4096 / analogRead(39);
  Serial.println(mv);
  delay(2000);
}
