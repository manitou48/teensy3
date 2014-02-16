// RTC alarm
// teensy RTC  need 32khz   crystal
// hardware/teensy/cores/teensy3/pins_teensy.c   ch 39
// after program changes affecting RTC regs, recompile, and cycle power

#define SECS 10

volatile static unsigned long alarms;

void rtc_alarm_isr(void) {
	alarms++;
 	NVIC_DISABLE_IRQ(IRQ_RTC_ALARM);   // avoid double interrupt
	RTC_TAR = rtc_get() + SECS;   // need to set RTC_TAR to clear interrupt
	NVIC_ENABLE_IRQ(IRQ_RTC_ALARM);
}

void setup() {
	Serial.begin(9600);
	while(!Serial);
            
	RTC_TAR = rtc_get() + SECS;
    RTC_IER = 4;    //TAIE
	NVIC_DISABLE_IRQ(IRQ_RTC_ALARM);
    NVIC_CLEAR_PENDING(IRQ_RTC_ALARM);
	NVIC_ENABLE_IRQ(IRQ_RTC_ALARM);
}

void loop() {
	unsigned long t;

	t = rtc_get();
	Serial.println(t);
	Serial.println(alarms); 
    Serial.println(RTC_SR,HEX);
	delay(2000);
}

