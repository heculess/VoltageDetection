#ifndef DEVICE_CORE_INCLUDED
#define DEVICE_CORE_INCLUDED

#include <string>
#include <map>
#include <functional>
#include <string.h>
#include <esp_log.h>
#include "json_util.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#define NAME_PREF_CONFIG "pref_config"

#define NAME_KEY_MQTT_BROKER "mqtt_broker"
#define NAME_KEY_MQTT_PORT "mqtt_port"
#define NAME_KEY_MQTT_USERNAME "mqtt_username"
#define NAME_KEY_MQTT_PASSWORD "mqtt_password"
#define NAME_KEY_MQTT_TOPIC "mqtt_topic"

#ifdef __cplusplus
extern "C"
{
#endif

    class DeviceCore
    {
    public:
        static std::string get_version();
        static std::string device_id();
        static std::string get_mqtt_broker();
        static int get_mqtt_port();
        static std::string get_mqtt_username();
        static std::string get_mqtt_password();
        static std::string get_mqtt_topic();

        static void set_mqtt_broker(const char * broker);
        static void set_mqtt_port(int port);
        static void set_mqtt_username(const char * username);
        static void set_mqtt_password(const char * password);
        static void set_mqtt_topic(const char * topic);

        static void GotoDeepSleepAndExit(int time);
    };

    class TaskMutexLocker
    {
    public:
        TaskMutexLocker(SemaphoreHandle_t handle);
        virtual ~TaskMutexLocker();

    private:
        const SemaphoreHandle_t _mutex_handle;
    };

    namespace WatchDog
    {
        void int_wdt();
        void buy_dog();
        void kill_dog();
        void feed_dog();
    };


#ifdef __cplusplus
}
#endif

#endif // DEVICE_CORE_INCLUDED