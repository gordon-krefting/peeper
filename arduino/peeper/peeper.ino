#include <DHT.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include "config.h"

/*
 * Note: ModeMCU stores wi-fi connection deets in a memory location that isn't 
 * overwritten by uploading new code.
 * 
 * Uses WiFi manager to connect to an access point. When the app starts, the
 * manager will look for previous connection info. If there isn't any, it will
 * spin up an access point ("NodeMCU Setup") and a web server at 192.168.4.1.
 * A user (me!) can connect to the access point and choose a real access point
 * and enter a password.
 * 
 * TODO: force resending the mqtt config when the app config changes
 * TODO: re-learn C++!!! I'm not very happy with the layout of this app....
 * TODO: error handling (on the form especially...)
 * TODO: mDNS
 */
#define DHTPIN D2
#define DHTTYPE DHT22

WiFiClient espClient;
PubSubClient client;

DHT dht(DHTPIN, DHTTYPE);

String deviceId;

void setup() {
  Serial.begin(9600);

  // Wf_Fi Manager stuff
  WiFi.mode(WIFI_STA);
  WiFiManager wm;

  if(!wm.autoConnect("NodeMCU Setup")) {
    Serial.println("Unable to connect to Wi-Fi");
  } else {
    Serial.print("Wi-Fi connection successful. IP: ");
    Serial.println(WiFi.localIP());
  }

  setUpWebServer();

  LittleFS.begin();

  readConfig();

  client.setClient(espClient);

  pinMode(DHTPIN, INPUT);
  dht.begin();

  if (!configIsValid) {
    setStatus("Configuration invalid");
  } else {
    setStatus("READY");
  }

  deviceId = WiFi.macAddress();
  deviceId.replace(":", "");
  Serial.print("ID: ");
  Serial.println(deviceId);
  
}


long lastMsgTime = 0;

void loop() {
  client.loop();
  webserver.handleClient();

  if (!configIsValid) {
    return;
  }

  long now = millis();

  if (now - lastMsgTime > update_frequency) {
    lastMsgTime = now;
    if (configHasChanged || !client.connected()) {
      if (connectAndSendConfig()) {
        sendSensorValues();
      }
    } else {
      sendSensorValues();
    }
  }
  delay(1000);  // not sure we need this...
}

void callback(char* topic, byte* payload, unsigned int length) {
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';
  if (strcmp("online", message) == 0) {
    Serial.println("Home Assistant has come online, we need to resend config");
    configHasChanged = true; // need to change name of this...
  }
}


boolean connectAndSendConfig() {
  String tTopic = "homeassistant/sensor/" + deviceId + "/temperature/config";
  String hTopic = "homeassistant/sensor/" + deviceId + "/humidity/config";

  String tMessage = getFileContents("temperature.json");
  String hMessage = getFileContents("humidity.json");

  tMessage.replace("$device_nickname$", device_nickname);
  tMessage.replace("$device_id$", deviceId);
  hMessage.replace("$device_nickname$", device_nickname);
  hMessage.replace("$device_id$", deviceId);

  client.setServer(mqtt_server.c_str(), mqtt_port);

  client.setBufferSize(2000);
  
  if (!client.connect(deviceId.c_str(), mqtt_username.c_str(), mqtt_password.c_str())) {
    setStatus("ERROR: Failed to connect to:" + mqtt_server + ", code: " + client.state());
    return false;
  } else if (!(client.publish(tTopic.c_str(), tMessage.c_str()) && client.publish(hTopic.c_str(), hMessage.c_str()))) {
    setStatus("ERROR: Failed to publish config to:" + mqtt_server + ", code: " + client.state());
    return false;
  }
  client.subscribe("homeassistant/status");
  client.setCallback(callback);
  setStatus("OK - connection made and config sent.");
  configHasChanged = false;
  return true;
}

void sendSensorValues() {
  float h = dht.readHumidity();
  float f = dht.readTemperature(true); // true -> isFahrenheit
  
  if (isnan(h) || isnan(f)) {
    setStatus("ERROR: Failed to read from DHT sensor");
    return;
  }

  String sensorResult = "Humidity: " + String(h) + "% Temp:" + String(f) + "F";
  setStatus("OK - " + sensorResult);

  String tTopic = "peeper/" + deviceId + "/temperature/state";
  String hTopic = "peeper/" + deviceId + "/humidity/state";

  if (!(client.publish(tTopic.c_str(), String(f).c_str()) && client.publish(hTopic.c_str(), String(h).c_str()))) {
    setStatus("ERROR: Failed to publish to:" + mqtt_server + ", code: " + client.state());
  } else {
    setStatus("OK - sent: " + sensorResult);
  }
}
