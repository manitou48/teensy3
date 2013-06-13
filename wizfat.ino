/*
 wiztst  SPI   teensy  need breadoard power and common ground
  SdFat SPI version need w5100.cpp.fat
   test 1:  roundtrip latency 8 byte uechosrv
   test 2:  sink test 1000-byte 10 reps to udpsink
   test 3:  input test 1000-byte pkts  udpsrc
   test 4:   scope test
   test 5:  ttcp client
   test 6:  ttcp server
  3.3v from breadboard, grnd,reset-to-reset  (tricky) or use pin 7
  SPI1 NSS,MOSI,MISO,SCK  10-13
 */

#include <stdint.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

uint16_t fatwrite(uint16_t _addr, const uint8_t *_buf, uint16_t _len);
uint16_t fatread(uint16_t _addr, const uint8_t *_buf, uint16_t _len);

#define REPS 10
int test = 4;

static byte tdata[] = {0xa5,0x96,0x18,0x81};


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


void wizdump(){
	uint8_t wiz[1024];
        char str[128];
		double mbs;
	int t1,i;

	for(i=0;i<sizeof(wiz);i++) wiz[i] = i%256;
	t1 = micros();
	fatwrite(0x8000,wiz,1024);  // buffer memory
	t1 = micros() - t1;
    mbs = 8*1024/(float)t1;
    sprintf(str,"write %d us   %.2f mbs",t1,mbs);
    Serial.println(str);

	for(i=0;i<sizeof(wiz);i++) wiz[i] = 0;
	t1 = micros();
	fatread(0x8000,wiz,1024);  // buffer memory
	t1 = micros() - t1;
    mbs = 8*1024/(float)t1;
    sprintf(str,"read %d us   %.2f mbs",t1,mbs);
    Serial.println(str);
	t1=0;
	for(i=0;i<sizeof(wiz);i++) if(wiz[i] != i%256) t1++;
    sprintf(str,"wrt/rd errors %d",t1);
    Serial.println(str);
	
	t1 = micros();
	fatread(0,wiz,55);
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
  // start Ethernet and UDP
  Ethernet.begin(mac, ip);
  Udp.begin(localPort);
 //  ? careful  disable SPI first ?  set both TAR0 and 1 ??
 // SPI0_CTAR0 = SPI_CTAR_FMSZ(7) | SPI_CTAR_BR(9) | SPI_CTAR_PBR(1);
 // SPI0_CTAR1 = 0x78010001;
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
	fatread(0x4000,packetBuffer,32);
	hexdump(packetBuffer,32);
    break;
    
    case 4:
      buff[1]=1;
	  fatwrite(0x8000,(uint8_t *)tdata,sizeof(tdata));
	  delay(1);
	  for (i=0;i<sizeof(tdata);i++) buff[i]=i;
      fatread(0x8000,(uint8_t *)buff,4);  // scope test
      Serial.println(buff[1],HEX);
      break;

    case 5:
		ttcp_client();
		break;
	case 6:
		ttcp_server();
		fatread(0x4000,packetBuffer,32);
		hexdump(packetBuffer,32);
		break;

  }

  Serial.println(SPI0_CTAR1,HEX); // teensy
  // wait 
  delay(7000); 
}

//  ttcp  client server
//   ttcp -r -s or ttcp -l 1000 -n 100 -t -s 192.168.1.112


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
  sprintf(str,"send  %ld bytes %ld ms %.1f mbs",bytes,t1,mbs);
  Serial.println(str);
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
  sprintf(str,"recv  %ld bytes %ld ms %.1f mbs %d",bytes,t1,mbs,n);
  Serial.println(str);
  sender.flush();
  sender.stop();
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
