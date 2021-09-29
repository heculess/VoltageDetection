#include <string.h>
#include "ble_descriptor.h" 
#include "ble_service.h" 
#include "ble_characteristic.h" 

BLESimpleDescriptor::BLESimpleDescriptor(const char* uuid, uint16_t len) : 
BLESimpleDescriptor(BLESimpleUUID(uuid), len) {
}	

BLESimpleDescriptor::BLESimpleDescriptor(BLESimpleUUID uuid, uint16_t max_len) {
	m_bleUUID            = uuid;
	m_value.attr_len     = 0;                               
	m_value.attr_max_len = max_len;                     
	m_handle             = NULL_HANDLE;                              
	m_pCharacteristic    = nullptr;                                  

	m_value.attr_value   = (uint8_t*) malloc(max_len);  
} 

BLESimpleDescriptor::~BLESimpleDescriptor() {
	free(m_value.attr_value);   
} 


void BLESimpleDescriptor::executeCreate(BLESimpleCharacteristic* pCharacteristic) {

	if (m_handle != NULL_HANDLE) 
		return;

	m_pCharacteristic = pCharacteristic; 
	esp_attr_control_t control;
	control.auto_rsp = ESP_GATT_AUTO_RSP;
	m_semaphoreCreateEvt.take("executeCreate");

	if (::esp_ble_gatts_add_char_descr(pCharacteristic->getService()->getHandle(),
		getUUID().getNative(),(esp_gatt_perm_t)m_permissions,&m_value,&control) != ESP_OK) 
		return;

	m_semaphoreCreateEvt.wait("executeCreate");
} 

uint16_t BLESimpleDescriptor::getHandle() {
	return m_handle;
} 

size_t BLESimpleDescriptor::getLength() {
	return m_value.attr_len;
} 

BLESimpleUUID BLESimpleDescriptor::getUUID() {
	return m_bleUUID;
} 

uint8_t* BLESimpleDescriptor::getValue() {
	return m_value.attr_value;
} 

void BLESimpleDescriptor::handleGATTServerEvent(
		esp_gatts_cb_event_t      event,
		esp_gatt_if_t             gatts_if,
		esp_ble_gatts_cb_param_t* param) {
	switch (event) {
		case ESP_GATTS_ADD_CHAR_DESCR_EVT: {
			if (m_pCharacteristic != nullptr && m_bleUUID.equals(BLESimpleUUID(param->add_char_descr.descr_uuid)) 
                && m_pCharacteristic->getService()->getHandle() == param->add_char_descr.service_handle 
                && m_pCharacteristic == m_pCharacteristic->getService()->getLastCreatedCharacteristic()) {
				setHandle(param->add_char_descr.attr_handle);
				m_semaphoreCreateEvt.give();
			}
			break;
		} // ESP_GATTS_ADD_CHAR_DESCR_EVT
		case ESP_GATTS_WRITE_EVT: {
			if (param->write.handle == m_handle) {
				setValue(param->write.value, param->write.len);   
			}  

			break;
		} // ESP_GATTS_WRITE_EVT
		case ESP_GATTS_READ_EVT: {
			break;
		} 

		default:
			break;
	} 
} 

void BLESimpleDescriptor::setHandle(uint16_t handle) {
	m_handle = handle;
} 

void BLESimpleDescriptor::setValue(uint8_t* data, size_t length) {
	if (length > ESP_GATT_MAX_ATTR_LEN) 
		return;
	m_value.attr_len = length;
	memcpy(m_value.attr_value, data, length);
} 


void BLESimpleDescriptor::setValue(std::string value) {
	setValue((uint8_t*) value.data(), value.length());
} 

std::string BLESimpleDescriptor::toString() {
	char hex[5];
	snprintf(hex, sizeof(hex), "%04x", m_handle);
	std::string res = "UUID: " + m_bleUUID.toString() + ", handle: 0x" + hex;
	return res;
} 

BLES2902::BLES2902() : 
BLESimpleDescriptor(BLESimpleUUID((uint16_t) 0x2902)) {
	uint8_t data[2] = { 0, 0 };
	setValue(data, 2);
} 

bool BLES2902::getNotifications() {
	return (getValue()[0] & (1 << 0)) != 0;
} 


bool BLES2902::getIndications() {
	return (getValue()[0] & (1 << 1)) != 0;
} 

void BLES2902::setIndications(bool flag) {
	uint8_t *pValue = getValue();
	if (flag) pValue[0] |= 1 << 1;
	else pValue[0] &= ~(1 << 1);
} 

void BLES2902::setNotifications(bool flag) {
	uint8_t *pValue = getValue();
	if (flag) pValue[0] |= 1 << 0;
	else pValue[0] &= ~(1 << 0);
}