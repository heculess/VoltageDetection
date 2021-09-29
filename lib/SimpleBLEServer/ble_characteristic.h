#ifndef BLE_SIMPLE_CHARACTERISTIC_INCLUDED
#define BLE_SIMPLE_CHARACTERISTIC_INCLUDED

#include <esp_gatts_api.h>
#include <esp_gap_ble_api.h>
#include <string>
#include <map>
#include "semaphore.h" 
#include "ble_uuid.h" 
#include "ble_value.h" 


#ifdef __cplusplus
extern "C" {
#endif

class BLESimpleService;
class BLESimpleDescriptor;
class BLESimpleCharacteristicCallbacks;

class BLESimpleDescriptorMap {
public:
	void setByUUID(const char* uuid, BLESimpleDescriptor* pDescriptor);
	void setByUUID(BLESimpleUUID uuid, BLESimpleDescriptor* pDescriptor);
	void setByHandle(uint16_t handle, BLESimpleDescriptor* pDescriptor);
	BLESimpleDescriptor* getByUUID(const char* uuid);
	BLESimpleDescriptor* getByUUID(BLESimpleUUID uuid);
	BLESimpleDescriptor* getByHandle(uint16_t handle);

	void handleGATTServerEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param);
	BLESimpleDescriptor* getFirst();
	BLESimpleDescriptor* getNext();
private:
	std::map<BLESimpleDescriptor*, std::string> m_uuidMap;
	std::map<uint16_t, BLESimpleDescriptor*> m_handleMap;
	std::map<BLESimpleDescriptor*, std::string>::iterator m_iterator;
};


class BLESimpleCharacteristic {
public:
	BLESimpleCharacteristic(const char* uuid, uint32_t properties = 0);
	BLESimpleCharacteristic(BLESimpleUUID uuid, uint32_t properties = 0);
	virtual ~BLESimpleCharacteristic();

	void           addDescriptor(BLESimpleDescriptor* pDescriptor);
	BLESimpleDescriptor* getDescriptorByUUID(const char* descriptorUUID);
	BLESimpleDescriptor* getDescriptorByUUID(BLESimpleUUID descriptorUUID);
	BLESimpleUUID        getUUID();
	std::string    getValue();

	void indicate();
	void notify(bool is_notification = true);
	void setBroadcastProperty(bool value);
	void setCallbacks(BLESimpleCharacteristicCallbacks* pCallbacks);
	void setIndicateProperty(bool value);
	void setNotifyProperty(bool value);
	void setReadProperty(bool value);
	void setValue(uint8_t* data, size_t size);
	void setValue(std::string value);
	void setValue(uint16_t& data16);
	void setValue(uint32_t& data32);
	void setValue(int& data32);
	void setValue(float& data32);
	void setValue(double& data64); 
	void setWriteProperty(bool value);
	void setWriteNoResponseProperty(bool value);
	uint16_t getHandle();

	static const uint32_t PROPERTY_READ      = 1<<0;
	static const uint32_t PROPERTY_WRITE     = 1<<1;
	static const uint32_t PROPERTY_NOTIFY    = 1<<2;
	static const uint32_t PROPERTY_BROADCAST = 1<<3;
	static const uint32_t PROPERTY_INDICATE  = 1<<4;
	static const uint32_t PROPERTY_WRITE_NR  = 1<<5;
	static const uint32_t indicationTimeout = 1000;

private:

	friend class BLESimpleServer;
	friend class BLESimpleService;
	friend class BLESimpleDescriptor;
	friend class BLESimpleCharacteristicMap;

	BLESimpleUUID                     m_bleUUID;
	BLESimpleDescriptorMap      m_descriptorMap;
	uint16_t                    m_handle;
	esp_gatt_char_prop_t        m_properties;
	BLESimpleCharacteristicCallbacks* m_pCallbacks;
	BLESimpleService*                 m_pService;
	BLESimpleValue                    m_value;
	esp_gatt_perm_t             m_permissions = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE;
	bool						m_writeEvt = false; // If we have started a long write, this tells the commit code that we were the target

	void handleGATTServerEvent(
			esp_gatts_cb_event_t      event,
			esp_gatt_if_t             gatts_if,
			esp_ble_gatts_cb_param_t* param);

	void                 executeCreate(BLESimpleService* pService);
	esp_gatt_char_prop_t getProperties();
	BLESimpleService*          getService();
	void                 setHandle(uint16_t handle);
	Semaphore m_semaphoreCreateEvt = Semaphore("CreateEvt");
	Semaphore m_semaphoreConfEvt   = Semaphore("ConfEvt");
	Semaphore m_semaphoreSetValue  = Semaphore("SetValue");  

    //void hexDump(const uint8_t* pData, uint32_t length);
}; 

class BLESimpleCharacteristicCallbacks {
public:
	typedef enum {
		SUCCESS_INDICATE,
		SUCCESS_NOTIFY,
		ERROR_INDICATE_DISABLED,
		ERROR_NOTIFY_DISABLED,
		ERROR_GATT,
		ERROR_NO_CLIENT,
		ERROR_INDICATE_TIMEOUT,
		ERROR_INDICATE_FAILURE
	}Status;

	virtual ~BLESimpleCharacteristicCallbacks() {};
	virtual void onRead(BLESimpleCharacteristic* pCharacteristic) {};
	virtual void onWrite(BLESimpleCharacteristic* pCharacteristic) {};
	virtual void onNotify(BLESimpleCharacteristic* pCharacteristic) {};
	virtual void onStatus(BLESimpleCharacteristic* pCharacteristic, Status s, uint32_t code) {};
};


#ifdef __cplusplus
}
#endif


#endif // BLE_SIMPLE_CHARACTERISTIC_INCLUDED