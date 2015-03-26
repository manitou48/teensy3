// dacDMA v2  test DMA out with DAC and timer and DMAChannel
//  DAC in DMA mode, DMAMAX source is FTM1, FTM1 at frequency
// regular DMA buffer or try circular buffer (align)
//  could do triangle, sinewave,  or faux PWM  hi low
//  test jumper a12 to a0
//  could do ascii plot (see pyboard dacadc.py)
//   enable interrupts?  clear state?  need DMA_DCR_CS?
//  see analog.c

#include <DMAChannel.h>

#define PRREG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)

DMAChannel dma(false);

#define FREQ 1000
#define N 1024
uint16_t data[N];

void prregs() {
	PRREG(DMA_DSR_BCR0);
    PRREG(DMA_DCR0);
}

volatile int DMAdone;

void dma_isr() {
	DMAdone =1;
	dma.clearInterrupt();
}

volatile unsigned long ticks;

void ftm1_isr(void) {
	FTM1_SC |= FTM_SC_TOF;  // reset interrupt
	ticks++;
}

void timer_init() {
	// ftm1 channel 0
	NVIC_ENABLE_IRQ(IRQ_FTM1); 
	SIM_SCGC6 |= SIM_SCGC6_TPM1;
	FTM1_SC = 0;
	FTM1_CNT = 0;
	FTM1_MOD = (F_PLL/2)/FREQ - 1;
	FTM1_C0V = 0;
	FTM1_C0SC = FTM_CSC_DMA;    // DMA enable channel 0
	FTM1_SC = FTM_SC_CLKS(1) | FTM_SC_PS(0)  | FTM_SC_TOF | FTM_SC_TOIE ;
}

void dac_init() {
	dma.begin(true);

    SIM_SCGC6 |= SIM_SCGC6_DAC0;
	DAC0_C0 = DAC_C0_DACEN;
	timer_init();

	// DMA init
	dma.sourceBuffer(data,N);
	dma.destination((volatile uint8_t&)DAC0_DAT0L);
	dma.disableOnCompletion();
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM1_CH0);
	prregs();
	dma.enable();
	dma.attachInterrupt(dma_isr);
}


void setup() {
	unsigned long t1;
	int i;

	analogWriteResolution(12);
	Serial.begin(9600);
	while(!Serial);
	delay(4000);
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

	prregs();
	t1=micros();
	while(!DMAdone);
	t1 = micros() - t1;
	DMAdone=0;
	Serial.print(t1); Serial.print("us ");
	Serial.println(analogRead(A0));
	Serial.println(ticks);
	delay(3000);
}
