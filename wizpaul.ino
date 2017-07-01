/*
 wizpaul  SPI   teensy 3  need breadoard power and common ground
   test 1:  roundtrip latency 8 byte uechosrv
   test 2:  sink test 1000-byte 10 reps to udpsink
   test 3:  input test 1000-byte pkts  udpsrc
   test 4:   scope test
   test 5:  ttcp client
   test 6:  ttcp server
  3.3v from breadboard, grnd
  SPI1 NSS,MOSI,MISO,SCK  10-13
  new Ethernet lib teensy uses pin 9 to reset, SPI FIFO, auto detect 5200
  hack w5100.cpp .h to set SPI clock rate  SPIFIFO.begin
   ? sprintf %f not working
 */

#include <stdint.h>
#include <SPI.h>
#include <SPIFIFO.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

#define REPS 10
int test = 5;
static uint8_t tdata[] = {0xa5,0x96,0x18,0x81};


// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
static byte mac[6] = {0x0A, 0x1B, 0x3C, 0x4D, 0x5E, 0x6F};
IPAddress ip(192,168,1, 15);

#define NBYTES 100000
#define RECLTH 1000
#define TTCP_PORT 5001
EthernetServer server(TTCP_PORT);
EthernetClient client;


unsigned int localPort = 8888;      // local port to listen for UDP packets
unsigned int dstport = 7654;      //  dst port

IPAddress udpServer(192, 168, 1, 4); 

#define PACKET_SIZE 1024

byte packetBuffer[ PACKET_SIZE]; //buffer to hold incoming and outgoing packets 

// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

#define SS  10
#if 1
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
    sprintf(str,"write %d us mbs ",t1);
    Serial.print(str);
	Serial.println(mbs);

	for(i=0;i<sizeof(wiz);i++) wiz[i] = 0;
	t1 = micros();
	spiread(0x8000,wiz,1024);  // buffer memory
	t1 = micros() - t1;
    mbs = 8*1024/(float)t1;
    sprintf(str,"read %d us  mbs ",t1);
    Serial.print(str);
	Serial.println(mbs);
	t1=0;
	for(i=0;i<sizeof(wiz);i++) if(wiz[i] != i%256) t1++;
    sprintf(str,"wrt/rd errors %d",t1);
    Serial.println(str);
	
	t1 = micros();
	spiread(0,wiz,55);
	t1 = micros() - t1;
    mbs = 8*55/(float)t1;
    sprintf(str,"read %d us   mbs ",t1);
    Serial.print(str);
	Serial.println(mbs);
	hexdump(wiz,55);
	Serial.println("socket info");  // 0x4000  0x4100 ...
	spiread(0x4100,wiz,0x2c);
  hexdump(wiz,0x2c);
}
#endif
void setup() 
{


#if 0
       pinMode(9, OUTPUT);
       digitalWrite(9, LOW);   // reset the WIZ820io
       pinMode(10, OUTPUT);
       digitalWrite(10, HIGH);  // de-select WIZ820io
 #endif
#if 0
  // optional try of reset, reset to pin 7
  pinMode(7, OUTPUT);
  digitalWrite(7, LOW);
  delay(1);
  digitalWrite(7, HIGH);
  delay(500);
#endif
  // thd start
  // pinMode(SS,OUTPUT);
  // SPI.begin();
  // start Ethernet and UDP
  Ethernet.begin(mac, ip);
  Udp.begin(localPort);
      Serial.begin(9600);
  while(!Serial);
//  SPI.setClockDivider(SPI_CLOCK_DIV4);
 // SPI0_CTAR0 = SPI_CTAR_FMSZ(7) | SPI_CTAR_BR(9) | SPI_CTAR_PBR(1);
 //  38014004 1mhz  B8014004 2    38011001 4
 // B8011001  8mhz 38001000  12   B8010000  16  B8000000  24
 // SPI0_CTAR0 = 0xB8000000;   // disable SPI first ??
  wizdump();
Serial.print("IP  address:");
  Serial.println(Ethernet.localIP());
  Serial.println(SPI0_CTAR0,HEX);
}

void loop()
{
  uint32_t i,t1,t2,pkts,bytes,ipaddr;
  char buff[128];
  double mbs;

  switch(test){
   case 1:
     static int lost =0;  // for static ip, first pkt not sent?
  	t1=micros();
  	Udp.beginPacket(udpServer, dstport);   //uechosrv
  	Udp.write(packetBuffer,8);
  	Udp.endPacket(); 
 
  	while (!Udp.parsePacket()) {
		// wait to see if a reply is available
		if (micros() - t1 > 1000000) { 
			lost++;
			Serial.print("lost "); Serial.println(lost);
			return;
		}
	}
    // We've received a packet, read the data from it
	Udp.read(packetBuffer,8);  // read the packet into the buffer
  	t2= micros()-t1;
  	Serial.println(t2);
	break;

   case 2:
  	t1=micros();
	for(i=0;i<REPS;i++){
  		Udp.beginPacket(udpServer, 2000);   // to udpsink
  		Udp.write(packetBuffer,1000);
  		Udp.endPacket(); 
	 }
  	t2= micros()-t1;
	mbs = (8000.*REPS)/t2;
	Serial.print(mbs); Serial.print(" mbs  ");
  	Serial.println(t2);
   	break;

   case 3:
    ipaddr = Ethernet.localIP();
    sprintf(buff,"%d.%d.%d.%d",ipaddr & 0xff,(ipaddr>>8)& 0xff,(ipaddr>>16)& 0xff, (ipaddr>>24)& 0xff);
    Serial.print(buff);
  while(1) {
    sprintf(buff," port %d, hit key to stop",localPort);
    Serial.println(buff);
    pkts = bytes = 0;
    while(!Serial.available()) {
        int n = Udp.parsePacket();
        if (n){
		  if (!bytes) t1 = micros();
		  t2=micros();
          bytes += Udp.read(packetBuffer,1000); 
          pkts++;
       }
    }
	t1 = t2 -t1;
	mbs = (8.*bytes)/t1;
	Serial.print(mbs);
    sprintf(buff," mbs %d pkts %d bytes on port %d ",pkts,bytes,Udp.remotePort()); 
    Serial.print(buff);
    IPAddress remote = Udp.remoteIP();
    Serial.println(remote);
    while(Serial.available()) Serial.read();  // consume
  }
    break;
    
    case 4:
	  for(i=0;i<sizeof(tdata);i++) buff[i]=i;
  //    spiwrite(0x8000,tdata,sizeof(tdata));  // scope test
	  delay(1);
//      spiread(0x8000,(uint8_t *)buff,4);  // scope test
      hexdump((uint8_t *)buff,4);
//      Serial.println(SPI0_CTAR0,HEX); // teensy
      break;

    case 5:
        ttcp_client();
        break;
    case 6:
        ttcp_server();
        break;

  }

  // wait 
  delay(5000); 
}


void ttcp_client() {
  long t1,i,bytes=0,n,sndlth;
  float mbs;
  char str[64];

  if (!client.connect(udpServer, TTCP_PORT)) {
    Serial.println("connect failed");
    return;
  }
  t1 = millis();
  while(bytes < NBYTES) {
    sndlth = NBYTES-bytes;
    if (sndlth > RECLTH) sndlth = RECLTH;
    n = client.write(packetBuffer,sndlth);
    bytes += n;
  }
  client.stop();
  t1 = millis() - t1;
  mbs = 8*NBYTES*.001/t1;
  sprintf(str,"send  %ld bytes %ld ms  mbs ",bytes,t1);
  Serial.print(str);
  Serial.println(mbs);
}

void ttcp_server() {
    long t1,n,bytes=0;;
    char str[64];
        EthernetClient sender;
        float mbs;

    Serial.println("server listening");
    server.begin();
    while (! ( sender = server.available()) ) {}   // await connect

    t1 = millis();
    while(sender.connected()) {
      if ((n=sender.available()) > 0) {
        if (n > RECLTH)  n = RECLTH;
        sender.read(packetBuffer,n);
        bytes += n;
      }
    }
  t1 = millis() - t1;
  mbs = 8*NBYTES*.001/t1;
  sprintf(str,"recv  %ld bytes %ld ms n %d  mbs ",bytes,t1,n);
  Serial.print(str);
  Serial.println(mbs);
  sender.flush();
  sender.stop();
  //wizdump();
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

