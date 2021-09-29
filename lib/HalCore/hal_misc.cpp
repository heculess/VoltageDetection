#include "hal_misc.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include <esp_log.h>

#define NOP() asm volatile ("nop")


static const char *TAG = "CoreWDTLocker";

unsigned long IRAM_ATTR millis()
{
    return (unsigned long) (esp_timer_get_time() / 1000ULL);
}

unsigned long IRAM_ATTR micros()
{
    return (unsigned long) (esp_timer_get_time());
}

void IRAM_ATTR delayMicroseconds(uint32_t us)
{
    uint32_t m = micros();
    if(us){
        uint32_t e = (m + us);
        if(m > e){ //overflow
            while(micros() > e){
                NOP();
            }
        }
        while(micros() < e){
            NOP();
        }
    }
}

void delay(uint32_t ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

bool getLocalTime(struct tm * info, uint32_t ms)
{
    uint32_t start = millis();
    time_t now;
    while((millis()-start) <= ms) {
        time(&now);
        localtime_r(&now, info);
        if(info->tm_year > (2016 - 1900)){
            return true;
        }
        delay(10);
    }
    return false;
}

CoreWDTLocker::CoreWDTLocker(int core_index):
_idle(xTaskGetIdleTaskHandleForCPU(core_index))
{
    if(_idle == NULL || esp_task_wdt_delete(_idle) != ESP_OK){
        ESP_LOGE(TAG, "Failed to delete Core %d IDLE task to WDT", core_index);
    }
}

CoreWDTLocker::~CoreWDTLocker()
{
    if(_idle)
        esp_task_wdt_add(_idle);
}