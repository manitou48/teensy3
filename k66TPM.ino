// T3.6 TPM tests  pin 16 PDB0  TPM1_CH0
// default config:  16 MHz crystal clock,
// PPS to pin 23

#define PRREG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)

volatile uint16_t ticks, ding;

void ppsisr() {
  ticks = TPM1_CNT;
  ding = 1;
}

void pps() {
  analogWriteFrequency(16, 400);   // MOD 40000
  attachInterrupt(23, ppsisr, RISING);   // GPS PPS
  while (1) {
    if (ding) {
      static uint16_t prev, delta;    // 16-bit counter, count mod 40000
      int32_t d;

      ding = 0;
      //  Serial.print(ticks); Serial.print(" "); Serial.println(prev);
      if (ticks < prev) delta = 40000 + ticks - prev;  // rollover
      else delta = ticks -  prev;
      Serial.print(delta); Serial.print(" ticks ");
      if (delta < 40000) d = -(40000 - delta);
      else d = delta;
      Serial.print(.0625 * d); Serial.println(" ppm");
      prev = ticks;
    }
  }
}

void tpm1_isr() {
  TPM1_SC |= 1 << 7; // clear FTM_SC_TOF first
  ticks++;
}

void doticks() {
  analogWriteFrequency(16, 400);   // MOD 40000
  NVIC_ENABLE_IRQ(IRQ_TPM1);
  TPM1_SC |=  1 << 6; // FTM_SC_TOIE
  analogWrite(16, 128);
  PRREG(TPM1_SC);
  PRREG(TPM1_MOD);
  PRREG(TPM1_C0V);
  while (1) {
    delay(1000);
    Serial.println(ticks);
    Serial.print("TPM1_CNT "); Serial.println(TPM1_CNT);
  }
}

void dflt() {
  analogWrite(16, 128);
  PRREG(SIM_SCGC2);
  PRREG(SIM_SOPT2);
  PRREG(SIM_SOPT9);
  PRREG(TPM1_SC);
  PRREG(TPM1_MOD);
  PRREG(TPM1_C0V);
  PRREG(TPM1_C0SC);
}

void tpm2() {
  // pin 29 TPM2_CH0   default 16mhz/32768

  SIM_SCGC2 |= SIM_SCGC2_TPM2;
  SIM_SOPT2 |= SIM_SOPT2_TPMSRC(2);
  CORE_PIN29_CONFIG = PORT_PCR_MUX(6) | PORT_PCR_DSE | PORT_PCR_SRE;
  TPM2_CNT = 0;
  TPM2_MOD = 32767;
  TPM2_C0V = 0x4000;
  TPM2_C0SC = 0x28;
  TPM2_C1SC = 0x28;
  TPM2_SC = FTM_SC_CLKS(1) | FTM_SC_PS(0);
  PRREG(SIM_SCGC2);
  PRREG(SIM_SOPT2);
  PRREG(SIM_SOPT9);
  PRREG(TPM2_SC);
  PRREG(TPM2_MOD);
  PRREG(TPM2_C0V);
  PRREG(TPM2_C0SC);
}

void pwm8() {
  analogWriteFrequency(16, 8000000);   // 16 and 17 max 8mhz uses TPM T3.6 only
  analogWrite(16, 128);
  PRREG(TPM1_SC);
  PRREG(TPM1_MOD);
  PRREG(TPM1_C0V);
}

void setup() {
  Serial.begin(9600);
  while (!Serial);
  //dflt();   //  default
  //pwm8();    // 8mhz PWM
  //doticks();
  //pps();
  tpm2();
}


void loop() {

}
