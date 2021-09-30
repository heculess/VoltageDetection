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

std::string DeviceCore::get_mqtt_broker()
{
    Preferences pref;
    pref.begin(NAME_PREF_CONFIG, true);
    return pref.getString(NAME_KEY_MQTT_BROKER, "");
}

int DeviceCore::get_mqtt_port()
{
    Preferences pref;
    pref.begin(NAME_PREF_CONFIG, true);
    return pref.getInt(NAME_KEY_MQTT_PORT, 1883);
}

std::string DeviceCore::get_mqtt_username()
{
    Preferences pref;
    pref.begin(NAME_PREF_CONFIG, true);
    return pref.getString(NAME_KEY_MQTT_USERNAME, "");
}

std::string DeviceCore::get_mqtt_password()
{
    Preferences pref;
    pref.begin(NAME_PREF_CONFIG, true);
    return pref.getString(NAME_KEY_MQTT_PASSWORD, "");
}

std::string DeviceCore::get_mqtt_topic()
{
    Preferences pref;
    pref.begin(NAME_PREF_CONFIG, true);
    return pref.getString(NAME_KEY_MQTT_TOPIC, "");
}

void DeviceCore::set_mqtt_broker(const char * broker)
{
    Preferences pref;
    pref.begin(NAME_PREF_CONFIG);
    pref.putString(NAME_KEY_MQTT_BROKER, broker);
}   

void DeviceCore::set_mqtt_port(int port)
{
    Preferences pref;
    pref.begin(NAME_PREF_CONFIG);
    pref.putInt(NAME_KEY_MQTT_PORT, port);
}

void DeviceCore::set_mqtt_username(const char * username)
{
    Preferences pref;
    pref.begin(NAME_PREF_CONFIG);
    pref.putString(NAME_KEY_MQTT_USERNAME, username);
}

void DeviceCore::set_mqtt_password(const char * password)
{
    Preferences pref;
    pref.begin(NAME_PREF_CONFIG);
    pref.putString(NAME_KEY_MQTT_PASSWORD, password);
}

void DeviceCore::set_mqtt_topic(const char * topic)
{
    Preferences pref;
    pref.begin(NAME_PREF_CONFIG);
    pref.putString(NAME_KEY_MQTT_TOPIC, topic);
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

void WatchDog::int_wdt()
{
    esp_err_t err = esp_task_wdt_init(30, true);
    if (err != ESP_OK)
        ESP_LOGE(TAG, "esp_task_wdt_init failed: %d", err);
}

void WatchDog::buy_dog()
{
    esp_err_t err = esp_task_wdt_add(NULL);
    if (err != ESP_OK)
        ESP_LOGE(TAG, "esp_task_wdt_add failed: %d", err);
}

void WatchDog::kill_dog()
{
    esp_task_wdt_delete(NULL);
}

void WatchDog::feed_dog()
{
    esp_task_wdt_reset(); 
}