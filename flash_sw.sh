./fs.sh
esptool.py --baud 115200 --port /dev/ttyUSB0 -b 460800 write_flash --flash_mode dout 0x00000 /home/jooo/ESP8266_RTOS_SDK/bin/eagle.flash.bin 0x20000 /home/jooo/ESP8266_RTOS_SDK/bin/eagle.irom0text.bin
exit