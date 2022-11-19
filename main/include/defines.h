#ifndef _DEFINES_H_
#define _DEFINES_H_
#pragma once

/* define GPIO Pin Numbers */
#define GPIO_PIN_STATE  22
#define GPIO_PIN_CTRL   23
#define CTRL_SIG_TOGGLE_INTERVAL_MS 200

/* define MQTT broker info  */
#define MQTT_BROKER_URI         "mqtt broker host address"
#define MQTT_BROKER_PORT        8123
#define MQTT_BROKER_USERNAME    "mqtt broker id"
#define MQTT_BROKER_PASSWORD    "mqtt broker password"

#define MQTT_PUBLISH_TOPIC_DEVICE       "home/hillstate/sinklight/state"
#define MQTT_SUBSCRIBE_TOPIC_DEVICE     "home/hillstate/sinklight/command"

#define CTRL_SIG_TOGGLE_INTERVAL_MS     200
#define BLE_PROV_AP_PREFIX              "YOGYUI_"
#define BLE_PROV_POP                    "12345678"

#endif