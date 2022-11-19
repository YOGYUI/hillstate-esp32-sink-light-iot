#include "module_gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "defines.h"
#include "module_mqtt.h"

static const char *TAG = "GPIO";
static xQueueHandle queue_gpio = nullptr;

static void IRAM_ATTR gpio_interrupt_handler(void *arg) {
    uint32_t io_num = (uint32_t)arg;
    xQueueSendFromISR(queue_gpio, &io_num, nullptr);
}

static void state_isr_task(void *params) {
    uint32_t io_num;
    while (true) {
        if (xQueueReceive(queue_gpio, &io_num, portMAX_DELAY)) {
            int state = get_state_level();
            ESP_LOGI(TAG, "State GPIO(%u) Interrupt - Level = %d", io_num, state);
            mqtt_publish_current_state(state);
        }
    }
}

static bool set_ctrl_pin_as_input() {
    if (gpio_set_direction((gpio_num_t)GPIO_PIN_CTRL, GPIO_MODE_INPUT) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set GPIO(%d) mode as input", GPIO_PIN_CTRL);
        return false;
    }
    return true;
}

static bool set_ctrl_pin_as_output() {
    if (gpio_set_direction((gpio_num_t)GPIO_PIN_CTRL, GPIO_MODE_OUTPUT) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set GPIO(%d) mode as output", GPIO_PIN_CTRL);
        return false;
    }
    return true;
}

bool initialize_gpio() {
    queue_gpio = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(state_isr_task, "STATE_ISR_TASK", 2048, nullptr, 1, nullptr);

    gpio_config_t config = {};

    // On/Off Control GPIO (IN/OUT)
    config.pin_bit_mask = 1ULL << GPIO_PIN_CTRL;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;
    if (gpio_config(&config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO");
        return false;
    }
    set_ctrl_pin_as_input();

    // State GPIO (IN)
    config.pin_bit_mask = 1ULL << GPIO_PIN_STATE;
    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_ANYEDGE;
    if (gpio_config(&config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO");
        return false;
    }

    if (gpio_install_isr_service(0) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install interrupt service routine");
        return false;
    }

    if (gpio_isr_handler_add((gpio_num_t)GPIO_PIN_STATE, gpio_interrupt_handler, (void *)GPIO_PIN_STATE) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add isr handler");
        return false;
    }

    ESP_LOGI(TAG, "Configured GPIO");

    return true;
}

bool set_ctrl_level(uint32_t level) {
    if (gpio_set_level((gpio_num_t)GPIO_PIN_CTRL, level) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set GPIO(%d) level as %u", GPIO_PIN_CTRL, level);
        return false;
    }
    return true;
}

bool toggle_ctrl_signal() {
    set_ctrl_pin_as_output();

    if (!set_ctrl_level(0))
        return false;
    
    vTaskDelay(CTRL_SIG_TOGGLE_INTERVAL_MS / portTICK_PERIOD_MS);
    
    if (!set_ctrl_level(1))
        return false;

    set_ctrl_pin_as_input();

    return true;
}

int get_state_level() {
    return gpio_get_level((gpio_num_t)GPIO_PIN_STATE);
}