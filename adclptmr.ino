// teensy adclptmr  use LPTMR0 counter (pin 13) to clock ADC
// jumper PWM 23 to pin 13 for clock source
//  or use PDB timer
// https://forum.pjrc.com/threads/40782-LPTMR-on-the-Teensy-3-1-3-2-3-5-3-6
#define PRREG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)

#define Fpin13 24000
#define Fpwm 48000

volatile uint32_t ticks, aticks, adcval;

void lptmr_isr(void)
{
  ticks++;
  LPTMR0_CSR = 0b10000100;
  LPTMR0_CSR = 0b01000101;
}

void adc0_isr() {
  aticks++;
  adcval = ADC0_RA;  // read and clear
}

#define PIN_ADC                5        // A0, Pin 14

void adc_init() {
  // Initialise ADC0
  SIM_SCGC6 |= SIM_SCGC6_ADC0;
  ADC0_CFG1  = ADC_CFG1_ADIV(1) | ADC_CFG1_ADICLK(1) |                // Single-ended 12 bits, long sample time
               ADC_CFG1_MODE(1) | ADC_CFG1_ADLSMP;
  ADC0_CFG2  = ADC_CFG2_MUXSEL | ADC_CFG2_ADLSTS(2);                  // Select channels ADxxxb
  ADC0_SC2   = ADC_SC2_REFSEL(1) | ADC_SC2_ADTRG;                     // Voltage ref internal, hardware trigger
  ADC0_SC3   = ADC_SC3_AVGE | ADC_SC3_AVGS(0);                        // Enable averaging, 4 samples

  ADC0_SC3   = ADC_SC3_CAL;
  while (ADC0_SC3 & ADC_SC3_CAL)  ;                                      // Wait for calibration

  uint16_t sum0 = ADC0_CLPS + ADC0_CLP4 + ADC0_CLP3 +                 // Plus side gain
                  ADC0_CLP2 + ADC0_CLP1 + ADC0_CLP0;
  sum0 = (sum0 / 2U) | 0x8000U;
  ADC0_PG    = sum0;

  ADC0_SC1A  = ADC_SC1_AIEN | PIN_ADC;                                // Enable ADC interrupt, use A0
  NVIC_ENABLE_IRQ(IRQ_ADC0);
}

void lptmr_init() {
  // Set ADC0 to trigger from the LPTMR at 24 kHz

  SIM_SOPT7   = SIM_SOPT7_ADC0ALTTRGEN |                              // Enable ADC0 alternate trigger
                SIM_SOPT7_ADC0TRGSEL(14);                             // Trigger ADC0 by LPTMR0

  CORE_PIN13_CONFIG = PORT_PCR_MUX(3);   // pin 13 alt for LPTMR

  SIM_SCGC5  |= SIM_SCGC5_LPTIMER;                                    // Enable Low Power Timer Access
  LPTMR0_CSR  = 0;                                                    // Disable
  LPTMR0_PSR  = LPTMR_PSR_PBYP;                                       // Bypass prescaler/filter
  LPTMR0_CMR  = (Fpwm/Fpin13)-1;                              // counts til interrupt, rate = freq/(CMR+1)
  LPTMR0_CSR  = //LPTMR_CSR_TIE |                                       // Interrupt Enable
                LPTMR_CSR_TPS(2) |                                    // Pin: 0=CMP0, 1=xtal, 2=pin13
           //     LPTMR_CSR_TFC |                                       // Free-Running Counter
                LPTMR_CSR_TMS;                                        // Mode Select, 0=timer, 1=counter
  LPTMR0_CSR |= LPTMR_CSR_TEN;                                        // Enable
  //NVIC_ENABLE_IRQ(IRQ_LPTMR);
}

#define PDB_CHnC1_TOS 0x0100
#define PDB_CHnC1_EN  0x0001

void pdb_init() {
    // Setup PDB for ADC0 at 24 kHz
  SIM_SCGC6  |= SIM_SCGC6_PDB;                                        // Enable PDB clock
  PDB0_MOD    = (F_BUS / Fpin13) - 1;                                  // Timer period FIX
  PDB0_IDLY   = 0;                                                    // Interrupt delay
  PDB0_CH0C1  = PDB_CHnC1_TOS | PDB_CHnC1_EN;                         // Enable pre-trigger for ADC0
  PDB0_SC     = PDB_SC_TRGSEL(15) | PDB_SC_PDBEN |                    // SW trigger, enable interrupts, continuous mode
                PDB_SC_PDBIE | PDB_SC_CONT | PDB_SC_LDOK;             // No prescaling
  PDB0_SC    |= PDB_SC_SWTRIG;       
}

void setup() {
  Serial.begin(9600);
  while (!Serial);
  delay(2000);
  adc_init();
#if 0                  // select timer
  pdb_init();   
#else
  lptmr_init();
  PRREG(LPTMR0_CSR);
  analogWriteFrequency(23,Fpwm);
  analogWrite(23, 128);
#endif
}



void loop() {
  static uint32_t prev, aprev;
  Serial.print(adcval); Serial.print(" "); Serial.print(aticks-aprev); Serial.print(" ");
  Serial.println(aticks);
  prev = ticks;
  aprev = aticks;
  adcval=0;
 // PRREG(LPTMR0_CSR);
//  PRREG(LPTMR0_CNR);
  delay(1000);
}
