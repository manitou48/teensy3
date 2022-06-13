// comparator latency test COMP1  T3.2
// test with 1khz PWM from pin 22
// pin 23 comparator input COMP1_IN0
// jumper 22 to 23
// pin 10 comparator output COMP1_OUT   PTC4
// internal DAC for 1.5v threshold
// scope to pin 22 and pin 10

void ComparatorSetup(int threshold) { // threshold 0..63
  // Output is on Teensy pin 10 (TX2), chip pin 49; port C4
  // Input is pin A9 (Teensy pin 23, chip pin 45; port C2)
  // Reference is 6-bit internal DAC
  // DAC reference VIN1 = VREF (1.19 V), VIN2 = VDD (3.3)

  SIM_SCGC4 |= SIM_SCGC4_CMP; //Clock to Comparator
  CORE_PIN10_CONFIG = PORT_PCR_MUX(6); //Alternate function 6: Teensy pin10 = CMP1_OUT, PTC4


  //CMP1_MUXCR = 0b01000111; // Input pins select; plus = IN0 (pin 23), neg = DAC (code 7)
  CMP1_MUXCR = CMP_MUXCR_PSTM | CMP_MUXCR_PSEL(0) | CMP_MUXCR_MSEL(7);

  //CMP1_CR0 = 0b00000000; // FILTER_CNT=0; HYSTCTR=0
  //CMP1_CR0 = 0b01110011; // FILTER_CNT=7; HYSTCTR=11 (max)
  CMP1_CR0 = CMP_CR0_FILTER_CNT(7) | CMP_CR0_HYSTCTR(3);
  // hysteresis is higher if VIN common-mode = 0 V

  //CMP1_FPR = 0xff;

  //CMP1_CR1 = 0b00010111; // SE=0, high power, COUTA, output pin, enable; mode #2A
  //CMP1_CR1 = 0b00000111; // SE=0, low power, COUTA, output pin, enable; mode #2A; low power == slow
  CMP1_CR1 =  CMP_CR1_COS | CMP_CR1_OPE | CMP_CR1_EN;
  // read CMP1_SCR LSB is analog comparator output state

  //CMP1_DACCR = 0b11000000 + threshold; // enable DAC, VIN2 selected (=VDD), reference = threshold*VDD/64 -- 19 suits X72
  CMP1_DACCR = CMP_DACCR_DACEN | CMP_DACCR_VRSEL | CMP_DACCR_VOSEL(threshold) ;
  if (threshold == 0) CMP1_DACCR = 0; //Disable DAC ==> reference = 0 V
}

void setup() {
  analogWriteFrequency(22, 1000);
  analogWrite(22, 128);
  ComparatorSetup(32); // about 1.5 V

}

void loop() {

}
