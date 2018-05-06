// jumper 23 to 22  scope on 23 and 12, adjust PWM rate til 12 falls behind
//  could optimize by direct ISR link

#define FASTISR

// FASTRUN
void pin_isr() {
#ifdef FASTISR
  PORTC_ISFR = ~0;  //clear
#endif
  GPIOC_PTOR = CORE_PIN12_BITMASK;   // toggle
}
void setup() {
  pinMode(12, OUTPUT);
  attachInterrupt(22, pin_isr, CHANGE);
#ifdef FASTISR
  attachInterruptVector(IRQ_PORTC, pin_isr);
#endif
  analogWriteFrequency(23, 2500000);
  analogWrite(23, 127);
}

void loop() {
}
