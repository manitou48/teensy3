//  SPI0 and pauls DMAChannel
// MOSI MISO CLK CS   11 12 13  10
#include <DMAChannel.h>

#define PRREG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)

#define CS 10

#define SHORTS 5
#define BYTES SHORTS*2
uint16_t shorts[SHORTS];
uint8_t *bytes = (uint8_t *) shorts;

DMAChannel dma(false);

void spi_begin() {
	uint32_t sim4 = SIM_SCGC4;
	if (!(sim4 & SIM_SCGC4_SPI0)) SIM_SCGC4 = sim4 | SIM_SCGC4_SPI0;
	SPI0_C1 = SPI_C1_SPE | SPI_C1_MSTR;   // enable master MODEn 
	SPI0_C2 = 0;
	SPI0_BR =  SPI_BR_SPPR(2) | SPI_BR_SPR(0); // baud
	uint8_t tmp __attribute__((unused)) = SPI0_S;
	// enable pins
	CORE_PIN11_CONFIG = PORT_PCR_MUX(2);  // MOSI
	CORE_PIN12_CONFIG = PORT_PCR_MUX(2);  // MISO
	CORE_PIN13_CONFIG = PORT_PCR_MUX(2); // CLK
}

// from libraries/SPI KINETISL
void spi_transfer(void *buf, size_t count) {
        if (count == 0) return;
        uint8_t *p = (uint8_t *)buf;
        while (!(SPI0_S & SPI_S_SPTEF)) ; // wait
        SPI0_DL = *p;
        while (--count > 0) {
            uint8_t out = *(p + 1);
            while (!(SPI0_S & SPI_S_SPTEF)) ; // wait
            __disable_irq();
            SPI0_DL = out;
            while (!(SPI0_S & SPI_S_SPRF)) ; // wait
            uint8_t in = SPI0_DL;
            __enable_irq();
            *p++ = in;
        }
        while (!(SPI0_S & SPI_S_SPRF)) ; // wait
        *p = SPI0_DL;
}

void doDMA() {
	unsigned long t1;
	int i;
	double mbs;

	// DMA channel init
	dma.begin(true); // Allocate the DMA channel
	dma.disable();
	// dma.CFG->SAR =  bytes;    // could set SAR DAR DCR DSR_BCR
	dma.sourceBuffer(bytes, BYTES);
	dma.destination((volatile uint8_t&)SPI0_DL);
	dma.disableOnCompletion();
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SPI0_TX);
	SPI0_C1 = 0; // SPI reset
    SPI0_C1 = SPI_C1_SPE | SPI_C1_MSTR ;   // enable master  MODEn
	SPI0_C2 = SPI_C2_TXDMAE;  // ready
#if 0
    PRREG(SPI0_BR);
    PRREG(SPI0_C1);
    PRREG(SPI0_C2);
    PRREG(SPI0_S);
    PRREG(DMA_DSR_BCR0);
    PRREG(DMA_DCR0);
#endif


	while(1) {
		digitalWrite(CS,LOW);
		t1 = micros();
		dma.sourceBuffer(bytes, BYTES);
		dma.enable(); // go
		while(!dma.complete());
		t1 = micros() - t1;
		dma.clearComplete();
		digitalWrite(CS,HIGH);
		mbs = 8*BYTES/(float)t1;
		Serial.print(t1);   
		Serial.print(" us mbs: ");Serial.println(mbs,2);
		delay(3000);
	}
}



void setup() {
	int i;

	Serial.begin(9600);
	while(!Serial);
	delay(4000);
	Serial.println(BYTES);
	pinMode(CS,OUTPUT);
	digitalWrite(CS,HIGH);
	for (i=0;i<BYTES;i++) bytes[i]=i;
	spi_begin();
}

void loop() {
	uint32_t t1;
	double mbs;

	doDMA();   // runs forever
	digitalWrite(CS,LOW);
	t1 = micros();
//	spi_transfer(bytes,BYTES);
	t1 = micros() - t1;
	digitalWrite(CS,HIGH);
	mbs = 8*BYTES/(float)t1;
	Serial.print(t1);   
	Serial.print(" us mbs: ");Serial.println(mbs,2);
	delay(3000);
}
