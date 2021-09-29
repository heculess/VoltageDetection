#ifndef BLE_SIMPLE_SERVICE_INCLUDED
#define BLE_SIMPLE_SERVICE_INCLUDED

#include "ble_characteristic.h" 

#ifdef __cplusplus
extern "C" {
#endif


class BLESimpleServer;

class BLESimpleCharacteristicMap {
public:
	void setByUUID(BLESimpleCharacteristic* pCharacteristic, const char* uuid);
	void setByUUID(BLESimpleCharacteristic* pCharacteristic, BLESimpleUUID uuid);
	void setByHandle(uint16_t handle, BLESimpleCharacteristic* pCharacteristic);
	BLESimpleCharacteristic* getByUUID(const char* uuid);	
	BLESimpleCharacteristic* getByUUID(BLESimpleUUID uuid);
	BLESimpleCharacteristic* getByHandle(uint16_t handle);
	BLESimpleCharacteristic* getFirst();
	BLESimpleCharacteristic* getNext();

	void handleGATTServerEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param);

private:
	std::map<BLESimpleCharacteristic*, std::string> m_uuidMap;
	std::map<uint16_t, BLESimpleCharacteristic*> m_handleMap;
	std::map<BLESimpleCharacteristic*, std::string>::iterator m_iterator;
};


class BLESimpleService {
public:
	void               addCharacteristic(BLESimpleCharacteristic* pCharacteristic);
	BLESimpleCharacteristic* createCharacteristic(const char* uuid, uint32_t properties);
	BLESimpleCharacteristic* createCharacteristic(BLESimpleUUID uuid, uint32_t properties);

	void               executeCreate(BLESimpleServer* pServer);
	void			   executeDelete();
	BLESimpleCharacteristic* getCharacteristic(const char* uuid);
	BLESimpleCharacteristic* getCharacteristic(BLESimpleUUID uuid);
	BLESimpleUUID      getUUID();
	BLESimpleServer*   getServer();
	void               start();
	void			   stop();
	uint16_t           getHandle();
	uint8_t			   m_instId = 0;

private:
	BLESimpleService(const char* uuid, uint16_t numHandles);
	BLESimpleService(BLESimpleUUID uuid, uint16_t numHandles);
	friend class BLESimpleServer;
	friend class BLESimpleServiceMap;
	friend class BLESimpleDescriptor;
	friend class BLESimpleCharacteristic;

	BLESimpleCharacteristicMap m_characteristicMap;
	uint16_t             m_handle;
	BLESimpleCharacteristic*   m_lastCreatedCharacteristic = nullptr;
	BLESimpleServer*     m_pServer = nullptr;
	BLESimpleUUID        m_uuid;

	Semaphore  m_semaphoreCreateEvt = Semaphore("CreateEvt");
	Semaphore  m_semaphoreDeleteEvt = Semaphore("DeleteEvt");
	Semaphore  m_semaphoreStartEvt  = Semaphore("StartEvt");
	Semaphore  m_semaphoreStopEvt   = Semaphore("StopEvt");

	uint16_t             m_numHandles;

	BLESimpleCharacteristic* getLastCreatedCharacteristic();
	void handleGATTServerEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param);
	void               setHandle(uint16_t handle);
	//void               setService(esp_gatt_srvc_id_t srvc_id);
}; // BLESimpleService


#ifdef __cplusplus
}
#endif


#endif // BLE_SIMPLE_SERVICE_INCLUDED
