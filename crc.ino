// teensy CRC hardware vs software
//  see FastCRC lib for robust implementation, benchmarks, validation
// poly=0x04c11db7 init=0xffffffff refin=true refout=true xorout=0xffffffff check=0xcbf43926

#define PRREG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)

#define CRC_CTRL_TCRC_MASK                       (0x1000000U)
#define CRC_CTRL_TCRC_SHIFT                      (24U)
#define CRC_CTRL_TCRC(x)                         (((uint32_t)(((uint32_t)(x)) << CRC_CTRL_TCRC_SHIFT)) & CRC_CTRL_TCRC_MASK)
#define CRC_CTRL_WAS_MASK                        (0x2000000U)
#define CRC_CTRL_WAS_SHIFT                       (25U)
#define CRC_CTRL_WAS(x)                          (((uint32_t)(((uint32_t)(x)) << CRC_CTRL_WAS_SHIFT)) & CRC_CTRL_WAS_MASK)
#define CRC_CTRL_FXOR_MASK                       (0x4000000U)
#define CRC_CTRL_FXOR_SHIFT                      (26U)
#define CRC_CTRL_FXOR(x)                         (((uint32_t)(((uint32_t)(x)) << CRC_CTRL_FXOR_SHIFT)) & CRC_CTRL_FXOR_MASK)
#define CRC_CTRL_TOTR_MASK                       (0x30000000U)
#define CRC_CTRL_TOTR_SHIFT                      (28U)
#define CRC_CTRL_TOTR(x)                         (((uint32_t)(((uint32_t)(x)) << CRC_CTRL_TOTR_SHIFT)) & CRC_CTRL_TOTR_MASK)
#define CRC_CTRL_TOT_MASK                        (0xC0000000U)
#define CRC_CTRL_TOT_SHIFT                       (30U)
#define CRC_CTRL_TOT(x)                          (((uint32_t)(((uint32_t)(x)) << CRC_CTRL_TOT_SHIFT)) & CRC_CTRL_TOT_MASK)

#define BUFSIZE 16384
uint8_t buf[BUFSIZE] __attribute__((aligned(4)));


// bit-bang reverse of crc32 ethernet (04C11DB7)  0xEDB88320
// used by k64 for multicast mac hash
// test vector should be  flip of 0xcbf43926  same as forward

uint8_t ip[] = {224, 7, 8, 9}, mac[] = {1, 0, 0x5e, 7, 8, 9};
uint8_t check[9] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};

uint32_t crcbb(uint8_t *address, uint32_t bytes) {
  uint32_t crc = 0xFFFFFFFFU;
  uint32_t count1 = 0;
  uint32_t count2 = 0;

  /* Calculates the CRC-32 polynomial on the multicast group address. */
  for (count1 = 0; count1 < bytes; count1++)
  {
    uint8_t c = address[count1];
    for (count2 = 0; count2 < 0x08U; count2++)
    {
      if ((c ^ crc) & 1U)
      {
        crc >>= 1U;
        c >>= 1U;
        crc ^= 0xEDB88320U;
      }
      else
      {
        crc >>= 1U;
        c >>= 1U;
      }
    }
  }
  return crc;
}

#define POLY  0x04c11db7
#define SEED  0xffffffff
#define FLAGS CRC_CTRL_FXOR(1) | CRC_CTRL_TOT(2) | CRC_CTRL_TOTR(2)

#define CRC8 *(uint8_t *)&CRC_CRC
#define CRC16 *(uint16_t *)&CRC_CRC

void crc_init() {
  SIM_SCGC6 |= SIM_SCGC6_CRC;
}

uint32_t crc(const uint8_t *data, const uint16_t datalen) {
  const uint8_t *src = data;
  const uint8_t *target = src + datalen;

  CRC_CTRL = FLAGS | CRC_CTRL_TCRC(1) | CRC_CTRL_WAS(1);  // prep for seed
  CRC_GPOLY = POLY;
  CRC_CRC = SEED;
  CRC_CTRL = FLAGS | CRC_CTRL_TCRC(1);    // prep for data

  while (((uintptr_t)src & 0x03) != 0  && (src < target)) {
    CRC8 = *src++; //Write 8 BIT
  }

  while (src <= target - 4) {
    CRC_CRC = *( uint32_t  *)src; //Write 32 BIT
    src += 4;
  }

  while (src < target) {
    CRC8 = *src++; //Write 8 Bit
  }

  return CRC_CRC;
}

void setup() {
  char str[64];
  Serial.begin(9600);
  while (!Serial);
  sprintf(str, "F_CPU %d MHz  %d bytes", F_CPU / 1000000, BUFSIZE);
  Serial.println(str);
  for (int i = 0; i < BUFSIZE; i++) buf[i] = (i + 1) & 0xff;
  crc_init();
  Serial.println(crcbb(mac, 6), HEX); //k64 ether mac filter hash
  Serial.println(~crc(mac, 6), HEX); // flip bits
  Serial.println(crcbb(check, 9), HEX);
  Serial.println(crc(check, 9), HEX);
  Serial.println(crcbb(buf, BUFSIZE), HEX);
  Serial.println(crc(buf, BUFSIZE), HEX);
  PRREG(CRC_GPOLY);
  PRREG(CRC_CTRL);
}

void loop() {
  uint32_t t, c;
  char str[64];

  t = micros();
  c = crcbb(buf, BUFSIZE);
  t = micros() - t;
  sprintf(str, "bitbang %d us %.3f mbs  0x%x", t, 8.*BUFSIZE / t, c);
  Serial.println(str);
  t = micros();
  c = crc(buf, BUFSIZE);
  t = micros() - t;
  sprintf(str, "hardware %d us %.3f mbs  0x%x", t, 8.*BUFSIZE / t, c);
  Serial.println(str);
  delay(2000);
}
