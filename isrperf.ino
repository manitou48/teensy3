// isrperf
//  interrupt latency using cycle counter 
//  fast port write  (bit band) LED 13 *(int *)0x43fe1094 = 1;
// output 13 PTC5 LED   input 12  PTC7  jumper 12 to 13

#define FASTISR

#define ROLLOVER ((F_CPU / 1000) - 1)
volatile unsigned int ti;

void isr() {
	ti = SYST_CVR;   // or do this in pins_teensy.c  portc_isr()
#ifdef FASTISR
  PORTC_ISFR = ~0;  //clear
#endif
}

void setup() {
	unsigned int t1,t2,t3,t4,t5;
	char str[96];

	pinMode(13,OUTPUT);
    pinMode(12,INPUT);
	GPIOC_PTOR = 1<<5;   // LED toggle
	Serial.begin(9600);
	while (!Serial);
	delay(123);         // quiesce
	t1 = SYST_CVR;
	t2 = SYST_CVR;
	GPIOC_PTOR = 1<<5;
	t3 = SYST_CVR;
	digitalWrite(13,LOW);  // set bit
	t4 = SYST_CVR;
	isr();
	t5 = SYST_CVR;
	sprintf(str,"tick %d  set %d  write %d  fcn %d %d",
	      t1-t2,t2-t3,t3-t4, t4-ti,ti-t5);
	Serial.println(str);
	Serial.println("jumper 12 to 13");

	attachInterrupt(12,isr,RISING);
#ifdef FASTISR
  attachInterruptVector(IRQ_PORTC, isr);
#endif
}

void loop(){
	unsigned int t1,t3,t2,t4,t5,mint,maxt,i;
	char str[64];

	ti=0;
	t1 = SYST_CVR;
	GPIOC_PTOR = 1<<5;   // LED toggle on
	while(ti == 0);  // wait
	t3 = SYST_CVR;
	sprintf(str,"%d %d %d  isr %d %d",t1,ti,t3,t1-ti,ti-t3);  
	Serial.println(str);
	digitalWrite(13,LOW);

 	mint=99999; maxt = 0;
 	for(i=0;i<10000; i++) {
		t4 = SYST_CVR;
      	t3 = millis();
		t5 = SYST_CVR;
    	t2=t4-t5;
		if (t2 > ROLLOVER) t2 = ROLLOVER - (t5-t4);
      	if (t2> maxt) maxt=t2;
  		if (t2<mint) mint=t2;
 	}
 	Serial.print(maxt); Serial.print(" ");
 	Serial.println(mint);
	delay(3000);
}
