#ifndef BLE_SEMAPHORE_INCLUDED
#define BLE_SEMAPHORE_INCLUDED

#include <string>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#ifdef __cplusplus
extern "C" {
#endif


class Semaphore 
{
public:
    Semaphore(std::string owner = "<Unknown>");
    ~Semaphore();
    void        give();
    void        give(uint32_t value);
    bool        take(std::string owner = "<Unknown>");
    bool        take(uint32_t timeoutMs, std::string owner = "<Unknown>");
    uint32_t	wait(std::string owner = "<Unknown>");
    bool		timedWait(std::string owner = "<Unknown>", uint32_t timeoutMs = portMAX_DELAY);
	uint32_t	value(){ return m_value; };

private:
    SemaphoreHandle_t m_semaphore;
    pthread_mutex_t   m_pthread_mutex;
    std::string       m_name;
    std::string       m_owner;
    uint32_t          m_value;
    bool              m_usePthreads;

};


#ifdef __cplusplus
}
#endif


#endif // BLE_SEMAPHORE_INCLUDED