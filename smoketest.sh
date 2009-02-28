#! /bin/bash

#  $Id$
#  $URL$


rm -f smoketest.log lcd4linux

make distclean
./bootstrap

for driver in BeckmannEgle BWCT CrystalFontz Curses Cwlinux D4D EA232graphic G15 GLCD2USB HD44780 IRLCD LCD2USB LCDLinux LCDTerm LEDMatrix LPH7508 LUIse M50530 MatrixOrbital MatrixOrbitalGX MilfordInstruments Noritake NULL Pertelian PHAnderson picoLCD picoLCDGraphic PNG PPM RouterBoard Sample serdisplib SimpleLCD T6963 Trefon ULA200 USBHUB USBLCD WincorNixdorf X11; do

    make distclean
    ./configure --with-drivers=$driver
    make -s -j 2
    
    if [ -x lcd4linux ]; then
	echo "Success: drv_$driver" >>smoketest.log
    else
	echo "FAILED:  drv_$driver" >>smoketest.log
    fi
    
done

for plugin in apm asterisk button_exec cpuinfo diskstats dvb exec fifo file hddtemp i2c_sensors iconv imon isdn kvv loadavg meminfo mpd mysql netdev netinfo pop3 ppp proc_stat python sample seti statfs uname uptime w1retap wireless xmms; do

    make distclean
    ./configure --with-drivers=NULL --with-plugins=$plugin
    make -s -j 2
    
    if [ -x lcd4linux ]; then
	echo "Success: plugin_$plugin" >>smoketest.log
    else
	echo "FAILED:  plugin_$plugin" >>smoketest.log
    fi
    
done

make distclean
./configure
