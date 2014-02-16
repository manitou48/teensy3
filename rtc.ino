// teensy RTC  need 32khz crystal
// hardware/teensy/cores/teensy3/pins_teensy.c   ch 39
// after program changes affecting RTC regs, recompile, and cycle power

volatile static unsigned long ticks,us;
static unsigned long us0=0;

void rtc_seconds_isr(void) {
	if (us0 == 0) us0 = micros();
      else {
        ticks++;
        us = micros() - us0;
    }
}

void setup() {
	Serial.begin(9600);
	while(!Serial);
//	rtc_compensate(-100);
//  RTC_TCR =0x0101 ;  // compensate  1 tick is 30ppm 0x0001, 0x0101 2 seconds
//  RTC_CR = 0x3500;    // capacitance
	RTC_IER = 0x10 ;    //TSIE
	NVIC_ENABLE_IRQ(IRQ_RTC_SECOND);
	Serial.println(RTC_SR,HEX);
	Serial.println(RTC_CR,HEX);
	Serial.println(RTC_IER,HEX);
	Serial.println(RTC_TCR,HEX);
}

void loop() {
	unsigned long t;
        int ppm, tprev=0;
        char str[64];


        ppm = 1000000*ticks - us;
        ppm = 1.e6 * ppm/ (float)us;
        sprintf(str,"%d %d %f %d",ticks,ticks-tprev,us*1.e-6,ppm);
        Serial.println(str);

	t = rtc_get();
	Serial.println(t);
    Serial.println(RTC_TCR,HEX);
	delay(2000);
}

