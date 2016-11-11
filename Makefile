#
# Sample ESP8266 App
#
PROJ_NAME=app
COMPORT=/dev/ttyUSB0
OBJS=user_main.o uart.o
#
CC=xtensa-lx106-elf-gcc
APP=a.out
ESPTOOL_CK=esptool
# esptool.py is distributed with the pfalcon/esp-open-sdk in the xtensa-lx106 bin.
ESPTOOL=esptool.py
CCFLAGS= -Wimplicit-function-declaration -fno-inline-functions -mlongcalls -mtext-section-literals \
-mno-serialize-volatile -I$(ESP8266_SDK_ROOT)/include -I. -D__ETS__ -DICACHE_FLASH -DXTENSA -DUSE_US_TIMER

LDFLAGS=-nostdlib \
-L$(ESP8266_SDK_ROOT)/lib -L$(ESP8266_SDK_ROOT)/ld -T$(ESP8266_SDK_ROOT)/ld/eagle.app.v6.ld \
-Wl,--no-check-sections -u call_user_start -Wl,-static -Wl,--start-group \
-lc -lgcc -lhal -lphy -lpp -lnet80211 -llwip -lwpa -lmain -ljson -lupgrade -lssl \
-lpwm -lsmartconfig  $(OBJS) -Wl,--end-group

all: $(PROJ_NAME)_0x000000.bin $(PROJ_NAME)_0x010000.bin

a.out: $(OBJS)
	$(CC) -o a.out $(LDFLAGS)

$(PROJ_NAME)_0x000000.bin: a.out
	$(ESPTOOL_CK) -v -eo $< -bo $@ -bs .text -bs .data -bs .rodata -bs .iram0.text -bc -ec || true

$(PROJ_NAME)_0x010000.bin: a.out
	$(ESPTOOL_CK) -v -eo $< -es .irom0.text $@ -ec || true

try:
	esptool -eo a.out -bo $(PROJ_NAME)_0x000000_try.bin -bs .text -bs .data -bs .rodata -bc -ec \
		    -eo a.out -es .irom0.text $(PROJ_NAME)_0x001000_try.bin -ec

try1:
	esptool.py elf2image a.out

.c.o:
	$(CC) $(CCFLAGS) -c $<

clean:
	rm -f a.out *.o *.bin
	
flash: all
	esptool -cp $(COMPORT) -cd nodemcu \
		-ca 0x000000 -cf $(PROJ_NAME)_0x000000.bin \
		-ca 0x010000 -cf $(PROJ_NAME)_0x010000.bin 

flash1: all
	esptool -cp $(COMPORT) -cd nodemcu \
		-ca 0x000000 -cf $(ESP8266_SDK_ROOT)/bin/boot_v1.6.bin \
		-ca 0x07e000 -cf $(ESP8266_SDK_ROOT)/bin/blank.bin \
		-ca 0x3fc000 -cf $(ESP8266_SDK_ROOT)/bin/esp_init_data_default.bin \
		-ca 0x3fe000 -cf $(ESP8266_SDK_ROOT)/bin/blank.bin \
		-ca 0x001000 -cf $(PROJ_NAME)_0x010000.bin 

		
#$(ESPTOOL_CK) -cp $(COMPORT) -bm qio -bz 4M -bf 40 -cd nodemcu -cb 115200 -ca 0x00000 -cf $(PROJ_NAME)_0x00000.bin -ca 0x40000 -cf $(PROJ_NAME)_0x40000.bin
#(ESPTOOL) --port $(COMPORT) --baud 115200 write_flash --flash_freq 40m --flash_mode qio --flash_size 4m 0x00000 $(PROJ_NAME)_0x00000.bin 0x40000 $(PROJ_NAME)_0x40000.bin

		
at:
	esptool -cp $(COMPORT) -cd nodemcu \
		-ca 0x000000 -cf $(ESP8266_SDK_ROOT)/bin/boot_v1.6.bin \
		-ca 0x001000 -cf $(ESP8266_SDK_ROOT)/bin/at/512+512/user1.1024.new.2.bin \
		-ca 0x07e000 -cf $(ESP8266_SDK_ROOT)/bin/blank.bin \
		-ca 0x3fc000 -cf $(ESP8266_SDK_ROOT)/bin/esp_init_data_default.bin \
		-ca 0x3fe000 -cf $(ESP8266_SDK_ROOT)/bin/blank.bin 
	
	

		