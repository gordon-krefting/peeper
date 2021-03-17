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
  
  setStatus("READY");

  deviceId = WiFi.macAddress();
  deviceId.replace(":", "");
  Serial.print("ID: ");
  Serial.println(deviceId);
  
}

boolean configSent = false;
long lastMsgTime = 0;

void loop() {
  webserver.handleClient();

  long now = millis();
  if (!configSent) {
    configSent = sendConfig();    
  } else if (now - lastMsgTime > update_frequency) {
    lastMsgTime = now;
    sendSensorValues();
  }
}

boolean sendConfig() {
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
  
  if (!client.connect("unimportant_id", mqtt_username.c_str(), mqtt_password.c_str())) {
    setStatus("ERROR: Failed to connect to:" + mqtt_server + ", code: " + client.state());
    return false;
  } else if (!(client.publish(tTopic.c_str(), tMessage.c_str()) && client.publish(hTopic.c_str(), hMessage.c_str()))) {
    setStatus("ERROR: Failed to publish config to:" + mqtt_server + ", code: " + client.state());
    client.disconnect();
    return false;
  } else {
    setStatus("OK - config sent.");
    client.disconnect();  
  }
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

  client.setServer(mqtt_server.c_str(), mqtt_port);

  if (!client.connect("unimportant_id", mqtt_username.c_str(), mqtt_password.c_str())) {
    setStatus("ERROR: Failed to connect to:" + mqtt_server + ", code: " + client.state());
  } else if (!(client.publish(tTopic.c_str(), String(f).c_str()) && client.publish(hTopic.c_str(), String(h).c_str()))) {
    setStatus("ERROR: Failed to publish to:" + mqtt_server + ", code: " + client.state());
    client.disconnect();
  } else {
    setStatus("OK - sent: " + sensorResult);
    client.disconnect();  
  }
  
}
