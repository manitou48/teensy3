// dacDMA v3  test DMA out with DAC and timer and DMAChannel on LC
//  DAC in DMA mode, DMAMAX source is FTM1, FTM1 at frequency
// regular DMA buffer or try circular buffer (align)
//  could do triangle, sinewave,  or faux PWM  hi low
//  test jumper a12 to a0
//  could do ascii plot (see pyboard dacadc.py)

#include <DMAChannel.h>

#define PRREG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)

DMAChannel dma(false);

#define FREQ 1000
#define N 1024
uint16_t data[N];

void prregs() {
    PRREG(DMA_DSR_BCR0);
    PRREG(DMA_DCR0);
    PRREG(FTM1_SC);
    PRREG(DAC0_C0);
    PRREG(DAC0_DAT0L);
    PRREG(DAC0_DAT0H);
}

volatile int DMAdone;

void dma_isr() {
	DMAdone =1;
	dma.clearInterrupt();
}


void timer_init() {
	// ftm1 channel 0
	SIM_SCGC6 |= SIM_SCGC6_TPM1;
	FTM1_SC = 0; delay(1);
	FTM1_CNT = 0;
	FTM1_MOD = (F_PLL/2)/FREQ - 1;
	FTM1_C0V = 0;
	FTM1_SC =  0x100 | FTM_SC_CLKS(1) | FTM_SC_PS(0)  | FTM_SC_TOF;
}

void dac_init() {
	dma.begin(true);

	SIM_SCGC6 |= SIM_SCGC6_DAC0;
	DAC0_C0 = DAC_C0_DACEN | DAC_C0_DACRFS;	

	// DMA init
	dma.sourceBuffer(data,sizeof(data));
	dma.destination((volatile uint16_t&)DAC0_DAT0L);
	dma.disableOnCompletion();
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM1_OV);
	//prregs();
	dma.enable();
	timer_init();
}


void setup() {
	unsigned long t1;
	int i;

	analogWriteResolution(12);
	analogReadAveraging(8);
	Serial.begin(9600);
	while(!Serial);
	Serial.println("jumper A0 to A12");
	for (i=0;i<N;i++) data[i]=4*i;
	t1 = micros();
	for (i=0;i<N;i++) analogWrite(A12,data[i]);
	t1 = micros() - t1;
	Serial.println(t1);
	analogRead(A0);  // warm up
	Serial.println(analogRead(A0));
	dac_init();
}

void loop() {
	unsigned long t1;

//	prregs();
	t1=micros();
		delay(500); // bout halfway
		Serial.println(analogRead(A0));
		delay(1);
		Serial.println(analogRead(A0));
	while(!dma.complete());
	t1 = micros() - t1;
	Serial.print(t1); Serial.print(" us  A0= ");
	Serial.println(analogRead(A0));
	//prregs();
	while(1);  // stop
}
