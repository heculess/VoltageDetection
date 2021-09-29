#include "semaphore.h" 
#include <sstream>

uint32_t Semaphore::wait(std::string owner) {

	if (m_usePthreads) {
		pthread_mutex_lock(&m_pthread_mutex);
	} else {
		xSemaphoreTake(m_semaphore, portMAX_DELAY);
	}

	if (m_usePthreads) {
		pthread_mutex_unlock(&m_pthread_mutex);
	} else {
		xSemaphoreGive(m_semaphore);
	}

	return m_value;
} 

bool Semaphore::timedWait(std::string owner, uint32_t timeoutMs) {

	if (m_usePthreads && timeoutMs != portMAX_DELAY) {
		assert(false);  // We apparently don't have a timed wait for pthreads.
	}

	auto ret = pdTRUE;

	if (m_usePthreads) {
		pthread_mutex_lock(&m_pthread_mutex);
	} else {
		ret = xSemaphoreTake(m_semaphore, timeoutMs);
	}

	if (m_usePthreads) {
		pthread_mutex_unlock(&m_pthread_mutex);
	} else {
		xSemaphoreGive(m_semaphore);
	}

	return ret;
} 


Semaphore::Semaphore(std::string name) {
	m_usePthreads = false;   	// Are we using pThreads or FreeRTOS?
	if (m_usePthreads) {
		pthread_mutex_init(&m_pthread_mutex, nullptr);
	} else {
		m_semaphore = xSemaphoreCreateBinary();
		xSemaphoreGive(m_semaphore);
	}

	m_name      = name;
	m_owner     = std::string("<N/A>");
	m_value     = 0;
}


Semaphore::~Semaphore() {
	if (m_usePthreads) {
		pthread_mutex_destroy(&m_pthread_mutex);
	} else {
		vSemaphoreDelete(m_semaphore);
	}
}


void Semaphore::give() {
    m_owner = std::string("<N/A>");

	if (m_usePthreads) {
		pthread_mutex_unlock(&m_pthread_mutex);
	} else {
		xSemaphoreGive(m_semaphore);
	}
} 

void Semaphore::give(uint32_t value) {
	m_value = value;
	give();
} 

bool Semaphore::take(std::string owner) {
	bool rc = false;
	if (m_usePthreads) {
		pthread_mutex_lock(&m_pthread_mutex);
	} else {
		rc = ::xSemaphoreTake(m_semaphore, portMAX_DELAY) == pdTRUE;
	}
	
	if (rc) {
		m_owner = owner;
	} 

	return rc;
} 


bool Semaphore::take(uint32_t timeoutMs, std::string owner) {
	bool rc = false;
	if (m_usePthreads) {
		assert(false);  
	} else {
		rc = ::xSemaphoreTake(m_semaphore, timeoutMs / portTICK_PERIOD_MS) == pdTRUE;
	}
	if (rc) {
		m_owner = owner;
	} 
	return rc;
} 
