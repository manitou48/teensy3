/**********************************************************************
 teensy  only 16KB RAM
 use DMA for memcpy memset  ch 21  pg 387
 thanks to Paul
  new paul addition memcpy32p()
 */

#define BYTES 4096
uint8_t src[BYTES] __attribute__ ((aligned (16)));
uint8_t dst[BYTES] __attribute__ ((aligned (16)));


//int32_t DMAMEM _dma_Buffer_A[WORDS];
//int32_t DMAMEM _dma_Buffer_B[WORDS];

void prmbs(char *lbl,unsigned long us,int bytes) {
    float mbs = 8.*bytes/us;
    Serial.print(mbs,2); Serial.print(" mbs  ");
    Serial.print(us); Serial.print(" us   ");
    Serial.println(lbl);
}


void memcpy32(void *dest, const void *src, unsigned int bytes)
{
        DMA_TCD1_SADDR = src;
        DMA_TCD1_SOFF = 4;
        DMA_TCD1_ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2); //32bit
        DMA_TCD1_NBYTES_MLNO = bytes;
        DMA_TCD1_SLAST = 0;
        DMA_TCD1_DADDR = dest;
        DMA_TCD1_DOFF = 4;
        DMA_TCD1_CITER_ELINKNO = 1;
        DMA_TCD1_DLASTSGA = 0;
        DMA_TCD1_BITER_ELINKNO = 1;
        DMA_TCD1_CSR = DMA_TCD_CSR_START;
        while (!(DMA_TCD1_CSR & DMA_TCD_CSR_DONE)) /* wait */ ;
}

void memcpy32p(void *dest, const void *src, unsigned int bytes)
{
  // buffer needs to be aligned 16 and multiple of 16
  DMA_TCD1_SADDR = src;
  DMA_TCD1_SOFF = 16;
  DMA_TCD1_ATTR = DMA_TCD_ATTR_SSIZE(4) | DMA_TCD_ATTR_DSIZE(4);
  DMA_TCD1_NBYTES_MLNO = bytes;
  DMA_TCD1_SLAST = 0;
  DMA_TCD1_DADDR = dest;
  DMA_TCD1_DOFF = 16;
  DMA_TCD1_CITER_ELINKNO = 1;
  DMA_TCD1_DLASTSGA = 0;
  DMA_TCD1_BITER_ELINKNO = 1;
  DMA_TCD1_CSR = DMA_TCD_CSR_START;
  while (!(DMA_TCD1_CSR & DMA_TCD_CSR_DONE)) /* wait */ ;
}

void memset32(void *dest, int val, unsigned int bytes)
{
        DMA_TCD1_SADDR = &val;
        DMA_TCD1_SOFF = 0;
        DMA_TCD1_ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);
        DMA_TCD1_NBYTES_MLNO = bytes;
        DMA_TCD1_SLAST = 0;
        DMA_TCD1_DADDR = dest;
        DMA_TCD1_DOFF = 4;
        DMA_TCD1_CITER_ELINKNO = 1;
        DMA_TCD1_DLASTSGA = 0;
        DMA_TCD1_BITER_ELINKNO = 1;
        DMA_TCD1_CSR = DMA_TCD_CSR_START;
        while (!(DMA_TCD1_CSR & DMA_TCD_CSR_DONE)) /* wait */ ;
}

void gpio2mem(void *dest, void * src, unsigned int bytes)
{
        DMA_TCD1_SADDR = src;
        DMA_TCD1_SOFF = 0;
        DMA_TCD1_ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);
        DMA_TCD1_NBYTES_MLNO = bytes;
        DMA_TCD1_SLAST = 0;
        DMA_TCD1_DADDR = dest;
        DMA_TCD1_DOFF = 4;
        DMA_TCD1_CITER_ELINKNO = 1;
        DMA_TCD1_DLASTSGA = 0;
        DMA_TCD1_BITER_ELINKNO = 1;
        DMA_TCD1_CSR = DMA_TCD_CSR_START;
        while (!(DMA_TCD1_CSR & DMA_TCD_CSR_DONE)) /* wait */ ;
}

void setup(){
	Serial.begin(9600);
	while(!Serial);
}

void loop(){
	int i;
	unsigned long t1,t2;
	
	for (i=0;i<BYTES;i++){
		dst[i]=0;
		src[i]=i;
	}

	memcpy32(dst,src,BYTES);
	Serial.println(dst[3],DEC);
	for (i=0;i<BYTES;i++){
		dst[i]=0;
		src[i]=i;
	}
	memcpy32p(dst,src,BYTES);
	Serial.println(dst[3],DEC);
        memset32(dst,45,BYTES);
        Serial.println(dst[4],DEC);
        t1=micros();
        memcpy32(dst,src,BYTES);
        t2 = micros() - t1;
		prmbs("memcpy32",t2,BYTES);
        t1=micros();
        memcpy32p(dst,src,BYTES);
        t2 = micros() - t1;
		prmbs("memcpy32p",t2,BYTES);
        t1=micros();
        memset32(dst,66,BYTES);
        t2 = micros() - t1;
		prmbs("memset32",t2,BYTES);
        t1=micros();
        gpio2mem(dst,(void *)&GPIOA_PDIR,BYTES);
        t2 = micros() - t1;
   prmbs("gpio2mem",t2,BYTES);
        
        t1=micros();
        for(i=0;i<BYTES;i++) dst[i] = src[i];
        t2 = micros() - t1;
		prmbs("loop copy",t2,BYTES);
        dst[3]=99;
        t1=micros();
        memcpy(dst,src,BYTES);
        t2 = micros() - t1;
		prmbs("memcpy",t2,BYTES);
        Serial.println(dst[3],DEC);
        t1=micros();
        memset(dst,66,BYTES);
        t2 = micros() - t1;
		prmbs("memset",t2,BYTES);
        Serial.println(dst[3],HEX);
        t1=micros();
        for(i=0;i<BYTES;i++) dst[i] = 66;
        t2 = micros() - t1;
		prmbs("set loop",t2,BYTES);
        Serial.println();
	delay(3000);
}
