# template
Incorporate into all your esp8266 projects so you can compile/flash from commandline, primary/failover WPA connection with monitoring and reset, syslog, OTA updates

## Instructions
Compile on the commandline with https://github.com/plerup/makeEspArduino.git Currently works only up until commit fb748959db7b69f9353b6c052d0471d730cd6830 (HEAD, tag: 5.4.3), because of bug https://github.com/plerup/makeEspArduino/issues/155

Ensure the arduino GUI has downloaded any libraries you depend on.

Download this git repo and copy template/Makefile.customise-and-move-to-Makefile to template/Makefile, customising the first 8 or so configuration parameters.

For esp8266 projects, for the time being, use the master branch, and for esp32 projects, use the esp32 branch.

Configure the Makefile in the project you want to import this template repo into. eg:

```
ESP_ADDR=netclock

include $(HOME)/Arduino/template/Makefile

#UPLOAD_PORT = /dev/tty-USB-locked-down-by-udev

.DEFAULT_GOAL := ota
#.DEFAULT_GOAL := flash
```
and symlink esp8266-compat.h (if using esp32) and template.cpp and template.h from the template repo:
```
~/Arduino/esp32-cam$ ln -s ../template32/template.cpp
~/Arduino/esp32-cam$ ln -s ../template32/template.h
~/Arduino/esp32-cam$ ln -s ../template32/esp8266-compat.h
```


A much more complicated Makefile might be needed on more specialised projects:
```
ESP_ADDR=esp32cam
# default anyway: ESP_PORT=3232

#FIXME: if BOARD = esp32cam, we get a compile error:
#/home/tconnors/.arduino15/packages/esp32/hardware/esp32/1.0.6/cores/esp32/esp32-hal-misc.c:199:29: error: expected expression before '/' token
#     setCpuFrequencyMhz(F_CPU/1000000);
# We sort of hack around that by defining -DF_CPU=240000000L manually below:

#BOARD = lolin32
BOARD = esp32cam
# ttgo-lora32-v1
# esp32
CHIP = esp32
# on the lolin32 board (but not my other esp32 boards), flashing at full 921600 doesn't start - with communication error.  likely hardware fix is this, or just upload slower: https://randomnerdtutorials.com/solved-failed-to-connect-to-esp32-timed-out-waiting-for-packet-header/
UPLOAD_SPEED = 460800

# EXCLUDE_DIRS=$(wildcard $(ARDUINO_LIBS)/*/tests) $(ARDUINO_LIBS)/ESP8266SdFat $(ARDUINO_LIBS)/SDFS
EXCLUDE_DIRS=XXXXXXXXXXXXXXXX

include $(HOME)/Arduino/template/Makefile
BUILD_EXTRA_FLAGS:=$(BUILD_EXTRA_FLAGS) -DF_CPU=240000000L

UPLOAD_PORT = /dev/ttyUSB0
.DEFAULT_GOAL := ota
#.DEFAULT_GOAL := flash
```

First time around, you probably don't have an OTA-ready image flashed on your esp8266 board - so set .DEFAULT_GOAL := flash in Makefile. After the first successful flash, you'll probably find it more convenient to set that back to .DEFAULT_GOAL := ota.

Run make

## Using
For maximum wifi reliability, I have a set of Openwrt Acesss Points scattered around the house.  All of them answer on a common 2.4GHz SSID, and each of them have a dedicated 2.4GHz SSID as well (as well as SSIDs for all my other VLANs, 5GHz, etc).  Each device that uses this repo then associates primarily to the main common SSID, with failover configured to the second closest AP. That way, if one AP goes bad and can't talk to upstream, the monitoring on the ESP8266 device will notice that it can't talk to the router, then failover to the second closest device.  If that then becomes marginal or loses contact with the router, it will failback over to the primary common AP, and will choose the highest quality signal.  It will keep doing this until it succeeds to ping the router.  Monitoring on my main network will pick up when one of the devices is on the failover network via inspecting the wifi field of "http://$iot/uptime", and may decide to trigger "http://$iot/failover_wifi" if it knows the network situation is resolved.
Ping monitoring does not yet work in esp32 version.

