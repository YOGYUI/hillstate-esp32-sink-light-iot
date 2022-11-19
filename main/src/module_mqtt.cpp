#include "module_mqtt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "defines.h"
#include <cstring>
#include "cJSON.h"
#include "module_gpio.h"

static const char *TAG = "MQTT";
static esp_mqtt_client_handle_t mqtt_client = nullptr;
static xQueueHandle queue_command = nullptr;

static void subscribe_topics() {
    int msg_id;
    msg_id = esp_mqtt_client_subscribe(mqtt_client, MQTT_SUBSCRIBE_TOPIC_DEVICE, 0);
    ESP_LOGI(TAG, "sent subscribe, msg_id=%d", msg_id);
}

bool mqtt_publish_current_state(int state) {
    if (!mqtt_client)
        return false;

    cJSON* obj = cJSON_CreateObject();
    if (obj == nullptr)
        return false;
    
    cJSON* item_state = cJSON_CreateNumber(state);
    if (item_state) {
        cJSON_AddItemToObject(obj, "state", item_state);
    }

    char* payload = cJSON_Print(obj);
    if (payload) {
        esp_mqtt_client_publish(mqtt_client, MQTT_PUBLISH_TOPIC_DEVICE, payload, 0, 1, 0);
        // ESP_LOGI(TAG, "try to publish (topic: %s, payload: %s)", MQTT_PUBLISH_TOPIC_DEVICE, payload);
    } else {
        return false;
    }

    return true;
}

static void command_handle_task(void *params) {
    int command_value;
    int state_value;
    uint32_t count;
    while (true) {
        if (xQueueReceive(queue_command, &command_value, portMAX_DELAY)) {
            count = 0;
            for (;;) {
                state_value = get_state_level();
                if (command_value == state_value)
                    break;
                count++;
                ESP_LOGI(TAG, "Command = %d, State = %d >> try to toggle control signal (try cnt: %u)", command_value, state_value, count);
                toggle_ctrl_signal();
                vTaskDelay(500 / portTICK_PERIOD_MS);
            }
        }
    }
}

static void parse_recv_message(const char* payload, size_t payload_len) {
    cJSON* obj = cJSON_ParseWithLength(payload, payload_len);
    if (obj == nullptr) {
        const char* err = cJSON_GetErrorPtr();
        if (err) {
            ESP_LOGE(TAG, "Payload JSON parse error: %s", err);
        }
    } else {
        ESP_LOGI(TAG, "payload parse result: %s", cJSON_Print(obj));

        const cJSON* state = cJSON_GetObjectItemCaseSensitive(obj, "state");
        if (cJSON_IsNumber(state)) {
            int value = state->valueint;
            // toggle_ctrl_signal();
            xQueueSend(queue_command, (void*)&value, (TickType_t)0);
        }
    }
}

static void mqtt_event_handler(void* arg, esp_event_base_t base, int32_t evt_id, void* evt_data) {
    esp_mqtt_event_t* event = reinterpret_cast<esp_mqtt_event_t*>(evt_data);
    // esp_mqtt_client_handle_t client = event->client;
    esp_mqtt_event_id_t event_id = (esp_mqtt_event_id_t)evt_id;
    int state;
    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Connected");
            subscribe_topics();
            state = get_state_level();
            mqtt_publish_current_state(state);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGE(TAG, "Disconnected");
            esp_restart();      // MQTT가 끊어지면 리부팅 (TODO: 와이파이문제인지, 브로커 문제인지 파악 후 reconnect)
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "subscribe event, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "publish event, msg_id=%d", event->msg_id);
            // free_to_publish = true;
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "message arrived [%.*s] %.*s", event->topic_len, event->topic, event->data_len, event->data);
            if (!strncmp(event->topic, MQTT_SUBSCRIBE_TOPIC_DEVICE, strlen(MQTT_SUBSCRIBE_TOPIC_DEVICE))) {
                parse_recv_message(event->data, event->data_len);
            }
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "Error");
            break;
        default:
            ESP_LOGI(TAG, "Unhandled Event ID - %d", event_id);
            break;
    }
}

bool initialize_mqtt() {
    queue_command = xQueueCreate(10, sizeof(int));
    xTaskCreate(command_handle_task, "COMMAND_HANDLE_TASK", 2048, nullptr, 10, nullptr);

    esp_mqtt_client_config_t config = {};
    config.uri = MQTT_BROKER_URI;
    config.port = MQTT_BROKER_PORT;
    config.username = MQTT_BROKER_USERNAME;
    config.password = MQTT_BROKER_PASSWORD;

    mqtt_client = esp_mqtt_client_init(&config);
    if (esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, nullptr) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register mqtt client event");
        return false;
    } 

    if (esp_mqtt_client_start(mqtt_client) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start mqtt client");
        return false;
    }

    ESP_LOGI(TAG, "Configured MQTT");

    return true;
}