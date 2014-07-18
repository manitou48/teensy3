// low voltage interrupt  ch 15  LVW  LVD
volatile uint32_t cnt;

// low voltage ISR, flag will stay on unless voltage climbs above threshold
void low_voltage_isr(void) {
	digitalWrite(13,HIGH);
	cnt++;
	 PMC_LVDSC2 |= PMC_LVDSC2_LVWACK;  // clear if we can
        PMC_LVDSC1 |= PMC_LVDSC1_LVDACK;
}

void setup() {
  analogReference(EXTERNAL);
  analogReadResolution(12);
  analogReadAveraging(32);
  PMC_LVDSC1 =  PMC_LVDSC1_LVDV(1);  // enable hi v
  PMC_LVDSC2 = PMC_LVDSC2_LVWIE | PMC_LVDSC2_LVWV(3); // 2.92-3.08v
  NVIC_ENABLE_IRQ(IRQ_LOW_VOLTAGE);
  pinMode(13,OUTPUT);
  digitalWrite(13,LOW);
  Serial2.begin(9600); 
  Serial2.print(PMC_LVDSC1,HEX); Serial2.print(" "); Serial2.println(PMC_LVDSC2,HEX);
}

void loop() {
  int mv;
  mv = 1195 * 4096 /analogRead(39);
  Serial2.println(mv);
  Serial2.println(cnt); Serial2.println(PMC_LVDSC2,HEX);
  delay(2000);
  digitalWrite(13,LOW);
}
