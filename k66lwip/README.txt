   building lwIP for teensy K66 and proto Ethernet shield
      proof-of-concept, TCP/UDP examples with raw API (polling and callbacks)

teensy makefile template    https://github.com/apmorton/teensy-template
   make 
   make clean
   make upload  (ide loader running)   may need to push button or close monitor

   make -n  will show what make would do (doesn't actually do it)
    could do make -f makefile.tom

setup: mkdir src build libraries    link teensy3/ tools/
  lwiptst.ino   or  main.cpp into src
 ln -s /u1/home/linux/arduino-1.6.9/hardware/teensy/avr/cores/teensy3
 ln -s /u1/home/linux/arduino-1.6.9/hardware/tools
 mv /u1/home/linux/arduino-1.6.9/hardware/teensy/avr/cores/teensy3/main.cpp /u1/home/linux/arduino-1.6.9/hardware/teensy/avr/cores/teensy3/main.cpp.0
   (this breaks later use of IDE ??)
   get rid of main.cpp and use daxdotf.ino in src

 include(.h) order -I :  src  core   libraries

other  hardware/teensy/cores/teensy3/Makefile main.cpp in situ

replace LC with 35 in Makefile

make
[HEX]	k66lwip.hex
   text	   data	    bss	    dec	    hex	filename
  17480	   2292	   2680	  22452	   57b4	k66lwip.elf


make upload
Teensy did not respond to a USB-based request to automatically reboot.
Please press the PROGRAM MODE BUTTON on your Teensy to upload your sketch.
  have blink   7/17/16

 daxdotf.ino instead of main.cpp in src/
 reps 1000  vec lth 400
  ip 37  sax 40
  0.02

  fix RTC localtime in makefile

  cd libraries/
  ln -s /home/dunigan/sketchbook/libraries/BigNumber
  cd src
  rm daxdotf.ino
  ln -s /home/dunigan/sketchbook/perf/Factorials/Factorials.ino
  make
  make upload
    still have to push button to upload
  factorial ok 
    4659
	9332621544394415...

lwip-08f08bfc3f3d.tar.gz   lwip 1.4.0
  lwipk66/ has K66 ether interface software
  lwip/lwipopts.h  lwip config settings

   add lwip stuff to Makefile, order of includes is important
   don't need API sources in makefile

discussions: https://forum.pjrc.com/threads/34808-K66-Beta-Test?p=109161&viewfull=1#post109161
