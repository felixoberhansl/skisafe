from __future__ import print_function
import threading
import paho.mqtt.client as mqtt         # pip install paho-mqtt
import ssl
import json
import math
import struct
from autobahn.asyncio.component import Component
from autobahn.asyncio.component import run
from asyncio import sleep


# WAMP

wamp_session = None
alarm_timer = None


wamp_component = Component(
    transports=u"ws://localhost:8080/ws",
    realm=u"realm1",
)

@wamp_component.on_join
async def joined(session, details):
    global wamp_session
    print("WAMP connected")
    wamp_session = session


# MQTT
server = "mioty-summercamp.iis.fraunhofer.de"
username = "summercamp"
password = "HtS1z7EL"
path = 'mioty/+/+/uplink'

def timer_alarm_handler():
    global wamp_session, alarm_timer
    wamp_session.publish(u'fall_alert', True)
    print("ALARM timer")


def timer_timeout_handler():
    global wamp_session, alarm_timer
    wamp_session.publish(u'timeout', True)

    alarm_timer = threading.Timer(20, timer_alarm_handler)
    alarm_timer.start()
    print("TIMEOUT timer")

def reset_timers():
    global timeout_timer, alarm_timer

    timeout_timer.cancel()

    timeout_timer = threading.Timer(20, timer_timeout_handler)
    timeout_timer.start()
    if alarm_timer != None:
        alarm_timer.cancel()
        alarm_timer = threading.Timer(20, timer_alarm_handler)
        
        alarm_timer.start()

    print("RESET timers")


def mqtt_runner(name):
    global timeout_timer

    client = mqtt.Client()
    client.username_pw_set(username, password)
    client.tls_set(cert_reqs=ssl.CERT_NONE, tls_version=ssl.PROTOCOL_TLSv1_2)

    client.on_message = on_message
    client.on_connect = on_connect
    client.connect(server, 8883, 60)

    # TimeOut Timer
    timeout_timer = threading.Timer(20, timer_timeout_handler)
    timeout_timer.start()


    try:
        client.loop_forever()

    except KeyboardInterrupt:
        client.loop_stop()
        client.disconnect()


def calc_distance(rssi, id):

    if id == "campus":
        send_const = 0.564911*10**12
    elif id == "gerhaus":
        send_const = 1.362278*10**15

    return math.sqrt(send_const/(10**(rssi/10)))

def get_id(base_id):
    if base_id == int(0x70b3d56770020112):
        return "campus"
    elif base_id == int(0x70b3d56770020132):
        return "gerhaus"
    else:
        return "unknown"



def on_message(client, userdata, message):
    global wamp_session

    fields = json.loads(message.payload.decode("utf-8"))

    # Keep alive message used for location, temperature and time publishing
    if(fields['data'][0] == 0x7c):

        base_id = get_id(fields['bsEui'])
        dist = calc_distance(abs(fields['rssi']),base_id)
        location_info = []
        location_info.append(dist)
        location_info.append(base_id)
        location_info.append(fields['time'])
        print(location_info)

        temperature = fields['data'][1] * 256 + fields['data'][2]

        print(temperature)

        if wamp_session != None:
            wamp_session.publish(u'location_info', location_info)
            wamp_session.publish(u'temperature', temperature)
        else:
            print("WAMP session not existent")

        reset_timers()

    # Fall intensity
    elif(fields['data'][0] == 0x7b):

        max_accel = struct.unpack("<f", bytearray(fields['data'][1:5]))
        print(max_accel)
        if wamp_session != None:
            wamp_session.publish(u'fall_alert', True)
            wamp_session.publish(u"fall_intensity", max_accel)
        else:
            print("WAMP session not existent")



def on_connect(client,userdata,flag,rc):
    print("MQTT connected")
    client.subscribe(path)


if __name__ == "__main__":  

    mqtt_thread = threading.Thread(target=mqtt_runner, args=(1,))
    mqtt_thread.start()

    run([wamp_component])
    
