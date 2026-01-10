#!/bin/bash
#on gitpod
#cd /workspace/esp32-nmea2000-obp60 
#bash lib/obp60task/run_install_tools 
#bash lib/obp60task/run_obp60_s3 

#locally:
#Update Libs 
#~/.platformio/penv/bin/pio pkg update

~/.platformio/penv/bin/pio run

for i in .pio/build/obp60_s3/obp60_s3-*-update.bin ;do cp -v $i ~/OBP-Images/`basename $i .bin``date +%H%M`.bin; done
