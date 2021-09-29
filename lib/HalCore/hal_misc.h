#ifndef HAL_MISC_INCLUDED
#define HAL_MISC_INCLUDED

#include "esp_attr.h"
#include <freertos/FreeRTOS.h> 
#include "freertos/task.h"
#include "time.h"

unsigned long IRAM_ATTR millis();
unsigned long IRAM_ATTR micros();

void IRAM_ATTR delayMicroseconds(uint32_t us);

void delay(uint32_t ms);

bool getLocalTime(struct tm * info, uint32_t ms = 5000U);

class CoreWDTLocker
{
public:
    CoreWDTLocker(int core_index);
    virtual ~CoreWDTLocker();

private:
    TaskHandle_t _idle;
};

#endif // HAL_MISC_INCLUDED