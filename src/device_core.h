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

#ifdef __cplusplus
extern "C"
{
#endif

    class DeviceCore
    {
    public:
        static std::string get_version();
        static std::string device_id();
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


#ifdef __cplusplus
}
#endif

#endif // DEVICE_CORE_INCLUDED