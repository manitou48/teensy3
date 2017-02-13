/*
  multicast chat
  separate 3v3 power, common GND, SPI (10-13) reset to pin 7
*/

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

#define LINELTH 128
// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0x0A, 0x1B, 0x3C, 0x4D, 0x5E, 0x6F};
byte ip[] = { 192, 168, 1, 15};

EthernetUDP Udp;

byte mcastip[] = { 224, 7, 8, 9};
int mport = 7654;

const int PACKET_SIZE = 256;

byte packetBuffer[ PACKET_SIZE]; //buffer to hold incoming  packets

void setup()
{
  Serial.begin(9600);
  while (!Serial);
  Serial.println("mtalk starting");
  // start Ethernet and UDP
  pinMode(7, OUTPUT);
  digitalWrite(7, LOW);  // reset
  delay(1);
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  digitalWrite(7, HIGH);
  delay(3000);

  Ethernet.begin(mac, ip);
  Udp.beginMulticast(mcastip, mport);
  IPAddress ipaddr = Ethernet.localIP();
  Serial.print("my ip "); Serial.println(ipaddr);

}

void loop()
{
  int i, bytes;
  unsigned long pkts = 1, ms = millis();
  char buff[LINELTH];
  bool line;

  line=false;
  memset(buff,0,LINELTH);
  i=0;

  while (1) {
#if 1
    // periodic chirp
    if (millis() - ms > 5000) {
      sprintf(buff, "hello pkt %ld  %ld ms\n", pkts++, millis());
      Udp.beginPacket(mcastip, mport);
      Udp.write(buff, strlen(buff) + 1); // multicast our msg
      Udp.endPacket();
      Serial.print("sending mcast:"); Serial.println(buff);
      ms = millis();
    }
#else
    // build and send line
    while(Serial.available()) {
      char c;
      c=Serial.read();
      buff[i++]=c;
      if (c =='\n') {
        buff[i]=0;
        line=true;
        break;
      }
      if (i > LINELTH-2) i = LINELTH-2;  
    }
    if (line) {
      Udp.beginPacket(mcastip, mport);
      Udp.write(buff, strlen(buff) + 1); // multicast our line
      Udp.endPacket();
      Serial.print("sent: "); Serial.println(buff);
      line=false;
      memset(buff,0,LINELTH);
      i=0;
    }
#endif

    bytes = Udp.parsePacket();
    if (bytes) {
      // We've received a packet, read the data from it
      memset(packetBuffer, 0, sizeof(packetBuffer));
      Udp.read(packetBuffer, sizeof(packetBuffer));
      Serial.print("From ");
      IPAddress remote = Udp.remoteIP();
      Serial.println(remote);
      Serial.println((char *)packetBuffer);
    }
  }  // while
}

