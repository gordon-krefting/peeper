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
 */
#define DHTPIN D2
#define DHTTYPE DHT22

WiFiClient espClient;
PubSubClient client;

DHT dht(DHTPIN, DHTTYPE);

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
}

long lastMsgTime = 0;
void loop() {
  webserver.handleClient();
  
  long now = millis();
  if (now - lastMsgTime > update_frequency) {
    lastMsgTime = now;

    float h = dht.readHumidity();
    float f = dht.readTemperature(true); // true -> isFahrenheit
    
    if (isnan(h) || isnan(f)) {
      setStatus("ERROR: Failed to read from DHT sensor");
      return;
    }

    String sensorResult = "Humidity: " + String(h) + "% Temp:" + String(f) + "F";
    setStatus("OK - " + sensorResult);

    String tTopic = "peeper/" + device_nickname + "/temperature";
    String hTopic = "peeper/" + device_nickname + "/humidity";

    client.setServer(mqtt_server.c_str(), mqtt_port);

    if (!client.connect("unimportant_id", mqtt_username.c_str(), mqtt_password.c_str())) {
      setStatus("ERROR: Failed to connect to:" + mqtt_server + ", code: " + client.state());
    //} else if (!client.publish("home-assistant/gordon/mood", "criminyX")) {
    } else if (!(client.publish(tTopic.c_str(), String(f).c_str()) && client.publish(hTopic.c_str(), String(h).c_str()))) {
      setStatus("ERROR: Failed to publish to:" + mqtt_server + ", code: " + client.state());
      client.disconnect();
    } else {
      setStatus("OK - sent: " + sensorResult);
      client.disconnect();  
    }
  }
}
