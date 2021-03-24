#include <ArduinoJson.h>

void readConfig();
void writeConfig();
void dumpFileToSerial();
String getFileContents(String fName);

// config settings
String mqtt_server = "unknown.address";
int    mqtt_port = 1883;
String mqtt_username = "username";
String mqtt_password = "password";
String device_nickname = "unknown.sensor";
int    update_frequency = 10000;

String system_status = "STARTING";

boolean configIsValid = false;
boolean configHasChanged = false;

void setStatus(String status) {
  Serial.println(status);
  system_status = status;
}

// stuff for a simple webserver
ESP8266WebServer webserver(80);
void handleRoot();
void handleConfigForm();
void handleUpdateConfig();
void handleNotFound();
void handleDir();

void setUpWebServer(){
  webserver.on("/", handleRoot);
  webserver.on("/dir", handleDir);
  webserver.on("/config-form", handleConfigForm);
  webserver.on("/update-config", handleUpdateConfig);
  webserver.onNotFound(handleNotFound);
  webserver.begin();
}

void handleRoot() {
  String o = getFileContents("home.html");
  o.replace("%ssid%", WiFi.SSID());
  o.replace("%mqtt_server%", mqtt_server);
  o.replace("%mqtt_username%", mqtt_username);
  o.replace("%mqtt_password%", mqtt_password);
  o.replace("%device_nickname%", device_nickname);
  o.replace("%status%", system_status);
  webserver.send(200, "text/html", o);
}

void handleConfigForm() {
  String o = getFileContents("config-form.html");
  o.replace("%mqtt_server%", mqtt_server);
  o.replace("%mqtt_username%", mqtt_username);
  o.replace("%mqtt_password%", mqtt_password);
  o.replace("%device_nickname%", device_nickname);
  webserver.send(200, "text/html", o);
}

void handleUpdateConfig() {
  mqtt_server = webserver.arg("mqtt_server");
  mqtt_username = webserver.arg("mqtt_username");
  mqtt_password = webserver.arg("mqtt_password");
  device_nickname = webserver.arg("device_nickname");
  writeConfig();
  webserver.sendHeader("Location", "/");
  webserver.send(303);
  configHasChanged = true;
}

/* Only does the root level */
void handleDir() {
  String o = "";
  Serial.println("handleDir");
  Dir dir = LittleFS.openDir("/");
  while (dir.next()) {
    o += dir.fileName();
    o += "\n";
  }
  webserver.send(200, "text/plain", o);
}

void handleNotFound() {
  webserver.send(404, "text/plain", "Your file is not here! (" + webserver.uri() + ")");
}

void dumpFileToSerial(String fName) {
  Serial.println(getFileContents(fName));
}

String getFileContents(String fName) {
  String o = "";
  File f = LittleFS.open(fName, "r");
  while (f.available()) {
    o += f.readString();
  }
  f.close();
  return o;
}

void readConfig() {
  if (!LittleFS.exists("config.json")) {
    //writeConfig();
    return;
  }
  File f = LittleFS.open("config.json", "r");
  DynamicJsonDocument doc(1024);
  DeserializationError err = deserializeJson(doc, f);
  if (err) {
    Serial.print(F("deserializeJson failed: "));
    Serial.println(err.f_str());
  } else {
    mqtt_server = doc["mqtt_server"].as<String>();
    mqtt_username = doc["mqtt_username"].as<String>();
    mqtt_password = doc["mqtt_password"].as<String>();
    device_nickname = doc["device_nickname"].as<String>();
    configIsValid = true;
  }
  f.close();
}

void writeConfig() {
  DynamicJsonDocument doc(1024);
  doc["mqtt_server"] = mqtt_server;
  doc["mqtt_username"] = mqtt_username;
  doc["mqtt_password"] = mqtt_password;
  doc["device_nickname"] = device_nickname;
  File f = LittleFS.open("config.json", "w");
  serializeJson(doc, f);
  f.close();
  configIsValid = true;
}
