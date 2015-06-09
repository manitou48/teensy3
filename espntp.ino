/* Based on:
 * ====== ESP8266 Demo ======
 * (Updated Dec 14, 2014)
 * Ray Wang @ Rayshobby LLC
 * http://rayshobby.net/?p=9734
 * ==========================
 *
 * Modified by R. Wozniak
 * Compiled with Arduino 1.60 and Teensyduino 1.21b6
 * ESP8266 Firmware: AT21SDK95-2015-01-24.bin
 *
 * Change SSID and PASS to match your WiFi settings.
 * The IP address is displayed serial upon successful connection.
 */

#include <Time.h>

const int timeZone = -4;  // Eastern DST (USA)

#define BUFFER_SIZE 1024

#define SSID  "FIXME"      // change this to match your WiFi SSID
#define PASS  "FIXME"  // change this to match your WiFi password
#define PORT  "123"         
#define NTPSERVER  "192.168.1.4" 

char buffer[BUFFER_SIZE],query[48];

byte binfind(char *str) {
  unsigned long t=millis();
  bool found=false;
  int i=0,j=0, len = strlen(str); 
  char c;
  while(millis()<t+5000) {
    if(Serial1.available()) {
      c=Serial1.read();
	  if (c == str[i]) {
	  	i++;
		if (i == len) return true;
	  } else i =0;
    }
  }
  return found;
}


// By default we are looking for OK\r\n
char OKrn[] = "OK\r\n";

byte wait_for_esp_response(int timeout, char* term=OKrn) {
  unsigned long t=millis();
  bool found=false;
  int i=0;
  int len=strlen(term);
  // wait for at most timeout milliseconds
  // or if OK\r\n is found
  while(millis()<t+timeout) {
    if(Serial1.available()) {
      buffer[i++]=Serial1.read();
      if(i>=len) {
        if(strncmp(buffer+i-len, term, len)==0) {
          found=true;
          break;
        }
      }
    }
  }
  buffer[i]=0;
  Serial.print(buffer);
  return found;
}

void setup() {

  // assume esp8266 operates at 115200 baud rate
  // change if necessary to match your modules' baud rate
  Serial1.begin(115200);  // Teensy Hardware Serial port 1   (pins 0 and 1)
  Serial.begin(115200);   // Teensy USB Serial Port
  
  delay(5000);
  Serial.println("begin.");  
  setupWiFi();

  // print device IP address
  Serial.print("device ip addr: ");
  Serial1.println("AT+CIFSR");
  wait_for_esp_response(1000);
}


void loop() {
  int ch_id, packet_len;
  unsigned long t1, t;
  char *pb;

  // send UDP packet
  Serial.println("UDP send");
  while(Serial1.available()) Serial1.read();  // flush responses
  t1=millis();
  Serial1.print("AT+CIPSEND=4,");
  Serial1.println(sizeof(query));
  wait_for_esp_response(1000);
  Serial1.write((uint8_t *)query,sizeof(query));
  
  if (!binfind("+IPD,4,48:")){
  	Serial.println("IPD timeout");
        delay(10000);
	return;
  }
  t1= millis()-t1;
  // parse out ntp reply  +IPD,4,48:xxxxxxxxxxxxx
  // ntp packet is network-byte order, big endian
  Serial1.readBytes(buffer,sizeof(query));
  unsigned long secsSince1900;
  secsSince1900 = (unsigned long)buffer[40] << 24;
  secsSince1900 |= (unsigned long)buffer[41] << 16;
  secsSince1900 |= (unsigned long)buffer[42] << 8;
  secsSince1900 |= (unsigned long)buffer[43];
  t = secsSince1900 - 2208988800UL + timeZone * 3600;
  Serial.print(t1); Serial.println(" ms");
  Serial.println(t);
  Serial.printf("%d/%d/%d %d:%d:%d\n",month(t),day(t),year(t),hour(t),minute(t),second(t));
  wait_for_esp_response(1000);
  delay(10000);
}


void setupWiFi() {

  // turn on echo
  Serial1.println("ATE1");
  wait_for_esp_response(1000);
  
  // set mode 1 (client)
  Serial1.println("AT+CWMODE=3");
  wait_for_esp_response(1000); 
 
  // reset WiFi module
  Serial1.print("AT+RST\r\n");
  wait_for_esp_response(1500);

   //join AP
  Serial1.print("AT+CWJAP=\"");
  Serial1.print(SSID);
  Serial1.print("\",\"");
  Serial1.print(PASS);
  Serial1.println("\"");
  wait_for_esp_response(5000);

  //Create UDP ports
  Serial1.println("AT+CIPMUX=1");
   wait_for_esp_response(1000);
  
  String cmd = "AT+CIPSTART=4,\"UDP\",\"";
  cmd += NTPSERVER;
  cmd += "\",";
  cmd += PORT;
  cmd += ",4321,0";
  Serial1.println(cmd);
   wait_for_esp_response(1000);
  
  query[0] = 0x1B;    // NTP request
  Serial.println("UDP ready");
}
