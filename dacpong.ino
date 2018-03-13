// DAC DMA pong PDB   ping pong DMA buffers   
// hardware/teensy/avr/libraries/Audio/output_dac.cpp
// need more code for LC

#define PRREG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)

#include <DMAChannel.h>

#define PDB_CONFIG (PDB_SC_TRGSEL(15) | PDB_SC_PDBEN | PDB_SC_CONT | PDB_SC_PDBIE | PDB_SC_DMAEN)
// PDB rate   44.117 khz
#if F_BUS == 60000000
#define PDB_PERIOD (1360-1)
#elif F_BUS == 48000000
#define PDB_PERIOD (1088-1)
#endif
#define SAMPLES 128
//  process time 128/44.1k   2.9 ms

#define RAW_SIZE 4*SAMPLES
uint16_t raw_data[RAW_SIZE];
uint32_t raw_idx;

void next_sample(uint16_t *dest) {
  memcpy(dest, &raw_data[raw_idx], SAMPLES * 2);
  raw_idx += SAMPLES;
  if (raw_idx >= RAW_SIZE) raw_idx = 0;
}

void sine_fill(uint16_t *dest, uint32_t n, uint32_t v) {
  // sine table v is volume, max 2048 for 12-bit DAC
  for (uint32_t i = 0; i < n; i ++) {
    dest[i] = (int16_t)((v - 1) * sinf(6.2831853 * i / n)) + v;
  }
}

DMAMEM static uint16_t dac_buffer[SAMPLES * 2];

DMAChannel dma(false);

void isr(void)
{
  uint16_t *dest;
  uint32_t saddr;

  saddr = (uint32_t)(dma.TCD->SADDR);
  dma.clearInterrupt();
  if (saddr < (uint32_t)dac_buffer + sizeof(dac_buffer) / 2) {
    // DMA is transmitting the first half of the buffer
    // so we must fill the second half
    dest = &dac_buffer[SAMPLES];
  } else {
    // DMA is transmitting the second half of the buffer
    // so we must fill the first half
    dest = dac_buffer;
  }
  next_sample(dest);
}


void setup() {
  // fill raw_data   can vary volume v (max 2048) or frequency SAMPLES/2
  sine_fill(&raw_data[0], SAMPLES / 2, 2048);
  sine_fill(&raw_data[SAMPLES / 2], SAMPLES / 2, 2048);
  sine_fill(&raw_data[SAMPLES], SAMPLES, 2048);
  sine_fill(&raw_data[2 * SAMPLES], SAMPLES, 2048);
  sine_fill(&raw_data[3 * SAMPLES], SAMPLES, 2048);

  //pre fill DAC buffer
  next_sample(&dac_buffer[0]);
  next_sample(&dac_buffer[SAMPLES]);

  dma.begin(true); // Allocate the DMA channel first

  SIM_SCGC2 |= SIM_SCGC2_DAC0;
  DAC0_C0 = DAC_C0_DACEN;                   // 1.2V VDDA is DACREF_2
  // slowly ramp up to DC voltage, approx 1/4 second
  for (int16_t i = 0; i <= 2048; i += 8) {
    *(int16_t *)&(DAC0_DAT0L) = i;
    delay(1);
  }

  // set the programmable delay block to trigger DMA requests
  SIM_SCGC6 |= SIM_SCGC6_PDB; // enable PDB clock
  PDB0_IDLY = 0; // interrupt delay register
  PDB0_MOD = PDB_PERIOD; // modulus register, sets period
  PDB0_SC = PDB_CONFIG | PDB_SC_LDOK; // load registers from buffers
  PDB0_SC = PDB_CONFIG | PDB_SC_SWTRIG; // reset and restart
  PDB0_CH0C1 = 0x0101; // channel n control register?

  dma.TCD->SADDR = dac_buffer;
  dma.TCD->SOFF = 2;
  dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
  dma.TCD->NBYTES_MLNO = 2;
  dma.TCD->SLAST = -sizeof(dac_buffer);
  dma.TCD->DADDR = &DAC0_DAT0L;
  dma.TCD->DOFF = 0;
  dma.TCD->CITER_ELINKNO = sizeof(dac_buffer) / 2;
  dma.TCD->DLASTSGA = 0;
  dma.TCD->BITER_ELINKNO = sizeof(dac_buffer) / 2;
  dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
  dma.triggerAtHardwareEvent(DMAMUX_SOURCE_PDB);
  dma.enable();
  dma.attachInterrupt(isr);
}

void loop() {

}
