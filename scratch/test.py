#!/usr/bin/env python3
#
import time
import random
import paho.mqtt.client as mqtt
import paho.mqtt.publish as publish

mqtt_auth = {'username': 'mqttuser', 'password': 'mqttpass'}

broker = 'homeassistant.local'
# broker = 'test.mosquitto.org'
state_topic = '/test/gordon/number'
delay = 2

# Send a single message to set the mood
publish.single('home-assistant/gordon/mood', 'a little hungry',
               hostname=broker, auth=mqtt_auth)

# Send messages in a loop
client = mqtt.Client("ha-client")
client.connect(broker)
client.loop_start()

while True:
    client.publish(state_topic, random.randrange(0, 50, 1))
    time.sleep(delay)
