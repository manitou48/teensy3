// duff from https://forum.pjrc.com/threads/25370-Teensy-3-0-Watchdog-Timer
//  LPO @1khz 10% err,  4s WDOT timer
#define PRREG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  // You have about 4 secs to open the serial monitor before a watchdog reset
  while (!Serial);
  delay(100);
  printResetType();
  KickDog();
  PRREG(WDOG_STCTRLH);
  PRREG(WDOG_PRESC);
  PRREG(WDOG_TOVALL);
  PRREG(WDOG_TOVALH);
  PRREG(WDOG_RSTCNT);
  for (int i = 1; i <= 5; i++) {
    Serial.print("loop "); Serial.println(i);
    KickDog();
    delay(1000);
  }
  Serial.println("long delay reset");
  delay(5000);
}

void loop() {
  Serial.println("never gets here ...");
  delay(3000);
}

void KickDog() {
  Serial.println("Kicking the dog!");
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  noInterrupts();
  WDOG_REFRESH = 0xA602;
  WDOG_REFRESH = 0xB480;
  interrupts();
}

void printResetType() {
  if (RCM_SRS1 & RCM_SRS1_SACKERR)   Serial.println("[RCM_SRS1] - Stop Mode Acknowledge Error Reset");
  if (RCM_SRS1 & RCM_SRS1_MDM_AP)    Serial.println("[RCM_SRS1] - MDM-AP Reset");
  if (RCM_SRS1 & RCM_SRS1_SW)        Serial.println("[RCM_SRS1] - Software Reset");
  if (RCM_SRS1 & RCM_SRS1_LOCKUP)    Serial.println("[RCM_SRS1] - Core Lockup Event Reset");
  if (RCM_SRS0 & RCM_SRS0_POR)       Serial.println("[RCM_SRS0] - Power-on Reset");
  if (RCM_SRS0 & RCM_SRS0_PIN)       Serial.println("[RCM_SRS0] - External Pin Reset");
  if (RCM_SRS0 & RCM_SRS0_WDOG)      Serial.println("[RCM_SRS0] - Watchdog(COP) Reset");
  if (RCM_SRS0 & RCM_SRS0_LOC)       Serial.println("[RCM_SRS0] - Loss of External Clock Reset");
  if (RCM_SRS0 & RCM_SRS0_LOL)       Serial.println("[RCM_SRS0] - Loss of Lock in PLL Reset");
  if (RCM_SRS0 & RCM_SRS0_LVD)       Serial.println("[RCM_SRS0] - Low-voltage Detect Reset");
}

#ifdef __cplusplus
extern "C" {
void startup_early_hook();
}
extern "C" {
#endif
  void startup_early_hook() {
#if 1
    // clock source 0 LPO 1khz, 4 s timeout
    WDOG_TOVALL = 4000; // The next 2 lines sets the time-out value. This is the value that the watchdog timer compare itself to.
    WDOG_TOVALH = 0;
    WDOG_STCTRLH = (WDOG_STCTRLH_ALLOWUPDATE | WDOG_STCTRLH_WDOGEN | WDOG_STCTRLH_WAITEN | WDOG_STCTRLH_STOPEN); // Enable WDG
#else
    // bus clock  4s timeout
    uint32_t ticks = 4000 * (F_BUS / 1000); // ms
    WDOG_TOVALL = ticks & 0xffff; // The next 2 lines sets the time-out value. This is the value that the watchdog timer compare itself to.
    WDOG_TOVALH = (ticks >> 16) & 0xffff;
    WDOG_STCTRLH = (WDOG_STCTRLH_ALLOWUPDATE | WDOG_STCTRLH_CLKSRC | WDOG_STCTRLH_WDOGEN | WDOG_STCTRLH_WAITEN | WDOG_STCTRLH_STOPEN);
#endif
    WDOG_PRESC = 0; // prescaler
  }
#ifdef __cplusplus
}
#endif
