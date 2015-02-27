//http://forum.pjrc.com/threads/25168-wiznet-820i-on-Teensy-3-1-getMACaddress%28%29
//http://forum.pjrc.com/threads/91-teensy-3-MAC-address



// Everything on the network needs a unique MAC
// Each Teensy3 has a unique MAC burned in
static byte mac[6];
char mac_string[20];		 //string to hold MAC address for Bonjour name
void read(uint8_t word, uint8_t *mac, uint8_t offset) {
  FTFL_FCCOB0 = 0x41;             // Selects the READONCE command
  FTFL_FCCOB1 = word;             // read the given word of read once area

// launch command and wait until complete
  FTFL_FSTAT = FTFL_FSTAT_CCIF;
  while(!(FTFL_FSTAT & FTFL_FSTAT_CCIF));

  *(mac+offset) =   FTFL_FCCOB5;       // collect only the top three bytes,
  *(mac+offset+1) = FTFL_FCCOB6;       // in the right orientation (big endian).
  *(mac+offset+2) = FTFL_FCCOB7;       // Skip FTFL_FCCOB4 as it's always 0.
}
void read_mac() {
  read(0xe,mac,0);
  read(0xf,mac,3);
}

void print_mac()  {
  size_t count = 0;
  for(uint8_t i = 0; i < 6; ++i) {
    if (i!=0) count += Serial.print(":");
    count += Serial.print((*(mac+i) & 0xF0) >> 4, 16);
    count += Serial.print(*(mac+i) & 0x0F, 16);
  }
  sprintf(mac_string, "%02X:%02X:%02X:%02X:%02X:%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  Serial.println();
}


void setup()
{
  Serial.begin(9600); 
  while(!Serial);
  Serial.println("Reading MAC from hardware...");
  read_mac();
  Serial.print("MAC: ");
  print_mac();
  Serial.println();
}

void loop()
{
}
