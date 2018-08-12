//  UART Rx to DMA ping pong
#include <DMAChannel.h>
#define PRREG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)
#define SAMPLES 1024

DMAMEM static uint8_t rx_buffer[SAMPLES];
DMAChannel dma(false);

volatile uint8_t *dest;
volatile bool ready;

void isr(void)
{
  uint32_t daddr;

  dma.clearInterrupt();
  daddr = (uint32_t)(dma.TCD->DADDR);

  if (daddr < (uint32_t)rx_buffer + sizeof(rx_buffer) / 2) {
    // DMA is filling the first half of the buffer
    // so we must print the second half
    dest = &rx_buffer[SAMPLES / 2];
  } else {
    // DMA is filling the second half of the buffer
    // so we must print the first half
    dest = rx_buffer;
  }
  ready = true;
}

void dmainit()
{
  // set up a DMA channel to store UART byte
  // Serial4  UART3
  dma.begin(true); // Allocate the DMA channel first

  dma.TCD->SADDR = &UART3_D;
  dma.TCD->SOFF = 0;
  dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(0) | DMA_TCD_ATTR_DSIZE(0);
  dma.TCD->NBYTES_MLNO = 1;
  dma.TCD->SLAST = 0;
  dma.TCD->DADDR = rx_buffer;
  dma.TCD->DOFF = 1;
  dma.TCD->CITER_ELINKNO = sizeof(rx_buffer) ;
  dma.TCD->DLASTSGA = -sizeof(rx_buffer);
  dma.TCD->BITER_ELINKNO = sizeof(rx_buffer) ;
  dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
  dma.triggerAtHardwareEvent(DMAMUX_SOURCE_UART3_RX);
  dma.enable();
  dma.attachInterrupt(isr);

  Serial4.begin(4800);
  UART3_C5 =  UART_C5_RDMAS;  // ? enable DMA
}

void setup()
{
  Serial.begin(9600);
  while (!Serial);
  dmainit();
}

void loop()
{
  if (ready) {
    // print or write to SD
    Serial.print(millis()); Serial.println(" ms");
    Serial.println((uint32_t)dest, HEX);
    for (int i = 0; i < SAMPLES/2; i++)Serial.print((char)dest[i]);
    Serial.println();
    ready = false;
  }
}
