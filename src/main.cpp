#include "ble_server.h"
#include "WiFi.h"
#include "hal_misc.h"
#include <nvs_flash.h>
#include "network.h"
#include "device_core.h"
#include <esp_log.h>
#include <esp_task_wdt.h>
#include <esp_sleep.h>
#include <freertos/FreeRTOS.h>
#include <string>
#include <string.h>

static const char *TAG = "Main";

extern "C"
{
    void app_main(void);
}

BLEComServer *_wifi_info_fetcher = NULL;

void set_wifi_by_ble()
{
    if (!_wifi_info_fetcher)
        return;

    std::string ssid;
    std::string psk;
    if (!_wifi_info_fetcher->get_connect_info(ssid, psk))
    {
        if (_wifi_info_fetcher->get_structure_millis() > 5 * 60 * 1000)
        {
            ESP_LOGW(TAG, "Setting time out, Close Bluetooth config\r\n");
            delete _wifi_info_fetcher;
            _wifi_info_fetcher = NULL;
        }
        return;
    }

    ESP_LOGI(TAG, "ssid : %s, psk : %s |$\r\n", ssid.c_str(), psk.c_str());

    delete _wifi_info_fetcher;
    _wifi_info_fetcher = NULL;

    WiFi.begin(ssid.c_str(), psk.c_str());
    int wifiOK = 10;
    while (wifiOK-- > 0)
    {
        if (WiFi.has_wifi_config())
            break;
        printf("T.");
        delay(1000);
    }
}


void initAfterNetWork()
{
    static bool firstRun = true;
    if (!firstRun)
        return;

    if (WiFi.status() != WL_CONNECTED)
        return;

    firstRun = false;

    printf("[ network init complete ] ====\r\n");

}

void loop()
{
    set_wifi_by_ble();
    initAfterNetWork();

    esp_task_wdt_reset();

    if (WiFi.status() == WL_CONNECTED)
    {  
    }
}


void main_loop(void *pvParameters)
{
    esp_err_t err = esp_task_wdt_init(30, true);
    if (err != ESP_OK)
        ESP_LOGE(TAG, "esp_task_wdt_init failed: %d", err);
    err = esp_task_wdt_add(NULL);
    if (err != ESP_OK)
        ESP_LOGE(TAG, "esp_task_wdt_add failed: %d", err);

    while (true)
    {
        loop();
        delay(500);
    }

    vTaskDelete(NULL);
}

void app_main() 
{
    ESP_ERROR_CHECK(nvs_flash_init());
    WiFi.init();

    bool hasWifiConfig = WiFi.has_wifi_config();
    if (hasWifiConfig)
    {
        WiFi.begin();
        for (int i = 0; i < 20; i++) 
        {
            if (Network::auto_connect_wifi(10))
                break;
        }
    }

    Network::esp_initialize_sntp();

    if (WiFi.status() != WL_CONNECTED)
    {
        if (hasWifiConfig)//有 wifi 信息连接不上，则重启重试
        {
            DeviceCore::GotoDeepSleepAndExit(10);
            return;
        }
        _wifi_info_fetcher = new BLEComServer();
        if (!_wifi_info_fetcher)
        {
            DeviceCore::GotoDeepSleepAndExit(10);
            return;
        }
    }

    xTaskCreate(main_loop, "main_loop", 4096, NULL,
                configMAX_PRIORITIES - 1, NULL);
}