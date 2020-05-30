// random RNG

#define REPS 1000

#define RNG_CR_GO_MASK                           0x1u
#define RNG_CR_HA_MASK                           0x2u
#define RNG_CR_INTM_MASK                         0x4u
#define RNG_CR_CLRI_MASK                         0x8u
#define RNG_CR_SLP_MASK                          0x10u
#define RNG_SR_OREG_LVL_MASK                     0xFF00u
#define RNG_SR_OREG_LVL_SHIFT                    8
#define RNG_SR_OREG_LVL(x)                       (((uint32_t)(((uint32_t)(x))<<RNG_SR_OREG_LVL_SHIFT))&RNG_SR_OREG_LVL_MASK)
#define SIM_SCGC6_RNGA    ((uint32_t)0x00000200)

uint32_t trng() {
  while ((RNG_SR & RNG_SR_OREG_LVL(0xF)) == 0); // wait
  return RNG_OR;
}

void setup() {
  Serial.begin(9600);
  SIM_SCGC6 |= SIM_SCGC6_RNGA; // enable RNG  0x40029000
  RNG_CR &= ~RNG_CR_SLP_MASK;
  RNG_CR |= RNG_CR_HA_MASK;  // high assurance, not needed
  RNG_CR |= RNG_CR_GO_MASK;        // ? only need to do this once?
}

#if 1
// display
void loop() {
  uint32_t t, r;
  int i;

  t = micros();
  for (i = 0; i < REPS; i++) r = trng();
  t = micros() - t;
  float bps = REPS * 32.e6 / t;
  Serial.println(bps, 2);
  Serial.println(r, HEX);

  delay(2000);
}

#endif

#if 0
// logger
void loop() {
  // await start byte from host then start sending random numbers
  unsigned long rng;
  while (!Serial.available()) {} // wait for byte
  Serial.read();
  while (1) {
    rng = trng();
    Serial.write((uint8_t *)&rng, 4);
  }
}
#endif

#if 0
//  quick entropy check  expect 8 bits entropy  0.5 1s  127.5
uint32_t bits[8], cnts[256], bytes;

void dobyte(uint8_t byte) {
  bytes++;
  cnts[byte]++;
  for (int j = 0; j < 8; j++) if ( byte &  1 << j) bits[j]++;
}

void loop() {
  static uint32_t ms = millis();
  uint32_t r = trng();
  uint8_t *b = (uint8_t *)  &r;

  for (int i = 0; i < 4; i++) dobyte(b[i]);
  if (millis() - ms > 5000) {
    float avrg = 0, p, e = 0;
    ms = millis();
    for (int i = 0; i < 256; i++) {
      avrg += 1.0 * i * cnts[i];
      if (cnts[i]) {
        p = (float) cnts[i] / bytes;
        e -= p * log(p) / log(2.0);
      }
    }
    Serial.printf("%d bytes avrg %f entropy %f\n", bytes, avrg / bytes, e);
    for (int j = 0; j < 8; j++) {
      Serial.print((float) bits[j] / bytes, 6);
      Serial.print(" ");
    }
    Serial.println();
  }
}
#endif


