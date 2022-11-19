#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "sdkconfig.h"
#include "defines.h"
#include "module_gpio.h"
#include "module_mqtt.h"
#include "module_timer.h"
#include "module_wifi_provisioning.h"
#include "esp_log.h"
#include "nvs_flash.h"

static void timer_callback(void *arg) {  
    int state = get_state_level();
    mqtt_publish_current_state(state);
}

extern "C" void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    initialize_gpio();
    initialize_timer(60 * 1000 * 1000, timer_callback);  // 1분에 한번씩 정기적으로 publish
    initialize_wifi_provisioning();

    for (;;) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}