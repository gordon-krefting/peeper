#Peeper
A bunch of sensor that can send data to my Home Assistant instance
##Notes
A bunch of stuff happening here:
- Using wifimanager.h to autoprovision a Wi-Fi Connection.
  - On very first use, the app will start a Wi-Fi AP called "NodeMCU Setup"
  - Connect to it, then browse to 192.168.4.1 to pick a network and enter password
  - At the moment, to choose a different network, we've got to move out of range or wipe the flash memory (using Arduino IDE)
  - After successful connection, the device IP address is logged (or look in the router client screen)
- Using ESP8266WebServer to set up a web app to collect additional config parameters
  - Web server is sitting on port 80 of the device IP
  - There are templates in the _ino_/data directory
- Using ArduinoJson.h to serialize/deserialize a config file, also stored in _ino_/data
- Using LittleFS, the standard ESP8266 filesystem
  - Unfortunately, you have to use a plugin in the Arduino IDE to upload the web server template files... this wipes the config file as well
  - [Plugin](https://github.com/earlephilhower/arduino-esp8266littlefs-plugin)
  - There is a way to do this from command line using esptools.py (which gets installed with the board support), but I can't work out the command line yet
  - I don't think there's a good way to leave the config file intact while uploading templates. There's no random access to LittleFS from outside the board, the plugin creates the whole file system and uploads it in one go







