# ESP8266-RFID-to-REST
Simple but generic standalone RFID reader, posting identified UIDs to any upstream web server for less than 5 bucks (USD / EUR).

Idea: Keep the device "as dumb" and stateless as possible... Any logic to parse UIDs etc can sit on a (better secured) server.
WiFi password is not disclosed on serial port, i.w. without unsoldering the EEPROM and reading it externally you should be safe, even if your RFID reader gets captured. Normally UID of cards / tags cannot be copied, so at least for basic security this should be sufficient.

Use case: Using this for FHEM for home automation. Based on detected card, certain activities can be triggered.

## Hardware and Shopping List
1) Search aliexpress for "nodemcu ESP-12E CH340". For example: https://www.aliexpress.com/item/New-Wireless-module-NodeMcu-Lua-WIFI-Internet-of-Things-development-board-based-ESP8266-with-pcb-Antenna/32656775273.html - Approx 3€.
2) Search aliexpress for "RFID-RC522". For example: https://www.aliexpress.com/item/MFRC-522-IC-card-sensing-module-RFID-Radio-frequency-RF-S50-IC-card-Key-chain-RFID/32613929702.html - Approx 2€.
3) Some wire, maybe a push button, a nice enclosure ;-)


## Configure
Via USB CH340 Serial Port, the configuration can be easily updated:
ssid=<YourSSID>
pass=<YourWLANPassword>
url=<YourURLForPOST>     - must be style like http://foo.bar.com:12345/exx")
connect                  - Attempt to connect to WIFI. Only call after updating SSID and pass


Once a card is detected, the string "uid=<UID>" is POSTed to the endpoint configured.

## Setup
You have to install the Arduino IDE 1.6.7+.
* **Arduino** > **Preferences** > "Additional Boards Manager URLs:" and add: **http://arduino.esp8266.com/package_esp8266com_index.json**
* **Arduino** > **Tools** > **Board** > **Boards Manager** > type in **ESP8266** and install the board
* download MFRC522 module (see [Libraries](#libraries)) via Library Manager

## Libraries
* [RFID library by Miguel Balboa](https://github.com/miguelbalboa/rfid)

## Wiring
```
MISO - GPIO12 (hw spi)
MOSI - GPIO13 (hw spi)
SCK  - GPIO14 (hw spi)
SS   - GPIO04 (free GPIO)
RST  - GPIO05 (free GPIO)
```

In addition, a button is supported
```
D3   - Internal Pullup - Connect to GROUND via button switch
```
If the button is pressed, "uid=button1" is posted.


## FHEM
Hint: Using this on FHEM side via hack:
`fhem.cfg`
```
# Start background process to monitor RFID scanner
{system("socat TCP4-LISTEN:8989,fork EXEC:/home/pi/custom/rfidPostListener.sh &")}

define rfid_04:F2:C0:... dummy
define rfid notify rfid_04.* {\
  ... \
}
```

`rfidPostListener.sh`
```
#!/bin/bash

while read -r -t 0.2 line; do
  #echo $line >&2
  if [[ $line == uid=* ]]; then
    echo "RFID $line" >&2
    echo "trigger rfid_${line/uid=/}" |nc localhost 7072 >&2
  fi
done

echo "HTTP/1.0 200 OK"
echo ""
```
=======
Arduino sketch for NodeMCU (ESP8266) and RFID-RC522 reader, POSTing identified cards to arbitrary POST endpoint via WiFi. Configurable via serial connection. May be used, for example, with FHEM to trigger actions based on scanned cards or tags,


## Limitations
- Data is not transmitted SSL protected. So within network, sniffer may be used to capture valid UIDs and replay.
