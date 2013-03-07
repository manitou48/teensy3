void setup() {
  analogReference(INTERNAL);
  analogReadResolution(16);
  analogReadAveraging(32);
}

void loop() {
  double f,c;
  int a;
  a = analogRead(38);
  Serial.print(a);
  //c = a*.028011 - 363;
  c = 25 - (a - 38700)/-35.7;
  Serial.print(" ");
  Serial.print(c);
  f = 1.8 * c + 32;
  Serial.print(" ");
  Serial.println(f);
  delay(1000);
}
