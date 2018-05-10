ID="$1"
LANG="$2"
./version.sh
./param.sh ${ID} ${LANG}
./fs.sh
echo ${ID} ${LANG}
esptool.py --baud 115200 --port /dev/ttyUSB0 -b 460800 write_flash --flash_mode dout 0x00000 /home/jooo/ESP8266_RTOS_SDK/bin/eagle.flash.bin 0x20000 /home/jooo/ESP8266_RTOS_SDK/bin/eagle.irom0text.bin  0x100000 /home/jooo/ESP8266_RTOS_SDK/bin/spiffs-image.bin
exit