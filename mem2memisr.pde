/**********************************************************************
 teensy  only 16KB RAM
 use DMA for memcpy memset  ch 21  pg 387
 thanks to Paul
  ISR version
 */

static int t1,t2,t3;
#define WORDS 1000
int src[WORDS],dst[WORDS], xx[WORDS];
volatile int DMAdone=0;
#define DMA_CINT_CINT(n) ((uint8_t)(n & 3)<<0) // Clear Interrupt Request


void dma_ch1_isr(void)
{
  t3= micros();
  DMAdone=1;
  DMA_CINT = DMA_CINT_CINT(1); // use the Clear Intr. Request register
}


void memcpy32(int *dest, const int *src, unsigned int count)
{
        DMA_TCD1_SADDR = src;
        DMA_TCD1_SOFF = 4;
        DMA_TCD1_ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2); //32bit
        DMA_TCD1_NBYTES_MLNO = count * 4;
        DMA_TCD1_SLAST = 0;
        DMA_TCD1_DADDR = dest;
        DMA_TCD1_DOFF = 4;
        DMA_TCD1_CITER_ELINKNO = 1;
        DMA_TCD1_DLASTSGA = 0;
        DMA_TCD1_BITER_ELINKNO = 1;
		DMAdone=0;
		 DMA_TCD1_CSR = DMA_TCD_CSR_INTMAJOR; // interrupt on major loop completion
        DMA_TCD1_CSR |= DMA_TCD_CSR_START;
//        while (!(DMA_TCD1_CSR & DMA_TCD_CSR_DONE)) /* wait */ ;
	//	while(!DMAdone);
}

void memset32(int *dest, int val, unsigned int count)
{
        DMA_TCD1_SADDR = &val;
        DMA_TCD1_SOFF = 0;
        DMA_TCD1_ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);
        DMA_TCD1_NBYTES_MLNO = count * 4;
        DMA_TCD1_SLAST = 0;
        DMA_TCD1_DADDR = dest;
        DMA_TCD1_DOFF = 4;
        DMA_TCD1_CITER_ELINKNO = 1;
        DMA_TCD1_DLASTSGA = 0;
        DMA_TCD1_BITER_ELINKNO = 1;
		DMAdone=0;
		 DMA_TCD1_CSR = DMA_TCD_CSR_INTMAJOR; // interrupt on major loop completion
        DMA_TCD1_CSR |= DMA_TCD_CSR_START;
//        while (!(DMA_TCD1_CSR & DMA_TCD_CSR_DONE)) /* wait */ ;
		while(!DMAdone);
}



void setup(){
	int i;
	Serial.begin(9600);
	while(!Serial);
	NVIC_ENABLE_IRQ(IRQ_DMA_CH1);
}

void loop(){
	int i,t4;
	
	for (i=0;i<WORDS;i++){
		dst[i]=0;
		src[i]=i;
	}

  t1=micros();
	memcpy32(dst,src,WORDS);
   t2=micros();
      memcpy(xx,src,4*WORDS);
      t4 = micros();
      
      Serial.print(t2); Serial.print(" "); Serial.print(t3); Serial.print(" "); Serial.println(t4);
      t2= t4-t2;
      Serial.println(t2);
      t3=t3-t1;
      Serial.println(t3);
      t4=t4-t1;
      Serial.println(t4);
      delay(3000);
      return;
	Serial.println(dst[3],DEC);
        memset32(dst,45,WORDS);
        Serial.println(dst[3],DEC);
        t1=micros();
        memcpy32(dst,src,WORDS);
        t2 = micros() - t1;
        Serial.print("memcpy32 ");Serial.println(t2,DEC);
        
        t1=micros();
        memset32(dst,66,WORDS);
        t2 = micros() - t1;
        Serial.print("memset32 ");Serial.println(t2,DEC);
        
        t1=micros();
        for(i=0;i<WORDS;i++) dst[i] = src[i];
        t2 = micros() - t1;
        Serial.print("loop ");Serial.println(t2,DEC);
        dst[3]=99;
        t1=micros();
        memcpy(dst,src,4*WORDS);
        t2 = micros() - t1;
        Serial.print("memcpy ");Serial.println(t2,DEC);
        Serial.println(dst[3],DEC);
        t1=micros();
        memset(dst,66,4*WORDS);
        t2 = micros() - t1;
        Serial.print("memset ");Serial.println(t2,DEC);
        Serial.println(dst[3],HEX);
        t1=micros();
        for(i=0;i<WORDS;i++) dst[i] = 66;
        t2 = micros() - t1;
        Serial.print("set loop ");Serial.println(t2,DEC);
        Serial.println();
	delay(3000);
}
