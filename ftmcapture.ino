// teensy ftm capture   pin 20
// http://shawnhymel.com/681/learning-the-teensy-lc-input-capture/
#include "Adafruit_Si7021.h"

Adafruit_Si7021 sensor = Adafruit_Si7021();

volatile uint32_t count = 0;
volatile uint32_t prev_val = 0;
volatile uint32_t ovf_count = 0;
volatile uint8_t ic_flag = 0;
float gap;

void setup() {

  // Set up our Serial port
  Serial.begin(9600);
  while (!Serial);
  sensor.begin();
  //tone(2, 440, 0);   // test  jumper pin 2 to pin 20

  // The order of setting the TPMx_SC, TPMx_CNT, and TPMx_MOD
  // seems to matter. You must clear _SC, set _CNT to 0, set _MOD
  // to the desired value, then you can set the bit fields in _SC.

  // Clear FTM0_SC register (p. 572)
  FTM0_SC = 0;

  // Reset the FTM0_CNT counter (p. 574)
  FTM0_CNT = 0;

  // Set overflow value (modulo) (p. 574)
  FTM0_MOD = 0xFFFF;

  // Set FTM0_SC register (p. 572)
  // Bits | Va1ue | Description
  //  8   |    0  | DMA: Disable DMA
  //  7   |    1  | TOF: Clear Timer Overflow Flag
  //  6   |    1  | TOIE: Enable Timer Overflow Interrupt
  //  5   |    0  | CPWMS: TPM in up counting mode
  // 4-3  |   01  | CMOD: Counter incrememnts every TPM clock
  // 2-0  |  000  | PS: Prescale = 1
  FTM0_SC = 0b011001000;


  // Set TPM0_C5SC register (Teensy LC - pin 20) (p. 575)
  // As per the note on p. 575, we must disable the channel
  // first before switching channel modes. We also introduce
  // a magical 1 us delay to allow the new value to take.
  // Bits | Va1ue | Description
  //  7   |    0  | CHF: Do nothing
  //  6   |    1  | CHIE: Enable Channel Interrupt
  // 5-4  |   00  | MS: Input capture
  // 3-2  |   01  | ELS: Capture on rising  edge
  //  1   |    0  | Reserved
  //  0   |    0  | DMA: Disable DMA
  FTM0_C5SC = 0;
  delayMicroseconds(1);
  FTM0_C5SC = 0b01000100;

  // Set PORTD_PCR5 register (Teensy 3.2 - pin 20) (p. 199)
  // Bits | Value | Description
  // 10-8 |   100 | MUX: Alt 4 attach to FTM0_CH5 (p. 179)
  //  7   |     0 | Reserved
  //  6   |     0 | DSE: Low drive strength
  //  5   |     0 | Reserved
  //  4   |     0 | PFE: Disable input filter
  //  3   |     0 | Reserved
  //  2   |     0 | SRE: Fast slew rate if output
  //  1   |     0 | PE: Enable pull-up/down
  //  0   |     0 | PS: Internal pull-up
  //  PORTD_PCR5 = 0b10000000000;
  *portConfigRegister(20) = PORT_PCR_MUX(4);
  NVIC_SET_PRIORITY(IRQ_FTM0, 64);

  // Enable the interrupt vector. In this case, we want to execute
  // the ISR (named "ftm0_isr()" for Teensy) every time TPM0
  // overflows. We set bit 17 of &E000_E100.
  NVIC_ENABLE_IRQ(IRQ_FTM0);
  // Same as: NVIC_ISER0 |= (1 << 17);
}

void loop() {

  // If an input capture event has occurred, print the time
  // between edges
  if ( ic_flag ) {

    // Time between edges
    gap = count / (F_BUS / 1000000.);  // us
    float ppm = (int)(count - F_BUS) / (F_BUS / 1000000.);
    Serial.printf("%.3f ppm %.2f C ", ppm, sensor.readTemperature());
    Serial.print(count);
    Serial.print(" ticks   period: ");
    Serial.print(gap);
    Serial.println(" us");

    // Reset input capture flag
    ic_flag = 0;
  }
}

// "ftm0_isr" is an interrupt vector defined for the Teensy
void ftm0_isr(void) {
  bool inc = false;
  uint32_t val;
  if ( FTM0_SC & (1 << 7) ) {
    // FTM0_SC |= (1 << 7);    // LC
    FTM0_SC &= ~(1 << 7);   // T3.2
    ovf_count++;
    inc = true;
  }

  // If we got here from the input capture, we want to clear the
  // channel flag.
  if ( FTM0_C5SC & (1 << 7) ) {
    //  FTM0_C5SC |= (1 << 7);    // LC
    FTM0_C5SC &= ~(1 << 7);    // T3.2

    // We should only continue if we have handled the previous
    // input capture
    if ( !ic_flag ) {

      // Retrieve the counter value and compare it to the last one.
      // Take into account any overflows that have occured.
      val = FTM0_C5V;
      if ( val <= 0xE000 || !inc) val |= ovf_count << 16;
      else val |= (ovf_count - 1) << 16;
      count = val - prev_val;

      // Save the counter value and reset the overflow counter
      prev_val = val;
      //  ovf_count = 0;

      // Set a flag so that we can read the count in the main loop
      ic_flag = 1;
    }
  }
}
