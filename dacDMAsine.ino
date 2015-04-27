// dacDMA v6  test DMA out with DAC and timer and DMAChannel on LC
// v6 use ch1 DMA  and restart DMA   ascii plot values
//  DAC in DMA mode, DMAMAX source is FTM1, FTM1 at frequency
//  could do triangle, sinewave,  or faux PWM  hi low
//  test jumper a12 to a0

#include <DMAChannel.h>

#define PRREG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)

DMAChannel dma(false);

#define FREQ 1000
#define N 128
uint16_t data[N];

String s = "----------------------------------------";

void prregs() {
    PRREG(DMA_DSR_BCR0);
    PRREG(DMA_DCR0);
    PRREG(FTM1_SC);
    PRREG(FTM1_C1SC);
    PRREG(FTM1_MOD);
    PRREG(DAC0_C0);
    PRREG(DAC0_DAT0L);
    PRREG(DAC0_DAT0H);
}


void timer_init() {
	// ftm1 channel 0
	SIM_SCGC6 |= SIM_SCGC6_TPM1;
	FTM1_SC = 0; delay(1);
	FTM1_CNT = 0;
	FTM1_MOD = (F_PLL/2)/FREQ - 1;
	FTM1_C0V = 0;
	FTM1_SC =   FTM_SC_CLKS(1) | FTM_SC_PS(0) ;
//	FTM1_C1SC |= FTM_CSC_CHF;
	FTM1_C1SC |= FTM_CSC_DMA;
}

void dac_init() {
	dma.begin(true);

	SIM_SCGC6 |= SIM_SCGC6_DAC0;
	DAC0_C0 = DAC_C0_DACEN | DAC_C0_DACRFS;	

	// DMA init
	dma.sourceBuffer(data,sizeof(data));
	dma.destination((volatile uint16_t&)DAC0_DAT0L);
	dma.disableOnCompletion();
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM1_CH1);
	//prregs();
	dma.enable();
	timer_init();
}


void setup() {
	int i;
	float phase = 3.14159 * 2./N;
	

	analogWriteResolution(12);
	analogReadAveraging(8);
	Serial.begin(9600);
	while(!Serial);
	Serial.println("jumper A0 to A12");
	for (i=0;i<N/2;i++) data[i]=data[N-1-i] = 2*4096*i/N;  // sawtooth
	for (i=0;i<N;i++) data[i]= sin(i*phase) * 2000.0 + 2050.0;
	dac_init();
}

void loop() {
	int val;
	int nchars = s.length();

//	prregs();
	while(!dma.complete()){
		val = map(analogRead(A0),0,1023,0,nchars-1);
  		Serial.print(s.substring(val));
		Serial.println("o");
		delay(5);
	}
	//prregs();
	dma.clearInterrupt();
	FTM1_C1SC &= ~FTM_CSC_DMA;
	dma.sourceBuffer(data,sizeof(data));
	dma.enable();
	FTM1_C1SC |= FTM_CSC_DMA;
}
