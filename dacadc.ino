// DAC out (A14  A12 LC) to ADC A0 in   pin 12 for hi/lo
#define VREF 3.3

void setup() {
  Serial.begin(9600);
  analogWriteResolution(12);
  analogReadResolution(16);
  analogReadAveraging(32);
  pinMode(12,INPUT);  // voltage threshold
}
void loop() {
  int i, sensorValue,k;
  float v;
  char str[80];

  for (i=0; i< 4096; i+=40) {
	analogWrite(A14,i);
	//  ? should we wait for DAC to settle 2-15us, datasheet
	delayMicroseconds(10);
	sensorValue = analogRead(A0);
    v= VREF * sensorValue/65536.;
	k = sensorValue>>4;
    sprintf(str,"%d  %d %d   %d ",i,k,sensorValue,digitalRead(12));
	Serial.print(str);
	Serial.println(v,3);
    delay(1000);
  }
  for (i=4096-40; i >= 0; i-=40) {
	analogWrite(A14,i);
	delayMicroseconds(10);
	sensorValue = analogRead(A0);
    v= VREF * sensorValue/65536.;
	k = sensorValue>>4;
    sprintf(str,"%d  %d %d   %d ",i,k,sensorValue,digitalRead(12));
	Serial.print(str);
	Serial.println(v,3);
    delay(1000);
  }
}

