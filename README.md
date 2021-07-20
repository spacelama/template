# template
Incorporate into all your esp8266 projects so you can compile/flash from commandline, primary/failover WPA connection with monitoring and reset, syslog, OTA updates

Compile on the commandline with https://github.com/plerup/makeEspArduino.git Currently works only up until commit fb748959db7b69f9353b6c052d0471d730cd6830 (HEAD, tag: 5.4.3), because of bug https://github.com/plerup/makeEspArduino/issues/155

Ensure the arduino GUI has downloaded any libraries you depend on.

Download this git repo and copy template/Makefile.customise-and-move-to-Makefile to template/Makefile, customising the first 8 or so configuration parameters.

For esp8266 projects, for the time being, use the master branch, and for esp32 projects, use the esp32 branch.

First time around, you probably don't have an OTA-ready image flashed on your esp8266 board - so set .DEFAULT_GOAL := flash in Makefile. After the first successful flash, you'll probably find it more convenient to set that back to .DEFAULT_GOAL := ota.

For maximum wifi reliability, I have a set of Openwrt Acesss Points scattered around the house.  All of them answer on a common 2.4GHz SSID, and each of them have a dedicated 2.4GHz SSID as well (as well as SSIDs for all my other VLANs, 5GHz, etc).  Each device that uses this repo then associates primarily to the main common SSID, with failover configured to the second closest AP. That way, if one AP goes bad and can't talk to upstream, the monitoring on the ESP8266 decice will notice that it can't talk to the router, then failover to the second closest device.  If that then becomes marginal or loses contact with the router, it will failback over to the primary common AP, and will choose the highest quality signal.  It will keep doing this until it succeeds to ping the router.  Monitoring on my main network will pick up when one of the devices is on the failover network via inspecting the wifi field of "http://$iot/uptime", and may decide to trigger "http://$iot/failover_wifi" if it knows the network situation is resolved.
Ping monitoring does not yet work in esp32 version.

Run make
