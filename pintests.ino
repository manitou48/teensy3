// exericse pins
// input  INPUT_PULLUP    1k-grnd wire
// output  OUTPUT    1k-led-grnd
// analog   pot > 1 volt    or 1k-3.3v
// PWM   uno 3 5 6 9 10 11  leo  13    use ISR 
// ISR teensy pin number   use a PWM pin to gen pulses, ?conflict
// capacitve touch for teensy

enum pin_test_t {DIGITAL_IN, DIGITAL_OUT, ANALOG_IN, ANALOG_INA, PWM_OUT, TOUCH_IN, DO_ISR};
int pintest = ANALOG_INA;

#if defined(__MKL26Z64__)
// teensy LC
//  teensylc 27 14-26
#define PINS 27
pins
uint8_t adc_pins[] = {A0,A1,A2,A3,A4,A5,A6,A7,A8, A9,A10,A11,
    /* DAC */ A12};

// see PWM with ISR
uint8_t pwm_pins[] = {3,4,6,9,10,16,17,20,22,23}; // tlc

// ISR
uint8_t isr_pins[] = {2,3,4,5,6,7,8,9,10,11,12,13,14,15,20,21,22,23}; // tlc

uint8_t touch_pins[] = {0,1,3,4,15,16,17,18,19,22,23}; // tlc


#elif defined(__MK20DX256__)
  // teensy 3.1/3.2
#define PINS 34
uint8_t adc_pins[] = {A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,A11,A12,A13,
                      /*DAC */ A14 };
// see PWM with ISR
uint8_t pwm_pins[] = {3,4,5,6,8,9,20,21,22,23,25,32};
// ISR
uint8_t isr_pins[] = {2,3,4,5,6,7,8,9,10,11,12,13,14,15,20,21,22,23}; 

uint8_t touch_pins[] = {0,1,14,15,16,17,18,19,22,23,25,29,30,32,33};

#elif defined(__MK66FX1M0__)
  // K66
#define PINS 40
uint8_t adc_pins[] = {A0,A1,A2,A3,A4,A5,A6,A7,A8, A9, A10,
                      A11,A12,A13,A14,A15,A16,A17,A18,A19,A20,
					  /*DAC0 1 */ A21,A22 };
// see PWM with ISR
uint8_t pwm_pins[] = {2,3,4,5,6,7,8,9,10,14,20,21,22,23,29,30,35,36,37,38};    // 20
// ISR
uint8_t isr_pins[] = {2,3,4,5,6,7,8,9,10,11,12,13,14,15,20,21,22,23}; // 0-39

uint8_t touch_pins[] = {0,1,15,16,17,18,19,22,23,29,30}; 

#else
// pins for UNO and such
// UNO leo 20  14-19 
#define PINS 20
uint8_t adc_pins[] = {A0,A1,A2,A3,A4,A5};
uint8_t pwm_pins[] = {3,5,6,9,10,11}; // uno

uint8_t isr_pins[] = {0,1}; // uno pins 2 3
#endif

volatile unsigned long ticks;
unsigned long prev;

void pinISR() { ticks++; }

void setup() {
	int i;

	Serial.begin(9600);
	switch (pintest) {
		case DIGITAL_IN:
			for (i=0;i<PINS;i++) pinMode(i,INPUT_PULLUP);
			break;
		case DIGITAL_OUT:
			for (i=0;i<PINS;i++) {
				pinMode(i,OUTPUT);
				digitalWrite(i,HIGH);
			}
			break;
		case PWM_OUT:
			for (i=0;i<sizeof(pwm_pins);i++) analogWrite(pwm_pins[i],128);
#if  defined(__arm__) && defined(CORE_TEENSY)
				pinMode(isr_pins[0],INPUT);   // teensy
#endif
				attachInterrupt(isr_pins[0],pinISR,RISING); 
			break;
		case DO_ISR:
			for (i=0;i<sizeof(isr_pins);i++) {
#if  defined(__arm__) && defined(CORE_TEENSY)
				pinMode(isr_pins[i],INPUT);   // teensy
#endif
				attachInterrupt(isr_pins[i],pinISR,RISING); 
		     }
#ifdef __MKL26Z64__
			analogWrite(16,128);   // pulse generator, LC use 16
#else
			analogWrite(pwm_pins[1],128);   // pulse generator, LC use 16
#endif
			break;
		default:
			break;
	}
}

void loop() {
	int i;
	unsigned long delta;

	switch (pintest) {
		case DIGITAL_IN:
			for (i=0;i<PINS;i++) {
				if (digitalRead(i) == 0) {
					Serial.print("low "); Serial.println(i);
				}
			}
			break;
		case ANALOG_IN:
			for (i=0;i<sizeof(adc_pins);i++) {
				if (analogRead(adc_pins[i]) > 600) {
					Serial.print(i);
					Serial.print(" ADC  "); 
					Serial.println(analogRead(adc_pins[i]));
				}
			}
			break;
     case ANALOG_INA:
      for (i=0;i<sizeof(adc_pins);i++) {
          Serial.print(i);
          Serial.print(" ADC  "); 
          Serial.println(analogRead(adc_pins[i]));
      }
      break;
		case TOUCH_IN:
#if  defined(__arm__) && defined(CORE_TEENSY)
			for (i=0;i<sizeof(touch_pins);i++) {
			  if (touchRead(touch_pins[i]) > 1000) {
				Serial.print(touch_pins[i]);
				Serial.print(" touch ");
				Serial.println(touchRead(touch_pins[i]));
			  }
			}
#endif
			break;
		case PWM_OUT:
		case DO_ISR:
			delta = ticks - prev;
			if (delta) {
				Serial.print(delta);
				Serial.print(" ISR ");
				Serial.println(ticks);
				prev = ticks;
			}
			break;

		default:
			break;
	}
	delay(3000);
}
