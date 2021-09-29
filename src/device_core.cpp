#include "device_core.h"
#include "hal_misc.h"
#include "nvs_flash.h"
#include <map>
#include "esp_task_wdt.h"
#include <esp_system.h>
#include <esp_sleep.h>
#include "Preferences.h"
#include "esp_log.h"

static const char *TAG = "DeviceCore";

#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */


std::string DeviceCore::get_version()
{
    ESP_LOGE(TAG, "DeviceCore version is not init");
    return "";
}

std::string DeviceCore::device_id()
{
    static std::string device_id;
    if (device_id.empty())
    {
        uint8_t mac[6];
        char macStr[13] = {0};
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        sprintf(macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        device_id = macStr;
    }

    return device_id;
}

void DeviceCore::GotoDeepSleepAndExit(int time)
{
    if (time < 3)
        time = 3;

    printf("All done! Going to sleep now, wake up in %ds ...\r\n", time);

    esp_sleep_enable_timer_wakeup(time * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
}

///////////////////////////////////////////////////////////////////////////////////
TaskMutexLocker::TaskMutexLocker(SemaphoreHandle_t handle) : _mutex_handle(handle)
{
    ESP_LOGD(TAG, "xSemaphoreTake");
    if (_mutex_handle)
        xSemaphoreTake(_mutex_handle, portMAX_DELAY);
}

TaskMutexLocker::~TaskMutexLocker()
{
    ESP_LOGD(TAG, "xSemaphoreGive");
    if (_mutex_handle)
        xSemaphoreGive(_mutex_handle);
}