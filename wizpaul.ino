/*
 wizpaul  SPI   teensy 3  need breadoard power and common ground
   test 1:  roundtrip latency 8 byte uechosrv
   test 2:  sink test 1000-byte 10 reps to unpsink
   test 3:  input test 1000-byte pkts  udpsrc
  3.3v from breadboard, grnd
  SPI1 NSS,MOSI,MISO,SCK  10-13
  new Ethernet lib teensy uses pin 9 to reset, SPI FIFO, auto detect 5200
  hack w5100.cpp to set SPI clock rate  SPIFIFO.begin
 */

#include <stdint.h>
#include <SPI.h>
#include <SPIFIFO.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

#define REPS 10
int test = 4;
static uint8_t tdata[] = {0xa5,0x96,0x18,0x81};


// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
static byte mac[6] = {0x0A, 0x1B, 0x3C, 0x4D, 0x5E, 0x6F};
IPAddress ip(192,168,1, 15);

unsigned int localPort = 8888;      // local port to listen for UDP packets
unsigned int dstport = 7654;      //  dst port

IPAddress udpServer(192, 168, 1, 4); 

#define PACKET_SIZE 1024

byte packetBuffer[ PACKET_SIZE]; //buffer to hold incoming and outgoing packets 

// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

#define SS  10

uint16_t spiread(uint16_t addr, uint8_t *buf, uint16_t len)
{
  uint32_t i;
    // len = 1:  write header, write 1 byte, read
    // len = 2:  write header, write 2 byte, read
    // len = 3,5,7
    SPIFIFO.clear();
    SPIFIFO.write16(addr, SPI_CONTINUE);
    SPIFIFO.write16(len & 0x7FFF, SPI_CONTINUE);
    SPIFIFO.read();
    if (len == 1) {
        // read only 1 byte
        SPIFIFO.write(0);
        SPIFIFO.read();
        *buf = SPIFIFO.read();
    } else if (len == 2) {
        // read only 2 bytes
        SPIFIFO.write16(0);
        SPIFIFO.read();
        uint32_t val = SPIFIFO.read();
        *buf++ = val >> 8;
        *buf = val;
    } else if ((len & 1)) {
        // read 3 or more, odd length
        //Serial.print("W5200 read, len=");
        //Serial.println(len);
        uint32_t count = len / 2;
        SPIFIFO.write16(0, SPI_CONTINUE);
        SPIFIFO.read();
        do {
            if (count > 1) SPIFIFO.write16(0, SPI_CONTINUE);
            else SPIFIFO.write(0);
            uint32_t val = SPIFIFO.read();
            //TODO: WebClient_speedtest with READSIZE 7 is
            //dramatically faster with this Serial.print(),
            //and the 2 above, but not without both.  Why?!
            //Serial.println(val, HEX);
            *buf++ = val >> 8;
            *buf++ = val;
        } while (--count > 0);
        *buf = SPIFIFO.read();
        //Serial.println(*buf, HEX);
    } else {
        // read 4 or more, odd length
        //Serial.print("W5200 read, len=");
        //Serial.println(len);
        uint32_t count = len / 2 - 1;
        SPIFIFO.write16(0, SPI_CONTINUE);
        SPIFIFO.read();
        do {
            SPIFIFO.write16(0, (count > 0) ? SPI_CONTINUE : 0);
            uint32_t val = SPIFIFO.read();
            *buf++ = val >> 8;
            *buf++ = val;
        } while (--count > 0);
        uint32_t val = SPIFIFO.read();
        *buf++ = val >> 8;
        *buf++ = val;
    }
  return len;
}

uint16_t spiwrite(uint16_t addr, uint8_t *buf, uint16_t len)
{
  uint32_t i;
    SPIFIFO.clear();
    SPIFIFO.write16(addr, SPI_CONTINUE);
    SPIFIFO.write16(len | 0x8000, SPI_CONTINUE);
    for (i=0; i<len; i++) {
        SPIFIFO.write(buf[i], ((i+1<len) ? SPI_CONTINUE : 0));
        SPIFIFO.read();
    }
    SPIFIFO.read();
    SPIFIFO.read();
  return len;
}

void wizdump(){
	uint8_t wiz[1024];
        char str[128];
		double mbs;
	int t1,i;

	for(i=0;i<sizeof(wiz);i++) wiz[i] = i%256;
	t1 = micros();
	spiwrite(0x8000,wiz,1024);  // buffer memory
	t1 = micros() - t1;
    mbs = 8*1024/(float)t1;
    sprintf(str,"write %d us   %.2f mbs",t1,mbs);
    Serial.println(str);

	for(i=0;i<sizeof(wiz);i++) wiz[i] = 0;
	t1 = micros();
	spiread(0x8000,wiz,1024);  // buffer memory
	t1 = micros() - t1;
    mbs = 8*1024/(float)t1;
    sprintf(str,"read %d us   %.2f mbs",t1,mbs);
    Serial.println(str);
	t1=0;
	for(i=0;i<sizeof(wiz);i++) if(wiz[i] != i%256) t1++;
    sprintf(str,"wrt/rd errors %d",t1);
    Serial.println(str);
	
	t1 = micros();
	spiread(0,wiz,55);
	t1 = micros() - t1;
    mbs = 8*55/(float)t1;
    sprintf(str,"read %d us   %.2f mbs",t1,mbs);
    Serial.println(str);
	hexdump(wiz,55);
}

void setup() 
{

    Serial.begin(9600);
	while(!Serial);

  // optional try of reset, reset to pin 7
  pinMode(7, OUTPUT);
  digitalWrite(7, LOW);
  delayMicroseconds(100);
  digitalWrite(7, HIGH);
  delay(500);
  // thd start
  // pinMode(SS,OUTPUT);
  // SPI.begin();
  // start Ethernet and UDP
  Ethernet.begin(mac, ip);
  Udp.begin(localPort);
//  SPI.setClockDivider(SPI_CLOCK_DIV4);
 // SPI0_CTAR0 = SPI_CTAR_FMSZ(7) | SPI_CTAR_BR(9) | SPI_CTAR_PBR(1);
 //  38014004 1mhz  B8014004 2    38011001 4
 // B8011001  8mhz 38001000  12   B8010000  16  B8000000  24
 // SPI0_CTAR0 = 0xB8000000;   // disable SPI first ??
  wizdump();
}

void loop()
{
  uint32_t i,t1,t2,pkts,bytes,ipaddr;
  char buff[128];

  switch(test){
   case 1:
  	t1=micros();
  	Udp.beginPacket(udpServer, dstport);   //uechosrv
  	Udp.write(packetBuffer,8);
  	Udp.endPacket(); 

  	while (!Udp.parsePacket());  // wait to see if a reply is available
    // We've received a packet, read the data from it
    	Udp.read(packetBuffer,8);  // read the packet into the buffer
  	t2= micros()-t1;
  	Serial.println(t2,DEC);
	break;

   case 2:
  	t1=micros();
	for(i=0;i<REPS;i++){
  		Udp.beginPacket(udpServer, 2000);   // to udpsink
  		Udp.write(packetBuffer,1000);
  		Udp.endPacket(); 
	 }
  	t2= micros()-t1;
  	Serial.println(t2,DEC);
   	break;

   case 3:
    ipaddr = Ethernet.localIP();
    sprintf(buff,"%d.%d.%d.%d",ipaddr & 0xff,(ipaddr>>8)& 0xff,(ipaddr>>16)& 0xff, (ipaddr>>24)& 0xff);
    Serial.print(buff);
    sprintf(buff," port %d, hit key to stop",localPort);
    Serial.println(buff);
    pkts = bytes = 0;
    while(!Serial.available()) {
        int n = Udp.parsePacket();
        if (n){
          bytes += Udp.read(packetBuffer,1000); 
          pkts++;
       }
    }
    sprintf(buff,"%d pkts %d bytes",pkts,bytes); Serial.println(buff);
    while(Serial.available()) Serial.read();  // consume
    break;
    
    case 4:
	  for(i=0;i<sizeof(tdata);i++) buff[i]=i;
      spiwrite(0x8000,tdata,sizeof(tdata));  // scope test
	  delay(1);
      spiread(0x8000,(uint8_t *)buff,4);  // scope test
      hexdump((uint8_t *)buff,4);
//      Serial.println(SPI0_CTAR0,HEX); // teensy
      break;

  }

  // wait 
  delay(7000); 
}


void hexdump(uint8_t *p, int lth) {
    char str[16];
    int i,j,k,n;

    if (lth <= 0) return;
    sprintf(str,"    "); Serial.print(str);
    for (i=0;i<16;i++) {
        sprintf(str," %02x",i);
        Serial.print(str);
    }
    Serial.println("");
    k=i=0;
    do {
        sprintf(str,"%04x",i); Serial.print(str);
        i+=16;
        if (lth > 16) n = 16;  else n = lth;
        for (j=0;j<n;j++) {
            sprintf(str," %02x",p[k++]);
            Serial.print(str);
        }
        Serial.println("");
        lth -= n;
    } while (lth);
}

