#include "ble_server.h"
#include "WiFi.h"
#include "hal_misc.h"
#include <nvs_flash.h>
#include "network.h"
#include "device_core.h"
#include "report_mqtt.h"
#include "config_web_server.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <string>
#include <string.h>

static const char *TAG = "Main";

extern "C"
{
    void app_main(void);
}

BLEComServer *_wifi_info_fetcher = NULL;
ConfigWebServer *_config_server = NULL;

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
    _config_server = new ConfigWebServer();
}

void loop()
{
    set_wifi_by_ble();
    initAfterNetWork();

    WatchDog::feed_dog();

    if (WiFi.status() == WL_CONNECTED)
    {  
        JsonSerializer json;
        json.add_value(DeviceCore::get_device_name().c_str(), "power_on");
        ReportMQTT::publish(json.serialize().c_str());
    }
}


void main_loop(void *pvParameters)
{
    WatchDog::buy_dog();

    ReportMQTT::InitMQTT(1, nullptr);

    int delay_time = DeviceCore::get_mainloop_delay();

    while (true)
    {
        loop();
        delay(delay_time);
    }

    vTaskDelete(NULL);
}

void app_main() 
{
    ESP_ERROR_CHECK(nvs_flash_init());
    WiFi.init();

    if (WiFi.has_wifi_config()){

        WiFi.begin();
        for (int i = 0; i < 3; i++){
            if (Network::auto_connect_wifi(5))
                break;
        }
    }

    Network::esp_initialize_sntp();
    WatchDog::int_wdt();

    if (WiFi.status() != WL_CONNECTED)
    {
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