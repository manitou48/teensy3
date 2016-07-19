// low-level testing of proto ether shield on beta K66
// https://github.com/PaulStoffregen/k66_ethernet/blob/master/k66_ethernet.ino
//  summary https://forum.pjrc.com/threads/34808-K66-Beta-Test?p=109161&viewfull=1#post109161
#include "IPAddress.h"

// set this to an unused IP number for your network
IPAddress myaddress(192, 168, 1, 17);
IPAddress manitou(192,168,1,4);
uint8_t dstmac[6] = {0x00,0x13,0x20,0x2e,0x53,0x90}; // manitou mac, arp table
// could add mask and gateway ip/mac

#define MACADDR1 0x04E9E5
#define MACADDR2 0x000001

// This test program prints a *lot* of info to the Arduino Serial Monitor
// Ping response time is approx 1.3 ms with 180 MHz clock, due to all the
// time spent printing.  To get a realistic idea of ping time, you would
// need to delete or comment out all the Serial print stuff.

typedef struct {
	uint16_t length;
	uint16_t flags;
	void *buffer;
	uint32_t moreflags;
	uint16_t checksum;
	uint16_t header;
	uint32_t dmadone;
	uint32_t timestamp;
	uint32_t unused1;
	uint32_t unused2;
} enetbufferdesc_t;

#define RXSIZE 12
#define TXSIZE 10
#define BWORDS 380
static enetbufferdesc_t rx_ring[RXSIZE] __attribute__ ((aligned(16)));
static enetbufferdesc_t tx_ring[TXSIZE] __attribute__ ((aligned(16)));
uint32_t rxbufs[RXSIZE*BWORDS] __attribute__ ((aligned(16)));
uint32_t txbufs[TXSIZE*BWORDS] __attribute__ ((aligned(16)));

//#define HW_CHKSUMS     // warning: breaks if pkt > 198
//  enable HW CHKSUMS in TX ring buffer and TACC

//#define SW_UDP_CHKSUM    // otherwise chksum field 0 implies no calculate, faster

// my pkt count
uint32_t inpkts,outpkts,apkts,tpkts,upkts,ipkts, ippkts, bcast,mcast;
uint32_t ulth, us, us0, arpt0;
#define swap2 __builtin_bswap16
#define swap4 __builtin_bswap32

// prefab output frame: ether, IP, udp
uint8_t prefab[1522] __attribute__ ((aligned(16))) = { 0,0,  // pad
 0x00,0x13,0x20,0x2E,0x53,0x90, 0x00,0x13,0x20,0x2E,0x53,0x90, 0x08,0x00,
 0x45,0x00,0x00,0x24,0x3A,0x0F,0x40,0x00,0x40,0x11,0x7D,0x54,
 0xC0,0xA8,0x01,0x11,0xC0,0xA8,0x01,0x04,
 0x1D,0xE6,0xCB,0x83,0x00,0x10,0x97,0x4D,  0x09,0x00,0x00,0x00,0x67,0x45};

uint8_t UDP_buff[1522] __attribute__ ((aligned(16)));  //  udp holding area
uint32_t UDP_lth;   // non-zero when data in buff

#define PRREG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)

void prregs() {
    PRREG(MPU_RGDAAC0);
    PRREG(SIM_SCGC2);
    PRREG(SIM_SOPT2);
    PRREG(ENET_PALR);
    PRREG(ENET_PAUR);
    PRREG(ENET_EIR);
    PRREG(ENET_EIMR);
    PRREG(ENET_ECR);
    PRREG(ENET_MSCR);
    PRREG(ENET_MRBR);
    PRREG(ENET_RCR);
    PRREG(ENET_TCR);
    PRREG(ENET_TACC);
    PRREG(ENET_RACC);
    PRREG(ENET_MMFR);
}

#define ENET_EIMR_TXF                       (0x8000000U)
#define ENET_EIMR_TXB                       (0x4000000U)
#define ENET_EIMR_RXF                       (0x2000000U)
#define ENET_EIMR_RXB                       (0x1000000U)

volatile uint32_t rxcnt,txcnt;

void enet_rx_isr() {
  if (ENET_EIR & ENET_EIMR_RXF) {
   rxcnt++;
    ENET_EIR = ENET_EIMR_RXF;   // clear
  }
}

void enet_tx_isr() {
  if (ENET_EIR & ENET_EIMR_TXF){
    txcnt++;
    ENET_EIR = ENET_EIMR_TXF;   // clear
  }
}

// initialize the ethernet hardware
void setup()
{
	while (!Serial) ; // wait
	print("Ethernet Testing");
	print("----------------\n");
  Serial.println(F_CPU);
   Serial.println(myaddress);
	MPU_RGDAAC0 |= 0x007C0000;  // bus master 3 access
	SIM_SCGC2 |= SIM_SCGC2_ENET;   // enet peripheral
	CORE_PIN3_CONFIG =  PORT_PCR_MUX(4); // RXD1
	CORE_PIN4_CONFIG =  PORT_PCR_MUX(4); // RXD0
	CORE_PIN24_CONFIG = PORT_PCR_MUX(2); // REFCLK
	CORE_PIN25_CONFIG = PORT_PCR_MUX(4); // RXER
	CORE_PIN26_CONFIG = PORT_PCR_MUX(4); // RXDV
	CORE_PIN27_CONFIG = PORT_PCR_MUX(4); // TXEN
	CORE_PIN28_CONFIG = PORT_PCR_MUX(4); // TXD0
	CORE_PIN39_CONFIG = PORT_PCR_MUX(4); // TXD1
	CORE_PIN16_CONFIG = PORT_PCR_MUX(4); // MDIO
	CORE_PIN17_CONFIG = PORT_PCR_MUX(4); // MDC
	SIM_SOPT2 |= SIM_SOPT2_RMIISRC | SIM_SOPT2_TIMESRC(3);
	// ENET_EIR	1356	Interrupt Event Register
	// ENET_EIMR	1359	Interrupt Mask Register
	// ENET_RDAR	1362	Receive Descriptor Active Register
	// ENET_TDAR	1363	Transmit Descriptor Active Register
	// ENET_ECR	1363	Ethernet Control Register
	// ENET_RCR	1369	Receive Control Register
	// ENET_TCR	1372	Transmit Control Register
	// ENET_PALR/UR	1374	Physical Address
	// ENET_RDSR	1378	Receive Descriptor Ring Start
	// ENET_TDSR	1379	Transmit Buffer Descriptor Ring
	// ENET_MRBR	1380	Maximum Receive Buffer Size
	//		1457	receive buffer descriptor
	//		1461	transmit buffer descriptor

	print("enetbufferdesc_t size = ", sizeof(enetbufferdesc_t));
	print("rx_ring size = ", sizeof(rx_ring));
	memset(rx_ring, 0, sizeof(rx_ring));
	memset(tx_ring, 0, sizeof(tx_ring));
  print("buffer size ",BWORDS*4);
  print("RX buffers ",RXSIZE);
  print("TX buffers ",TXSIZE);

	for (int i=0; i < RXSIZE; i++) {
		rx_ring[i].flags = 0x8000; // empty flag
    rx_ring[i].moreflags = 0x800000;  // interrupt
		rx_ring[i].buffer = rxbufs + i * BWORDS;
	}
	rx_ring[RXSIZE-1].flags = 0xA000; // empty & wrap flags
	for (int i=0; i < TXSIZE; i++) {
    tx_ring[i].moreflags = 0x40000000;  // interrupt 
#ifdef HW_CHKSUMS
    tx_ring[i].moreflags |= 0x18000000;  // insert checksums
#endif
		tx_ring[i].buffer = txbufs + i * BWORDS;
	}
	tx_ring[TXSIZE-1].flags = 0x2000; // wrap flag

	ENET_EIMR = 0;
	ENET_MSCR = ENET_MSCR_MII_SPEED(15);  // 12 is fastest which seems to work
	ENET_RCR = ENET_RCR_NLC | ENET_RCR_MAX_FL(1522) | ENET_RCR_CFEN |
		ENET_RCR_CRCFWD | ENET_RCR_PADEN | ENET_RCR_RMII_MODE |
		/* ENET_RCR_FCE |  ENET_RCR_PROM | */ ENET_RCR_MII_MODE;
	ENET_TCR = ENET_TCR_ADDINS | /* ENET_TCR_RFC_PAUSE | ENET_TCR_TFC_PAUSE | */
		ENET_TCR_FDEN;
	ENET_PALR = (MACADDR1 << 8) | ((MACADDR2 >> 16) & 255);
	ENET_PAUR = ((MACADDR2 << 16) & 0xFFFF0000) | 0x8808;
	ENET_OPD = 0x10014;
	ENET_IAUR = 0;
	ENET_IALR = 0;
	ENET_GAUR = 0;
	ENET_GALR = 0;
	ENET_RDSR = (uint32_t)rx_ring;
	ENET_TDSR = (uint32_t)tx_ring;
	ENET_MRBR = BWORDS*4;
	ENET_TACC = ENET_TACC_SHIFT16;
#ifdef HW_CHKSUMS
	ENET_TACC |=  ENET_TACC_IPCHK | ENET_TACC_PROCHK;  // zero checksums
#endif
	ENET_RACC = ENET_RACC_SHIFT16;

  ENET_EIMR  = ENET_EIMR_TXF | ENET_EIMR_RXF;   // interrupts
  NVIC_ENABLE_IRQ(IRQ_ENET_TX);
  NVIC_ENABLE_IRQ(IRQ_ENET_RX);

	ENET_ECR = 0xF0000000 | ENET_ECR_DBSWP | ENET_ECR_EN1588 | ENET_ECR_ETHEREN;
	ENET_RDAR = ENET_RDAR_RDAR;
	ENET_TDAR = ENET_TDAR_TDAR;

	printhex("MDIO PHY ID2 (LAN8720A should be 0007): ", mdio_read(0, 2));
	printhex("MDIO PHY ID3 (LAN8720A should be C0F?): ", mdio_read(0, 3));
  printhex("PHY BCR control reg 0x",mdio_read(0, 0));
  printhex("PHY status reg 0x",mdio_read(0, 1));
  printhex("PHY reg 17 0x",mdio_read(0, 17));
  prregs();
  arp_request(manitou);
  uint8_t mybuff[8];
//  sendto(mybuff,sizeof(mybuff),4444,manitou, 7654); // to uechosrv
//  udp_blast(20,1000);
  //udp_echo(20);  // to uechosrv
 // udp_ntp(5,5000);  // make some ntp queries
  // udp_tcp(1000);
}

static int rxnum=0;   // need outside for loop or check_rx
// watch for data to arrive
void loop()
{

  static uint32_t ms = millis();
	volatile enetbufferdesc_t *buf;

	buf = rx_ring + rxnum;

	if ((buf->flags & 0x8000) == 0) {
		incoming(buf->buffer, buf->length, buf->flags);
		if (rxnum < RXSIZE-1) {
			buf->flags = 0x8000;
			rxnum++;
		} else {
			buf->flags = 0xA000;
			rxnum = 0;
		}
	}
  if (millis() - ms > 5000) {
    char str[256];
    
    ms = millis();
    sprintf(str,"in %d out %d arp %d ip %d  tcp %d udp %d ulth %d icmp %d bcast %d mcast %d",
      inpkts,outpkts,apkts,ippkts,tpkts,upkts,ulth,ipkts,bcast,mcast);
    Serial.println(str);
    sprintf(str,"rxcnt %d txcnt %d", rxcnt, txcnt);
    Serial.println(str);
    if (us0) {
      Serial.println(us-us0);
      us0=0;
    }
  }
}

// watch for data to arrive, quietly
void check_rx()
{
  static uint32_t ms = millis();
  volatile enetbufferdesc_t *buf;

  buf = rx_ring + rxnum;

  if ((buf->flags & 0x8000) == 0) {
    handle_frame(buf->buffer, buf->length, buf->flags);
    if (rxnum < RXSIZE-1) {
      buf->flags = 0x8000;
      rxnum++;
    } else {
      buf->flags = 0xA000;
      rxnum = 0;
    }
  }
}

// when we get data, try to parse it
void incoming(void *packet, unsigned int len,uint16_t flags)
{
	const uint8_t *p8;
	const uint16_t *p16;
	const uint32_t *p32;
	IPAddress src, dst;
	uint16_t type;

  inpkts++;
	Serial.println();
	print("data in, len=", len);
  printhex("flags 0x",flags);
	p8 = (const uint8_t *)packet + 2;
	p16 = (const uint16_t *)p8;
	p32 = (const uint32_t *)packet;
	type = p16[6];
  if (*p8 == 0xff) bcast++;
  if (*p8 == 1) mcast++;
	if (type == 0x0008) {   // IP
    ippkts++;
		src = p32[7];
		dst = p32[8];
		Serial.print("IPv4 Packet, src=");
		Serial.print(src);
		Serial.print(", dst=");
		Serial.print(dst);
    Serial.print(", type=");
    Serial.print(p8[23]);
		Serial.println();
	//	printpacket(p8, len - 2);
		if (p8[23] == 1 && dst == myaddress) {  // ICMP
      ipkts++;
			Serial.println("  Protocol is ICMP:");
			if (p8[34] == 8) {
				print("  echo request:");
				uint16_t id = __builtin_bswap16(p16[19]);
				uint16_t seqnum = __builtin_bswap16(p16[20]);
				printhex("   id = ", id);
				print("   sequence number = ", seqnum);
				ping_reply((uint32_t *)packet, len);
			}
		} else if (p8[23] == 17 && dst == myaddress) {  //  UDP
      us = micros();
      if (!us0) us0 = us;
      upkts++;
      ulth += len-44;
     // udp_reply((uint32_t *)packet,len);   // for echo test
      uint16_t sport = swap2(p16[17]);
      uint16_t dport = swap2(p16[18]);
      Serial.print("sport "); Serial.print(sport); Serial.print(" "); Serial.println(dport);
		} else if (p8[23] == 6 && dst == myaddress) {   //  TCP
      tpkts++;
      // could send back reset, need checksums
      uint16_t sport = swap2(p16[17]);
      uint16_t dport = swap2(p16[18]);
      Serial.print("sport "); Serial.print(sport); Serial.print(" "); Serial.println(dport);
    }
	} else if (type == 0x0608) {    // ARP
      arp_parse(packet,len);
	} else printpacket(p8,64);   // unknown
}


// when we get data, try to parse it quietly, queue UDP
void handle_frame(void *packet, unsigned int len, uint16_t flags)
{
  const uint8_t *p8;
  const uint16_t *p16;
  const uint32_t *p32;
  IPAddress src, dst;
  uint16_t type;

  inpkts++;

  p8 = (const uint8_t *)packet + 2;
  p16 = (const uint16_t *)p8;
  p32 = (const uint32_t *)packet;
  type = p16[6];
  if (*p8 == 0xff) bcast++;
  if (*p8 == 1) mcast++;
  if (type == 0x0008) {   // IP
    ippkts++;
    src = p32[7];
    dst = p32[8];

    if (p8[23] == 1 && dst == myaddress) {  // ICMP
      ipkts++;
     
      if (p8[34] == 8) {
        ping_reply((uint32_t *)packet, len);
      }
    } else if (p8[23] == 17 && dst == myaddress) {  //  UDP
      memcpy(UDP_buff,packet,len);
      UDP_lth=len;   // UDP pkt ready
    } else if (p8[23] == 6 && dst == myaddress) {   //  TCP
      tpkts++;
    }
  } else if (type == 0x0608) {    // ARP
      arp_parse(packet,len);
  } 
}

void arp_parse(void *packet, unsigned int len){
  const uint8_t *p8;
  const uint16_t *p16;
  const uint32_t *p32;

  p8 = (const uint8_t *)packet + 2;
  p16 = (const uint16_t *)p8;
  p32 = (const uint32_t *)packet;
    apkts++;
   // Serial.println("ARP Packet:");
  //  printpacket(p8, len - 2);
    if (p32[4] == 0x00080100 && p32[5] == 0x01000406) {
      // request is for IPv4 address of ethernet mac
      IPAddress from((p16[15] << 16) | p16[14]);
      IPAddress to(p32[10]);
      Serial.print("ARP  Who is ");
      Serial.print(to);
      Serial.print(" from ");
      Serial.print(from);
      Serial.print(" (");
      printmac(p8 + 22);
      Serial.println(")");
      if (to == myaddress) {
        arp_reply(p8+22, from);
      }
    } else if (p32[4] == 0x00080100 && p32[5] == 0x02000406) {
      // response to our query,  add to ARP table
      uint32_t t=micros()-arpt0;   // ARP RTT
      IPAddress from((p16[15] << 16) | p16[14]);
      Serial.print("ARP response: ");
      Serial.print(from);
      Serial.print(" (");
      printmac(p8 + 22);
      Serial.print(") ");
      Serial.print(t);
      Serial.println(" us");
    }
}

void arp_request( IPAddress ip)
{
  uint32_t packet[11]; // 42 bytes needed + 2 pad
  uint8_t *p = (uint8_t *)packet + 2;

  packet[0] = 0;       // first 2 bytes are padding
  memset(p, 0xff, 6);  // broadcast
  memset(p + 6, 0, 6); // hardware automatically adds our mac addr

  p[12] = 8;
  p[13] = 6;  // arp protocol
  packet[4] = 0x00080100; // IPv4 on ethernet
  packet[5] = 0x01000406; // request, ip 4 byte, macaddr 6 bytes
  packet[6] = (__builtin_bswap32(MACADDR1) >> 8) | ((MACADDR2 << 8) & 0xFF000000);
  packet[7] = __builtin_bswap16(MACADDR2 & 0xFFFF) | ((uint32_t)myaddress << 16);
  packet[8] = (((uint32_t)myaddress & 0xFFFF0000) >> 16) ;
  packet[9] = 0;
  packet[10] = (uint32_t)ip;
  Serial.print("ARP request: ");
  Serial.println(ip);
//  printpacket(p, 42);
  arpt0=micros();
  outgoing(packet, 44);
}

// compose an answer to ARP requests
void arp_reply(const uint8_t *mac, IPAddress &ip)
{
	uint32_t packet[11]; // 42 bytes needed + 2 pad
	uint8_t *p = (uint8_t *)packet + 2;

	packet[0] = 0;       // first 2 bytes are padding
	memcpy(p, mac, 6);
	memset(p + 6, 0, 6); // hardware automatically adds our mac addr

	p[12] = 8;
	p[13] = 6;  // arp protocol
	packet[4] = 0x00080100; // IPv4 on ethernet
	packet[5] = 0x02000406; // reply, ip 4 byte, macaddr 6 bytes
	packet[6] = (__builtin_bswap32(MACADDR1) >> 8) | ((MACADDR2 << 8) & 0xFF000000);
	packet[7] = __builtin_bswap16(MACADDR2 & 0xFFFF) | ((uint32_t)myaddress << 16);
	packet[8] = (((uint32_t)myaddress & 0xFFFF0000) >> 16) | (mac[0] << 16) | (mac[1] << 24);
	packet[9] = (mac[5] << 24) | (mac[4] << 16) | (mac[3] << 8) | mac[2];
	packet[10] = (uint32_t)ip;
	Serial.println("ARP Reply:");
	printpacket(p, 42);
	outgoing(packet, 44);
}

// compose an reply to pings
void ping_reply(const uint32_t *recv, unsigned int len)
{
	uint32_t packet[256];
	uint8_t *p8 = (uint8_t *)packet + 2;
  uint16_t *p16 = (uint16_t *)p8;

	if (len > sizeof(packet)) return;
	memcpy(packet, recv, len);
	memcpy(p8, p8 + 6, 6); // send to the mac address we received
	// hardware automatically adds our mac addr
	packet[8] = packet[7]; // send to the IP number we received
	packet[7] = (uint32_t)myaddress;
	p8[34] = 0;            // type = echo reply
	//  checksums in IP and ICMP headers - is the hardware
	// really inserting correct checksums automatically?  OK 0 for icmp
#ifdef HW_CHKSUMS
 p16[18] = p16[12]=0;    // checksums 0 for hardware else OK
#endif
	//printpacket((uint8_t *)packet + 2, len - 2);
	outgoing(packet, len);
}

// UDP reply  (warning: using ring recv buffer for transmit)
void udp_reply( uint32_t *recv, unsigned int len)
{
  uint16_t *p16;
  uint8_t *p8 = (uint8_t *)recv + 2;
  p16 = (uint16_t *)p8;
  uint16_t tmp;

  // swap mac addresses, IP addresses, and src/dst port  checksum should be ok
  memcpy(p8, p8 + 6, 6); // send to the mac address we received
  // hardware automatically adds our mac addr
  recv[8] = recv[7]; // send to the IP number we received
  recv[7] = (uint32_t)myaddress;
  tmp = p16[17];  // swap port
  p16[17]=p16[18];
  p16[18]=tmp;
#ifdef HW_CHKSUMS
  p16[20] = p16[12]=0;    // checksums 0 for hardware, else ok
#endif

  //printpacket((uint8_t *)recv + 2, len - 2);
  outgoing(recv, len);
}

void udp_blast(int pkts, int bytes) {
  int i,udplth, iplth, pktlth;  
  uint8_t *p8 = (uint8_t *)prefab + 2;
  uint16_t *p16 = (uint16_t *)p8;
  uint32_t t, *p32 = (uint32_t *) prefab;

  Serial.println("UDP blast");
  // fill in prefab to send 20 1000 byte to udpsink port 2000
  udplth = bytes+8;
  iplth = 20 + udplth;
  pktlth = 14 + 2 + iplth;  
  p16[18] = swap2(2000);  // dst port
  p16[19] = swap2(udplth);
  p16[8] = swap2(iplth);
  p16[20] = p16[12]=0;    // checksums 0 
#ifndef HW_CHKSUMS
  // do IP hdr checksum
  p16[12] = inet_chksum(p8+14,20);
#ifdef SW_UDP_CHKSUM
  // do UDP chksum  else 0 means no chksum, to fffff if 0
  p16[20] = inet_chksum_pseudo(p8+34,p32[7],p32[8],17,udplth);
  if (p16[20] == 0) p16[20] = 0xffff;
#endif
#endif
  //printpacket(prefab + 2, 64);
  t=micros();
  for(i=0;i<pkts; i++) {
    p32[11] = swap4(i);   // sequence number
    outgoing(prefab, pktlth);
//    delay(1);   // rate limit
  }
  t=micros()-t;
  Serial.println(t);
}

void sendto(void *packet, unsigned int bytes, int sport, IPAddress dest,int dport) {
  int i,udplth, iplth, pktlth;  
  uint8_t *p8 = (uint8_t *)prefab + 2;
  uint16_t *p16 = (uint16_t *)p8;
  uint32_t t, *p32 = (uint32_t *) prefab;

 // Serial.println("UDP sendto");
  // fill in prefab (already setup for manitou) arp tab
  memcpy(p8+42,packet,bytes);    // UDP payload
  udplth = bytes+8;
  iplth = 20 + udplth;
  pktlth = 14 + 2 + iplth; 
  p16[17] = swap2(sport);  // src port 
  p16[18] = swap2(dport);  // dst port
  p16[19] = swap2(udplth);
  p16[8] = swap2(iplth);
  p16[20] = p16[12]=0;    // checksums 0 
#ifndef HW_CHKSUMS
  // do IP hdr checksum
  p16[12] = inet_chksum(p8+14,20);
#ifdef SW_UDP_CHKSUM
  // do UDP chksum  else 0 means no chksum, to fffff if 0
  p16[20] = inet_chksum_pseudo(p8+34,p32[7],p32[8],17,udplth);
  if (p16[20] == 0) p16[20] = 0xffff;
#endif
#endif
//  printpacket(prefab + 2, 64);
  outgoing(prefab, pktlth);
}

int recvfrom(void *packet, unsigned int len,int sport, IPAddress *dest,int *dport) {
  // don't call til UDP_lth is non zero
  // copy packet return length of UDP data payload
  int lth;
  uint8_t *p8 = (uint8_t *)UDP_buff + 2;
  uint16_t *p16 = (uint16_t *)p8;
  uint32_t t, *p32 = (uint32_t *) UDP_buff;

  lth = swap2(p16[19]) - 8;   // UDP lth less UDP header
  memcpy(packet,p8+42,lth);    // UDP payload  check lth vs len?
  //  retrieve IP src and IP src port   TODO
  UDP_lth=0;    // ready for another
  return lth;
}


void udp_echo(int reps) {
  // app client to  send packets to uechosrv, actively poll ether ring
  uint32_t t, tmax=0, tmin=9999999, sum, i;
  IPAddress sender;
  int sport;
  uint8_t buff[256];
  char str[64];

  UDP_lth=0;
  for (i=0;i<reps; i++) {
      t=micros();
      sendto(buff,8,4444,manitou,7654);
      while(UDP_lth==0) check_rx();  // poll ether ring
      recvfrom(buff,sizeof(buff), 4444, &sender, &sport);
      t=micros()-t;
      sum += t;
      if (t>tmax) tmax=t;
      if (t< tmin) tmin = t;
  }
  sprintf(str,"min %d max %d  avrg %d  us",tmin,tmax,sum/reps);
  Serial.println(str);
}

#define MSS 1460
#define WPKTS 2   // window
void udp_tcp(int reps) {
  // app client to  send packets to udp_ack, actively poll ether ring
  uint32_t t;
  int i, acks=0;
  IPAddress sender;
  int sport;
  int rtt[reps] ; // optional stats
  uint8_t buff[MSS] __attribute__ ((aligned(4)));
  char str[128];

  UDP_lth=0;
  // starting blast
  t=micros();
  for (i=0;i<WPKTS; i++) {
    *(int *)buff = i;   // pkt count
    *(int *)(buff+4) = micros(); // micros for RTT
    sendto(buff,sizeof(buff),4444,manitou,7654);
  }
  for (i=WPKTS;i<reps; i++) {   
      while(UDP_lth==0) check_rx();  // poll ether ring
      recvfrom(buff,sizeof(buff), 4444, &sender, &sport);
      rtt[acks] = micros() - *(int *)(buff+4);
      acks++;
      *(int *)buff = i;   // pkt count
      *(int *)(buff+4) = micros(); // micros for RTT
      sendto(buff,sizeof(buff),4444,manitou,7654);
  }
  while(acks < reps) {
      while(UDP_lth==0) check_rx();  // poll ether ring
      recvfrom(buff,sizeof(buff), 4444, &sender, &sport);
      rtt[acks] = micros() - *(int *)(buff+4);
      acks++;
  }
  t=micros()-t;
  sprintf(str,"WPKTS %d MSS %d  %d pkts %d bytes %d us   mbs ",WPKTS,MSS,reps, reps*MSS, t);
  Serial.print(str);
  Serial.println(8.*reps*MSS/t);

  for (i=0;i<reps; i++) {
    Serial.print(i); Serial.print(" "); Serial.println(rtt[i]);
  }
}

void udp_ntp(int reps, int ms) {
  int i, sport, t;
  uint32_t  secs;
  IPAddress sender;
  uint8_t buff[48] __attribute__ ((aligned(4)));

  UDP_lth=0;
  for (i=0;i<reps; i++) {
    buff[0] = 0x1b;   // ntp query
    sendto(buff,sizeof(buff),4444,manitou,123);
    while(UDP_lth==0) check_rx();  // poll ether ring
    recvfrom(buff,sizeof(buff), 4444, &sender, &sport);
    secs = *(uint32_t *) (buff+40);
    Serial.print("ntp "); Serial.println(swap4(secs));
    t=millis();
    while(millis() -t < ms) check_rx();   // active delay
  }
}

// transmit a packet
void outgoing(void *packet, unsigned int len)
{
	static int txnum=0;
	volatile enetbufferdesc_t *buf;
	uint16_t flags;

  outpkts++;
	buf = tx_ring + txnum;
 while(1) {   // keep trying   80us max?
	flags = buf->flags;
	if ((flags & 0x8000) == 0) {
	//	print("tx, num=", txnum);
		buf->length = len;
		memcpy(buf->buffer, packet, len);
		buf->flags = flags | 0x8C00;
		ENET_TDAR = ENET_TDAR_TDAR;
		if (txnum < TXSIZE-1) {
			txnum++;
		} else {
			txnum = 0;
		}
    return;  // q'd it
	}
 }  // while
}

// IP checksums
// chksum
//
// Sums up all 16 bit words in a memory portion. Also includes any odd byte.
// This function is used by the other checksum functions.
//

static unsigned long chksum(void *dataptr, int len) {
  unsigned long acc;
  unsigned short *p = (unsigned short *) dataptr;
    
  //for (acc = 0; len > 1; len -= 2) acc += *((unsigned short *) dataptr++);
  for (acc = 0; len > 1; len -= 2) {
   acc += *p++;
   }

  // Add up any odd byte
  if (len == 1) acc += *(unsigned char *) p;

  acc = (acc >> 16) + (acc & 0xFFFF);
  if ((acc & 0xFFFF0000) != 0) acc = (acc >> 16) + (acc & 0xFFFF);

  return acc;
}

//
// inet_chksum_pseudo
//
// Calculates the pseudo Internet checksum used by TCP and UDP for a pbuf chain.
//  TCP header could vary (options), UDP 8,  UDP sum =0 to ignore
// proto tcp 6  udp 17

unsigned short inet_chksum_pseudo(void *p, unsigned long srcaddr, unsigned long destaddr, unsigned char proto, unsigned short proto_len) {
  unsigned long acc;
  int swapped;

  acc = 0;
  swapped = 0;
    acc += chksum(p, proto_len);
    while (acc >> 16) acc = (acc & 0xFFFF) + (acc >> 16);

    if (proto_len % 2 != 0) {
      swapped = 1 - swapped;
      acc = ((acc & 0xFF) << 8) | ((acc & 0xFF00) >> 8);
    }

  if (swapped) acc = ((acc & 0xFF) << 8) | ((acc & 0xFF00) >> 8);

  acc += (srcaddr & 0xFFFF);
  acc += ((srcaddr >> 16) & 0xFFFF);
  acc += (destaddr & 0xFFFF);
  acc += ((destaddr >> 16) & 0xFFFF);
  acc += (unsigned long) swap2((unsigned short) proto);
  acc += (unsigned long) swap2(proto_len);  
  
  while (acc >> 16) acc = (acc & 0xFFFF) + (acc >> 16);

  return (unsigned short) ~(acc & 0xFFFF);
}

//
// inet_chksum
//
// Calculates the Internet checksum over a portion of memory. Used primarely for IP
// and ICMP.
//

unsigned short inet_chksum(void *dataptr, int len) {
  unsigned long acc;

  acc = chksum(dataptr, len);
  while (acc >> 16) acc = (acc & 0xFFFF) + (acc >> 16);

  return (unsigned short) ~(acc & 0xFFFF);
}

// read a PHY register (using MDIO & MDC signals)
uint16_t mdio_read(int phyaddr, int regaddr)
{
	ENET_MMFR = ENET_MMFR_ST(1) | ENET_MMFR_OP(2) | ENET_MMFR_TA(2)
		| ENET_MMFR_PA(phyaddr) | ENET_MMFR_RA(regaddr);
	// TODO: what is the proper value for ENET_MMFR_TA ???
	//int count=0;
	while ((ENET_EIR & ENET_EIRM_MII) == 0) {
		//count++; // wait
	}
	//print("mdio read waited ", count);
	uint16_t data = ENET_MMFR;
	ENET_EIR = ENET_EIRM_MII;
	//printhex("mdio read:", data);
	return data;
}

// write a PHY register (using MDIO & MDC signals)
void mdio_write(int phyaddr, int regaddr, uint16_t data)
{
	ENET_MMFR = ENET_MMFR_ST(1) | ENET_MMFR_OP(1) | ENET_MMFR_TA(2)
		| ENET_MMFR_PA(phyaddr) | ENET_MMFR_RA(regaddr) | ENET_MMFR_DATA(data);
	// TODO: what is the proper value for ENET_MMFR_TA ???
	//int count=0;
	while ((ENET_EIR & ENET_EIRM_MII) == 0) {
		//count++; // wait
	}
	ENET_EIR = ENET_EIRM_MII;
	//print("mdio write waited ", count);
	//printhex("mdio write :", data);
}


// misc print functions, for lots of info in the serial monitor.
// this stuff probably slows things down and would need to go
// for any hope of keeping up with full ethernet data rate!

void print(const char *s)
{
	Serial.println(s);
}

void print(const char *s, int num)
{
	Serial.print(s);
	Serial.println(num);
}

void printhex(const char *s, int num)
{
	Serial.print(s);
	Serial.println(num, HEX);
}

void printmac(const uint8_t *data)
{
	Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X",
		data[0], data[1], data[2], data[3], data[4], data[5]);
}

void printpacket(const uint8_t *data, unsigned int len)
{
#if 1
	unsigned int i;

	for (i=0; i < len; i++) {
		Serial.printf(" %02X", *data++);
		if ((i & 15) == 15) Serial.println();
	}
	Serial.println();
#endif
}

