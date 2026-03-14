#!/bin/bash
#Update libs 
#~/.platformio/penv/bin/pio pkg update
#clean all libs
#~/.platformio/penv/bin/pio run -t fullclean

~/.platformio/penv/bin/pio run -e obp60_s3

for i in .pio/build/obp60_s3/obp60_s3-*-update.bin ;do cp -v $i ~/OBP-Images/`basename $i .bin``date +%H%M`.bin; done
