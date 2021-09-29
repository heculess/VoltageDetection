#ifndef BLE_SIMPLE_DESCRIPTOR_INCLUDED
#define BLE_SIMPLE_DESCRIPTOR_INCLUDED

#include <esp_gatts_api.h>
#include <string>
#include "semaphore.h" 
#include "ble_uuid.h" 

#define NULL_HANDLE (0xffff)

#ifdef __cplusplus
extern "C" {
#endif

class BLESimpleService;
class BLESimpleCharacteristic;


class BLESimpleDescriptor {
public:
	BLESimpleDescriptor(const char* uuid, uint16_t max_len = 100);
	BLESimpleDescriptor(BLESimpleUUID uuid, uint16_t max_len = 100);
	virtual ~BLESimpleDescriptor();

	uint16_t getHandle();                                   // Get the handle of the descriptor.
	size_t   getLength();                                   // Get the length of the value of the descriptor.
	BLESimpleUUID  getUUID();                                     // Get the UUID of the descriptor.
	uint8_t* getValue();                                    // Get a pointer to the value of the descriptor.
	void handleGATTServerEvent(
			esp_gatts_cb_event_t      event,
			esp_gatt_if_t             gatts_if,
			esp_ble_gatts_cb_param_t* param);

	void setValue(uint8_t* data, size_t size);              // Set the value of the descriptor as a pointer to data.
	void setValue(std::string value);                       // Set the value of the descriptor as a data buffer.

	std::string toString();                                 // Convert the descriptor to a string representation.

private:
	friend class BLESimpleDescriptorMap;
	friend class BLESimpleCharacteristic;
	BLESimpleUUID                 m_bleUUID;
	uint16_t                m_handle;

	BLESimpleCharacteristic*      m_pCharacteristic;
	esp_gatt_perm_t			m_permissions = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE;
	Semaphore     m_semaphoreCreateEvt = Semaphore("CreateEvt");
	esp_attr_value_t        m_value;

	void executeCreate(BLESimpleCharacteristic* pCharacteristic);
	void setHandle(uint16_t handle);
}; 

class BLES2902: public BLESimpleDescriptor {
public:
	BLES2902();
	bool getNotifications();
	bool getIndications();
	void setNotifications(bool flag);
	void setIndications(bool flag);

}; // BLE2902

#ifdef __cplusplus
}
#endif


#endif // BLE_SIMPLE_DESCRIPTOR_INCLUDED