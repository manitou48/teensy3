//  test SDHC 4-bit SDIO
// init and sector read
// TODO  FIFO  DMA  FAT  class
// based on BRTOS https://community.freescale.com/thread/85176

#include "SDHC.h"
#define LEDpin 13


#define PRREG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)

void dump(uint8_t * buff, int lines) {
    int i;

    char str[64];

    while(lines--){
        for(i=0;i<16;i++) {sprintf(str,"%02x ",buff[i]); Serial.print(str);}
    Serial.print("|");
    for(i=0;i<16;i++){
      if (buff[i] < ' ' | buff[i]  > '\177') Serial.print("?");
       else Serial.print(buff[i]);
    }
    Serial.print("|");
    Serial.println();
    buff += 16;
    }
}


void setup()
{
    uint32_t t, parm, r;
    uint8_t sector[512] __attribute__((aligned(4)));
    char str[64];

    Serial.begin(9600);
    while(!Serial);
    pinMode(LEDpin, OUTPUT);
    pinMode(12,OUTPUT);  //  testpin
    
    Serial.println();Serial.print(F_CPU); Serial.print(" ");
    Serial.print(F_BUS); Serial.print(" ");
    Serial.print(F_MEM); Serial.print(" ");
    Serial.print(__TIME__);Serial.print(" ");Serial.println(__DATE__);

    digitalWrite(LEDpin,LOW);  // toggled in driver

    PRREG(SIM_SCGC3);
    PRREG(SIM_SCGC5);
    SIM_SCGC5 |= SIM_SCGC5_PORTE;
    SIM_SCGC3 |= SIM_SCGC3_SDHC;
    PRREG(SDHC_SYSCTL);

    r=disk_initialize(0);
    Serial.print("disk init "); Serial.println(r);
    PRREG(SIM_SCGC3);
    PRREG(SIM_SCGC5);
    PRREG(SDHC_HTCAPBLT);
    PRREG(SDHC_SYSCTL);
    PRREG(SDHC_PROCTL);
    PRREG(SDHC_WML);
    parm = ESDHC_BUS_WIDTH_4BIT;   // set 4-bit width  need to set in driver too
    SDHC_ioctl(IO_IOCTL_ESDHC_SET_BUS_WIDTH, &parm);
    PRREG(SDHC_PROCTL);
    PRREG(SDHC_IRQSTAT);
    PRREG(SDHC_PRSSTAT);
    PRREG(SDHC_WML);

    disk_read(0,sector,0,1);   // warm up
    memset(sector,1,sizeof(sector));
    t = micros();
    disk_read(0,sector,16512,1);
    t=micros()-t;
    
    Serial.print(t); Serial.print(" us  "); Serial.println((8.*sizeof(sector))/t);
    sprintf(str,"sector 16512   %d %d %d",sector[0],sector[1],sector[2]);
    Serial.println(str);
    dump(sector,2);
    int errs=0;
    for (int i=0; i < sizeof(sector); i++) if (i%256 != sector[i]) errs++;
    sprintf(str,"errs %d",errs);
    Serial.println(str);
#if 0
    // multi sectors
    uint8_t sectors[2*512] __attribute__((aligned(4)));
    memset(sectors,1,sizeof(sectors));
    t = micros();
    disk_read(0,sectors,16512,2);
    t=micros()-t;
    Serial.print(t); Serial.print(" us  "); Serial.println((8.*sizeof(sector))/t);
    sprintf(str,"sector 16512   %d %d %d",sector[0],sector[1],sector[2]);
    Serial.println(str);
    dump(sectors,2);
    errs=0;
    for (int i=0; i < sizeof(sectors); i++) if (i%256 != sectors[i]) errs++;
    sprintf(str,"errs %d",errs);
    Serial.println(str);
#endif 
    PRREG(SDHC_WML);
    PRREG(SDHC_IRQSTAT);
    PRREG(SDHC_XFERTYP);
    PRREG(SDHC_BLKATTR);
    PRREG(SDHC_DSADDR);
}

void loop() 
{
   digitalWrite(LEDpin,!digitalRead(LEDpin)); // toggle led
   delay(500);
}

