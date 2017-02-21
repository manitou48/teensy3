// init K66 ethernet regs
// init lwip
//  low level input output
//  eth_init(myip, gw, mask)   eth_poll()
// TODO ENET_PAD TACC/RACC SHIFT16, my counters 
#include "lwipk66.h"

uint32_t inpkts,outpkts;
uint32_t tom1,tom2,tom3; // debug

static struct netif netif;

static char mac_addr[19];
static char ip_addr[17] = "\0";
static char gateway[17] = "\0";
static char networkmask[17] = "\0";

//  eventually fetch mac address from teensy ROM
#if  1    //static IP
static char mac[6] __attribute__ ((aligned(4))) = {0x04,0xe9,0xe5,0xba,0xbe,0x01};
#else    // dhcp
static char mac[6] __attribute__ ((aligned(4))) = {0x04,0xe9,0xe5,0xba,0xbe,0xdc};
#endif

//#define MACADDR1 0x04E9E5
//#define MACADDR2 0xBABE01

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
static uint32_t rxbufs[RXSIZE*BWORDS] __attribute__ ((aligned(16)));
static uint32_t txbufs[TXSIZE*BWORDS] __attribute__ ((aligned(16)));

//#define HW_CHKSUMS     // warning: breaks if pkt > 198
//  enable HW CHKSUMS in TX ring buffer and TACC

void etherk66_init() {
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
    memset(rx_ring, 0, sizeof(rx_ring));
    memset(tx_ring, 0, sizeof(tx_ring));

    for (int i=0; i < RXSIZE; i++) {
        rx_ring[i].flags = 0x8000; // empty flag
    	// rx_ring[i].moreflags = 0x800000;  // interrupt
        rx_ring[i].buffer = rxbufs + i * BWORDS;
    }
    rx_ring[RXSIZE-1].flags = 0xA000; // empty & wrap flags
    for (int i=0; i < TXSIZE; i++) {
//    tx_ring[i].moreflags = 0x40000000;  // interrupt
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
    ENET_PALR = __builtin_bswap32(*(uint32_t *)mac);
    ENET_PAUR = ((__builtin_bswap32(((uint32_t *)mac)[1])) & 0xFFFF0000) | 0x8808;
//    ENET_PALR = (MACADDR1 << 8) | ((MACADDR2 >> 16) & 255);
//    ENET_PAUR = ((MACADDR2 << 16) & 0xFFFF0000) | 0x8808;
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

    ENET_ECR = 0xF0000000 | ENET_ECR_DBSWP | ENET_ECR_EN1588 | ENET_ECR_ETHEREN;
    ENET_RDAR = ENET_RDAR_RDAR;
    ENET_TDAR = ENET_TDAR_TDAR;

}

err_t k66_low_level_output(struct netif *netif, struct pbuf *p)
{
	//  k64f was more complicated?  who frees pbuf ?
	//  blocks to xmit ring buffer available  80us max?
    static int txnum=0;
    volatile enetbufferdesc_t *buf;
    uint16_t flags;
	struct pbuf *q;
	uint8_t  *dst;

	outpkts++;
	LINK_STATS_INC(link.xmit);
    buf = tx_ring + txnum;
 	while(1) {   // keep trying   @100mbs 120us max?
    	flags = buf->flags;
    	if ((flags & 0x8000) == 0) {
        	buf->length = p->tot_len;
			//  are there really multi pbufs?
    		for (q = p, dst = buf->buffer; q != NULL; q = q->next) {
      			MEMCPY(dst, q->payload, q->len);
      			dst += q->len;
    		}
        	buf->flags = flags | 0x8C00;
        	ENET_TDAR = ENET_TDAR_TDAR;
        	if (txnum < TXSIZE-1) {
            	txnum++;
        	} else {
            	txnum = 0;
        	}
    		return ERR_OK;  // q'd it
    	}
 	}  // while
}

void handle_frame(void *packet, unsigned int len, uint16_t flags)
{
	// flags could indicate errors, just drop it
	//  create pbuf, memcpy, and  pass it on
	//  TODO
	struct pbuf *p;

	inpkts++;
	LINK_STATS_INC(link.recv);
	p = pbuf_alloc(PBUF_RAW,len,PBUF_RAM);
	if (p==NULL) {
		LINK_STATS_INC(link.memerr);
		LINK_STATS_INC(link.drop);
		return;
	}
	MEMCPY(p->payload,packet,len);   // assume not chain ?
	if(ethernet_input(p,&netif) != ERR_OK) {
		pbuf_free(p);
    }
}

err_t k66_enetif_init(struct netif *netif)
{
	// fcn passed to netif_add()

	memcpy(netif->hwaddr,mac,6);
	netif->hwaddr_len = ETHARP_HWADDR_LEN;
	netif->mtu = 1500;
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET |  NETIF_FLAG_LINK_UP;
#if LWIP_IGMP
	netif->flags |= NETIF_FLAG_IGMP;
#endif /* LWIP_IGMP */

	
	etherk66_init();    

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "lwipk64f";
#endif /* LWIP_NETIF_HOSTNAME */

  netif->name[0] = 'e';
  netif->name[1] = 'n';

  netif->output = etharp_output;
  netif->linkoutput = k66_low_level_output;

	return ERR_OK;
}

void ether_init(const char *ipaddr, const char *netmask, const char *gw) {
	
	snprintf(mac_addr, 19, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    strcpy(ip_addr, ipaddr);
    strcpy(gateway, gw);
    strcpy(networkmask, netmask);
    
    ip_addr_t ip_n, mask_n, gateway_n;
    inet_aton(ipaddr, &ip_n);
    inet_aton(netmask, &mask_n);
    inet_aton(gw, &gateway_n);
	memset((void*) &netif, 0, sizeof(netif));
    netif_add(&netif, &ip_n, &mask_n, &gateway_n, NULL, k66_enetif_init, ethernet_input);
    netif_set_default(&netif);
	// ? netif call backs?
  	netif_set_up(&netif);
}

int ether_init_dhcp() {
#if LWIP_DHCP
	uint32_t ms;
	snprintf(mac_addr, 19, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	memset((void*) &netif, 0, sizeof(netif));
	netif_add(&netif, NULL,NULL,NULL, NULL,k66_enetif_init, ethernet_input);
	netif_set_default(&netif);
	dhcp_start(&netif);   // try DHCP
	ms = sys_now();
	while (netif.dhcp->state != DHCP_BOUND) {
		ether_poll();
		tom1 = netif.dhcp->state;
		if (sys_now()-ms > 10000) return -1; // timed out
	}
	return 0;
#else
	return -1;   // no dhcp
#endif
}

void ether_poll() {
  static int rxnum=0;
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

  sys_check_timeouts();   // lwip timers
}

void ether_delay(uint32_t ms) {
	// wait and poll
	uint32_t t = sys_now();
	while(sys_now() -t < ms) ether_poll();
}

uint32_t ether_get_ipaddr() {
	return  netif.ip_addr.addr;
}
