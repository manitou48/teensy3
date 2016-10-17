//  combine RTC TPR with seconds
// https://community.nxp.com/thread/378715
// double reads insure consistent
uint32_t rtc_ms()
{
    uint32_t read1, read2,secs,us;
 
    do{
        read1 = RTC_TSR;
        read2 = RTC_TSR;
    }while( read1 != read2);       //insure the same read twice to avoid 'glitches'
    secs = read1;
    do{
        read1 = RTC_TPR;
        read2 = RTC_TPR;
    }while( read1 != read2);       //insure the same read twice to avoid 'glitches'
//Scale 32.768KHz to microseconds, but do the math within 32bits by leaving out 2^6
// 30.51758us per TPR count
    us = (read1*(1000000UL/64)+16384/64)/(32768/64);    //Round crystal counts to microseconds
    if( us < 100 ) //if prescaler just rolled over from zero, might have just incremented seconds -- refetch
    {
        do{
            read1 = RTC_TSR;
            read2 = RTC_TSR;
        }while( read1 != read2);       //insure the same read twice to avoid 'glitches'
        secs = read1;
    }
	return(secs*1000 + us/1000);   // ms
}

void setup() {
	Serial.begin(9600);
	while(!Serial);
	uint32_t a,b;

	a= RTC_TPR;
	delay(100);
	b= RTC_TPR;
	Serial.print(a); Serial.print(" "); Serial.println(b);
}

void loop() {
	Serial.println(rtc_ms());
	delay(2345);
}
