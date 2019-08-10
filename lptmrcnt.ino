// teensy adclptmr  use LPTMR0 counter (pin 13)
//  version 1 count with rollover like FreqCount but with ISR
// jumper PWM 23 to pin 13 for clock source
// https://forum.pjrc.com/threads/40782-LPTMR-on-the-Teensy-3-1-3-2-3-5-3-6
#define PRREG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)

uint32_t prev;

volatile uint32_t msw;

void lptmr_isr(void)
{
  LPTMR0_CSR |=  LPTMR_CSR_TCF;    // clear, first thing else ticks doubles?
  msw++;
}


void lptmr_init() {

  CORE_PIN13_CONFIG = PORT_PCR_MUX(3);   // pin 13 alt for LPTMR

  SIM_SCGC5  |= SIM_SCGC5_LPTIMER;      // Enable Low Power Timer Access
  LPTMR0_CSR  = 0;                     // Disable
  LPTMR0_PSR  = LPTMR_PSR_PBYP;      // Bypass prescaler/filter
  LPTMR0_CMR  = 0xffff;        // counts til interrupt
  LPTMR0_CSR  = LPTMR_CSR_TIE |           // Interrupt Enable
                LPTMR_CSR_TPS(2) |        // Pin: 0=CMP0, 1=xtal, 2=pin13
                LPTMR_CSR_TMS;            // Mode Select, 0=timer, 1=counter
  LPTMR0_CSR |= LPTMR_CSR_TEN;           // Enable
  NVIC_ENABLE_IRQ(IRQ_LPTMR);
}

IntervalTimer it1;

volatile uint32_t dataReady, ticks;

void it1cb() {
  LPTMR0_CNR = 1; // dummy prime
  ticks = LPTMR0_CNR | (msw << 16);   // ? need atomic
  dataReady = 1;
}

void setup() {
  Serial.begin(9600);
  while (!Serial);
  delay(2000);

  analogWriteFrequency(23, 30000000);
  analogWrite(23, 128);
  it1.begin(it1cb, 1000000);   // every second
  lptmr_init();
  PRREG(LPTMR0_CSR);
  PRREG(LPTMR0_CMR);
}

void loop() {
  while (1) { // avoid yield jitter
    if (dataReady) {
      Serial.println(ticks - prev);
      prev = ticks;
      dataReady = 0;
    }
  }
}
