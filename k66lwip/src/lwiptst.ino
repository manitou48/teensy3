// K66 lwiptst   
// discussion on https://forum.pjrc.com/threads/34808-K66-Beta-Test?p=109161&viewfull=1#post109161

#include "lwipk66.h"

// api includes
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "lwip/stats.h"

#define swap2 __builtin_bswap16
#define swap4 __builtin_bswap32

// debug stats stuff
extern "C" { 
	uint32_t inpkts,outpkts;
//	char thdstr[64];
#if LWIP_STATS
	struct stats_ lwip_stats;
#endif
}

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

void print_stats() {
	 // lwip stats_display() needed printf
#if LWIP_STATS
	char str[128];

	// ? LINK stats are 0
	sprintf(str,"TCP in %d out %d drop %d memerr %d",
	 lwip_stats.tcp.recv,lwip_stats.tcp.xmit,lwip_stats.tcp.drop,lwip_stats.tcp.memerr);
	Serial.println(str);
	sprintf(str,"UDP in %d out %d drop %d memerr %d",
	 lwip_stats.udp.recv,lwip_stats.udp.xmit,lwip_stats.udp.drop,lwip_stats.udp.memerr);
	Serial.println(str);
	sprintf(str,"ICMP in %d out %d",
	 lwip_stats.icmp.recv,lwip_stats.icmp.xmit);
	Serial.println(str);
#if MEM_STATS
	sprintf(str,"HEAP avail %d used %d max %d err %d",
	 lwip_stats.mem.avail,lwip_stats.mem.used,lwip_stats.mem.max,lwip_stats.mem.err);
	Serial.println(str);
#endif
#endif
}


// UDP callbacks
//  fancy would be pbuf recv_q per UDP pcb, if q full, drop and free pbuf

static volatile int udp_bytes, udp_pkts;

// udp recv callback
void udp_callback(void * arg, struct udp_pcb * upcb, struct pbuf * p, struct ip_addr * addr, u16_t port) 
{
	if (p == NULL) return;
	udp_bytes +=  p->tot_len;
	udp_pkts++;
	pbuf_free(p);
}

void ntp_callback(void * arg, struct udp_pcb * upcb, struct pbuf * p, struct ip_addr * addr, u16_t port) 
{
	if (p == NULL) return;
	if (p->tot_len == 48) {
		uint32_t secs = ((uint32_t *) p->payload)[10]; // NTP secs
		Serial.println(swap4(secs));
	}
	pbuf_free(p);
}


void udp_echo(int pkts, int bytes) {
	int i,prev=0;
	struct udp_pcb *pcb;
	pbuf *p;
	uint32_t t,ms;
	ip_addr_t server;

    inet_aton("192.168.1.4", &server);
	pcb = udp_new();
	udp_bind(pcb, IP_ADDR_ANY, 4444);    // local port
	udp_recv(pcb,udp_callback,NULL /* *arg */);    // do once?
	for(i=0; i<pkts; i++) {
		p = pbuf_alloc(PBUF_TRANSPORT, bytes, PBUF_RAM);  // need each time?
		t=micros();
		udp_sendto(pcb,p,&server,7654);
		while(udp_bytes <= prev) ether_poll();  // wait for reply
		t=micros()-t;
		prev=udp_bytes;
   		pbuf_free(p);
		Serial.print(t); Serial.print(" us  "); Serial.println(udp_bytes);
		ms=millis();  // ether delay
		while(millis()-ms < 2000) ether_poll();
	}

	pbuf_free(p);
	udp_remove(pcb);
}

void udp_ntp(int pkts) {
    int i;
    struct udp_pcb *pcb;
    pbuf *p;
    uint32_t ms;
    ip_addr_t server;

    inet_aton("192.168.1.4", &server);
    pcb = udp_new();
    udp_bind(pcb, IP_ADDR_ANY, 4444);    // local port
    udp_recv(pcb,ntp_callback,NULL /* *arg */);    // do once?
    for(i=0; i<pkts; i++) {
        p = pbuf_alloc(PBUF_TRANSPORT, 48, PBUF_RAM);  // need each time?
        *(uint8_t *)p->payload = 0x1b;    // NTP query
        udp_sendto(pcb,p,&server,123);
        pbuf_free(p);
        ms=millis();  // ether delay
        while(millis()-ms < 5000) ether_poll();
    }

    pbuf_free(p);
    udp_remove(pcb);
}

void udp_sink() {
    int pkts=0,prev=0;
    struct udp_pcb *pcb;
    uint32_t t,t0=0,ms=0;

	Serial.println("udp sink on port 8888");
    pcb = udp_new();
    udp_bind(pcb, IP_ADDR_ANY, 8888);    // local port
    udp_recv(pcb,udp_callback,NULL);    // do once?
    while(1) {   
        while(udp_bytes <= prev) {
			ether_poll();  // wait for incoming
			if (ms && (millis()-ms > 2000)) { // timeout
				char str[64];
				t=t-t0;
				sprintf(str,"%d pkts %d bytes %d us ",pkts,udp_bytes,t);
				Serial.print(str);
				Serial.println(8.*udp_bytes/t);
    			udp_remove(pcb);
				return;
			}
		}
        t=micros();
		if (t0==0) {t0 = t; ms = millis();}
        pkts++;
        prev=udp_bytes;
    }

}

void udp_blast(int pkts, int bytes) {
    int i;
    struct udp_pcb *pcb;
    pbuf *p;
    uint32_t t;
    ip_addr_t server;

	Serial.println("UDP blast");
	delay(1000);
    inet_aton("192.168.1.4", &server);
    pcb = udp_new();
    udp_bind(pcb, IP_ADDR_ANY, 3333);    // local port
    t=micros();
    for(i=0; i<pkts; i++) {
    	p = pbuf_alloc(PBUF_TRANSPORT, bytes, PBUF_RAM);  // ? in the loop
		*(uint32_t *)(p->payload) = swap4(i);  // seq number
        udp_sendto(pcb,p,&server,2000);
		pbuf_free(p);
//		delay(100);  // rate limit ?
    }
    t=micros()-t;
    Serial.println(t);

    pbuf_free(p);
    udp_remove(pcb);
}

//   ----- TCP ------

void tcperr_callback(void * arg, err_t err)
{
	// set with tcp_err()
	Serial.print("TCP err "); Serial.println(err);
	*(int *)arg = err;
}

err_t connect_callback(void *arg, struct tcp_pcb *tpcb, err_t err) {
	Serial.print("connected "); Serial.println(tcp_sndbuf(tpcb));
	*(int *)arg = 1;
	return 0;
}

err_t sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len) {
	Serial.print("sent "); Serial.println(len);
	return 0;
}

void tcptx(int pkts) {
	// send to ttcp -r -s
	char buff[1000];
	ip_addr_t server;
	struct tcp_pcb * pcb;
	int i,connected=0;
	err_t err;
	uint32_t t, sendqlth;

	Serial.println("tcptx");
	inet_aton("192.168.1.4", &server);
	pcb = tcp_new();
	tcp_err(pcb,tcperr_callback);
//	tcp_sent(pcb,sent_callback);
	tcp_arg(pcb, &connected);
	tcp_bind(pcb,IP_ADDR_ANY, 3333);    // local port
	sendqlth = tcp_sndbuf(pcb);
	tcp_connect(pcb,&server,5001,connect_callback); 
	while(!connected) ether_poll();
	if (connected < 0) return;  // err
	t=micros();
	for(i=0;i<pkts;i++) {
		do {
			err=tcp_write(pcb,buff,sizeof(buff),TCP_WRITE_FLAG_COPY);
			ether_poll();   // keep checkin while we blast
		} while( err < 0);   // -1 is ERR_MEM
	}
	while(tcp_sndbuf(pcb) != sendqlth) ether_poll(); // wait til sent
	tcp_close(pcb);
	t=micros()-t;
	Serial.print(t); Serial.print(" us  ");Serial.println(8.*pkts*sizeof(buff)/t);
	//tcp_abort(pcb);
}

static struct tcp_pcb * pcba;   // accepted pcb

err_t accept_callback(void * arg, struct tcp_pcb * newpcb, err_t err) {
	Serial.println("accepted");
	tcp_accepted(newpcb);
	pcba = newpcb;    // let tcprx proceed
	return 0;
}

err_t recv_callback(void * arg, struct tcp_pcb * tpcb, struct pbuf * p, err_t err)
{
	static uint32_t t0=0, t,bytes=0;

	t=micros();
	if (!t0) t0=t;
	if (p == NULL) {
		// other end closed
		t=t-t0;
		Serial.println("remote closed");
		Serial.print(bytes); Serial.print(" ");
		Serial.print(t); Serial.print(" us   ");
		Serial.println(8.*bytes/t);

		tcp_close(tpcb);
		return 0;
	}
	tcp_recved(tpcb,p->tot_len);   // data processed
	bytes += p->tot_len;
	pbuf_free(p);
	return 0;
}

void tcprx() {
	struct tcp_pcb * pcb;
	struct tcp_pcb * pcbl;   // listen
	
	Serial.println("server listening");
	pcb = tcp_new();
	tcp_bind(pcb,IP_ADDR_ANY, 5001);  // server port
	pcbl = tcp_listen(pcb);   // pcb deallocated
	tcp_accept(pcbl, accept_callback);
	while (pcba == NULL) ether_poll();   // waiting connection
	tcp_err(pcba,tcperr_callback);
	tcp_recv(pcba, recv_callback);  // all the action is now in callback
	tcp_close(pcbl);
	// fall through to main ether_poll loop ....
}

void setup() {
	Serial.begin(9600);
	while(!Serial);
	delay(3000);
    Serial.println();Serial.print(F_CPU); Serial.print(" ");
    Serial.print(F_BUS); Serial.print(" ");
    Serial.print(F_MEM); Serial.print(" ");
    Serial.print(__TIME__);Serial.print(" ");Serial.println(__DATE__);

	Serial.println("lwiptst");

	// init Ether and lwip
	ether_init("192.168.1.23","255.255.255.0","192.168.1.1");
	prregs();
//	udp_sink();
//	udp_echo(10,8);
//	udp_blast(20,1000);   // blast needs echo to run first ?
//	udp_ntp(10);
	tcptx(100);
//	tcprx();
}

void loop() {
	static uint32_t ms = millis();
	ether_poll();    // check for new pkts, check timers
	if (millis() -ms > 5000) {   // active "delay"
	   char str[128];
	   sprintf(str,"%lu ms  in %lu  out %lu ",ms,inpkts,outpkts);
	   Serial.println(str);
	   print_stats();
//	   Serial.println(thdstr);
	   ms = millis();
	}
}
