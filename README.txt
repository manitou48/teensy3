Teensy 3.* sketches and such    https://github.com/manitou48/teensy3

Ethernet.h    W5200 mods

w5100.cpp     W5200 mods

w5100.h       W5200 mods

w5100.cpp.fat W5200 mods + SdFat SPI mods

spiperf.ino   SdFat SPI unconnected tester

spiDMA.ino   SPI + DMA unconnected tester,  work in progress?

mem2mem.pde   DMA memcpy test
mem2memisr.pde   DMA memcpy test with ISR

vreftst.pde   check Vcc voltage against internal vref

lowvolt.ino   low voltage ISR  LVD LVW

chiptemp.pde  read temperature from internal sensor on analog channel 38

rtc.ino       RTC pps interrupt to measure crystal frequency
rtc_alarm.ino demo sketch of RTC alarm
RTCms.ino     get millisecond precision from the RTC

wizfat.ino    hack to test SDfat SPI with W5200

wizpaul.ino    hack to test Paul's SPIFIFO from Ethernet lib with W5200

wizwww.ino     WebServer variation with forms, pin toggle

LCperf.txt     beta test results of teensy LC

spi1perf.ino   LC SPI1 tester

etherraw.ino   K66 beta test of proto ethernet shield

k66lwip/       K66 beta with proto ethernet shield and lwIP

cryptolib.ino  K66 CAU example

mtalk.ino      multicast chat for wiznet

crc.ino        CRC hardware test vs bit-bang

wdogduff.ino   teensy 3* watchdog test

dacpong.ino    DAC PDB ping-pong DMA buffers

isrperfpwm.ino ISR/attachInterrupt latency tester 

uartdma        ping-pong DMA of UART receive

whetstone.ino  double and single whetstone benchmark

lptmrcnt.ino   like FreqCount but ISR for rollover

rng.ino         T3.5/T3.6 TRNG random numbers

ftmpcapture.ino T3.2 FTM0 timer capture period

gpsfreq.ino     GPS PPS capture FreqMeasure

k66TPM.ino      T3.6 TPM timer tests

dacPID.ino      T3.2 PID controller using DAC

cmpperf.ino     T3.2 comparator example
--------
Some performance comparisons at

   https://github.com/manitou48/DUEZoo
   https://github.com/manitou48/crystals   teensy 3 RTC tuning
