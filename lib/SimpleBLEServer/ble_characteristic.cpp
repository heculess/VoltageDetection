#include <string.h>
#include "ble_characteristic.h" 
#include "ble_service.h" 
#include "ble_server_obj.h" 
#include "ble_descriptor.h" 

static BLESimpleCharacteristicCallbacks defaultCallback;

BLESimpleCharacteristic::BLESimpleCharacteristic(const char* uuid, uint32_t properties) : 
BLESimpleCharacteristic(BLESimpleUUID(uuid), properties) {
}

BLESimpleCharacteristic::BLESimpleCharacteristic(BLESimpleUUID uuid, uint32_t properties) {
	m_bleUUID    = uuid;
	m_handle     = NULL_HANDLE;
	m_properties = (esp_gatt_char_prop_t)0;
	m_pCallbacks = &defaultCallback;

	setBroadcastProperty((properties & PROPERTY_BROADCAST) != 0);
	setReadProperty((properties & PROPERTY_READ) != 0);
	setWriteProperty((properties & PROPERTY_WRITE) != 0);
	setNotifyProperty((properties & PROPERTY_NOTIFY) != 0);
	setIndicateProperty((properties & PROPERTY_INDICATE) != 0);
	setWriteNoResponseProperty((properties & PROPERTY_WRITE_NR) != 0);
} 


BLESimpleCharacteristic::~BLESimpleCharacteristic() {
} 

void BLESimpleCharacteristic::addDescriptor(BLESimpleDescriptor* pDescriptor) {
	m_descriptorMap.setByUUID(pDescriptor->getUUID(), pDescriptor);
} 


void BLESimpleCharacteristic::executeCreate(BLESimpleService* pService) {

	if (m_handle != NULL_HANDLE) 
		return;

	m_pService = pService; 

	esp_attr_control_t control;
	control.auto_rsp = ESP_GATT_RSP_BY_APP;

	m_semaphoreCreateEvt.take("executeCreate");

	if (::esp_ble_gatts_add_char(m_pService->getHandle(),getUUID().getNative(),
		static_cast<esp_gatt_perm_t>(m_permissions),getProperties(),nullptr,&control) != ESP_OK) 
		return;

	m_semaphoreCreateEvt.wait("executeCreate");

	BLESimpleDescriptor* pDescriptor = m_descriptorMap.getFirst();
	while (pDescriptor != nullptr) {
		pDescriptor->executeCreate(this);
		pDescriptor = m_descriptorMap.getNext();
	} 
} 

BLESimpleDescriptor* BLESimpleCharacteristic::getDescriptorByUUID(const char* descriptorUUID) {
	return m_descriptorMap.getByUUID(BLESimpleUUID(descriptorUUID));
} 

BLESimpleDescriptor* BLESimpleCharacteristic::getDescriptorByUUID(BLESimpleUUID descriptorUUID) {
	return m_descriptorMap.getByUUID(descriptorUUID);
} 

uint16_t BLESimpleCharacteristic::getHandle() {
	return m_handle;
} 

esp_gatt_char_prop_t BLESimpleCharacteristic::getProperties() {
	return m_properties;
} 

BLESimpleService* BLESimpleCharacteristic::getService() {
	return m_pService;
} 

BLESimpleUUID BLESimpleCharacteristic::getUUID() {
	return m_bleUUID;
} 

std::string BLESimpleCharacteristic::getValue() {
	return m_value.getValue();
} 

void BLESimpleCharacteristic::handleGATTServerEvent(
		esp_gatts_cb_event_t      event,
		esp_gatt_if_t             gatts_if,
		esp_ble_gatts_cb_param_t* param) {

	switch(event) {
		case ESP_GATTS_EXEC_WRITE_EVT: {
			if(m_writeEvt){
				m_writeEvt = false;
				if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC) {
					m_value.commit();
					m_pCallbacks->onWrite(this); 
				} else {
					m_value.cancel();
				}
				::esp_ble_gatts_send_response(
						gatts_if,
						param->write.conn_id,
						param->write.trans_id, ESP_GATT_OK, nullptr);
			}
			break;
		} 
		case ESP_GATTS_ADD_CHAR_EVT: {
			if (getHandle() == param->add_char.attr_handle) 
				m_semaphoreCreateEvt.give();
			break;
		} 
		case ESP_GATTS_WRITE_EVT: {
			if (param->write.handle == m_handle) {
				if (param->write.is_prep) {
					m_value.addPart(param->write.value, param->write.len);
					m_writeEvt = true;
				} else {
					setValue(param->write.value, param->write.len);
				}

				if (param->write.need_rsp) {
					esp_gatt_rsp_t rsp;

					rsp.attr_value.len      = param->write.len;
					rsp.attr_value.handle   = m_handle;
					rsp.attr_value.offset   = param->write.offset;
					rsp.attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
					memcpy(rsp.attr_value.value, param->write.value, param->write.len);

					::esp_ble_gatts_send_response(
							gatts_if,
							param->write.conn_id,
							param->write.trans_id, ESP_GATT_OK, &rsp);
				
				}

				if (param->write.is_prep != true) {
					m_pCallbacks->onWrite(this); 
				}
			} 
			break;
		} 
		case ESP_GATTS_READ_EVT: {
			if (param->read.handle == m_handle) {
				uint16_t maxOffset =  getService()->getServer()->getPeerMTU(param->read.conn_id) - 1;
				if (param->read.need_rsp) {
					esp_gatt_rsp_t rsp;

					if (param->read.is_long) {
						std::string value = m_value.getValue();

						if (value.length() - m_value.getReadOffset() < maxOffset) {
							rsp.attr_value.len    = value.length() - m_value.getReadOffset();
							rsp.attr_value.offset = m_value.getReadOffset();
							memcpy(rsp.attr_value.value, value.data() + rsp.attr_value.offset, rsp.attr_value.len);
							m_value.setReadOffset(0);
						} else {
							rsp.attr_value.len    = maxOffset;
							rsp.attr_value.offset = m_value.getReadOffset();
							memcpy(rsp.attr_value.value, value.data() + rsp.attr_value.offset, rsp.attr_value.len);
							m_value.setReadOffset(rsp.attr_value.offset + maxOffset);
						}
					} else { 

						m_pCallbacks->onRead(this);

						std::string value = m_value.getValue();

						if (value.length() + 1 > maxOffset) {
							m_value.setReadOffset(maxOffset);
							rsp.attr_value.len    = maxOffset;
							rsp.attr_value.offset = 0;
							memcpy(rsp.attr_value.value, value.data(), rsp.attr_value.len);
						} else {
							rsp.attr_value.len    = value.length();
							rsp.attr_value.offset = 0;
							memcpy(rsp.attr_value.value, value.data(), rsp.attr_value.len);
						}
					}
					rsp.attr_value.handle   = param->read.handle;
					rsp.attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;

					::esp_ble_gatts_send_response(
							gatts_if, param->read.conn_id,
							param->read.trans_id,
							ESP_GATT_OK,
							&rsp);
				} 
			} 
			break;
		}

		case ESP_GATTS_CONF_EVT: {
			if(param->conf.conn_id == getService()->getServer()->getConnId())
				m_semaphoreConfEvt.give(param->conf.status);
			break;
		}

		case ESP_GATTS_CONNECT_EVT: {
			break;
		}

		case ESP_GATTS_DISCONNECT_EVT: {
			m_semaphoreConfEvt.give();
			break;
		}

		default: {
			break;
		} 

	} 

	m_descriptorMap.handleGATTServerEvent(event, gatts_if, param);
} 


void BLESimpleCharacteristic::indicate() {
	notify(false);
} 

void BLESimpleCharacteristic::notify(bool is_notification) {
	assert(getService() != nullptr);
	assert(getService()->getServer() != nullptr);

	m_pCallbacks->onNotify(this); 

	//hexDump((uint8_t*)m_value.getValue().data(), m_value.getValue().length());

	if (getService()->getServer()->getConnectedCount() == 0) {
		m_pCallbacks->onStatus(this, BLESimpleCharacteristicCallbacks::Status::ERROR_NO_CLIENT, 0);
		return;
	}

	BLES2902 *p2902 = (BLES2902*)getDescriptorByUUID((uint16_t)0x2902);
	if(is_notification) {
		if (p2902 != nullptr && !p2902->getNotifications()) {
			m_pCallbacks->onStatus(this, BLESimpleCharacteristicCallbacks::Status::ERROR_NOTIFY_DISABLED, 0); 
			return;
		}
	}
	else{
		if (p2902 != nullptr && !p2902->getIndications()) {
			m_pCallbacks->onStatus(this, BLESimpleCharacteristicCallbacks::Status::ERROR_INDICATE_DISABLED, 0); 
			return;
		}
	}
	for (auto &myPair : getService()->getServer()->getPeerDevices(false)) {

		size_t length = m_value.getValue().length();
		if(!is_notification) 
			m_semaphoreConfEvt.take("indicate");
		esp_err_t errRc = ::esp_ble_gatts_send_indicate(getService()->getServer()->getGattsIf(),
				myPair.first,getHandle(), length, (uint8_t*)m_value.getValue().data(), !is_notification); 
		if (errRc != ESP_OK) {
			m_semaphoreConfEvt.give();
			m_pCallbacks->onStatus(this, BLESimpleCharacteristicCallbacks::Status::ERROR_GATT, errRc); 
			return;
		}
		if(!is_notification){ 
			if(!m_semaphoreConfEvt.timedWait("indicate", indicationTimeout)){
				m_pCallbacks->onStatus(this, BLESimpleCharacteristicCallbacks::Status::ERROR_INDICATE_TIMEOUT, 0); 
			} else {
				auto code = (esp_gatt_status_t) m_semaphoreConfEvt.value();
				if(code == ESP_GATT_OK) {
					m_pCallbacks->onStatus(this, BLESimpleCharacteristicCallbacks::Status::SUCCESS_INDICATE, code); 
				} else {
					m_pCallbacks->onStatus(this, BLESimpleCharacteristicCallbacks::Status::ERROR_INDICATE_FAILURE, code);
				}
			}
		} else {
			m_pCallbacks->onStatus(this, BLESimpleCharacteristicCallbacks::Status::SUCCESS_NOTIFY, 0); 
		}
	}
} 

void BLESimpleCharacteristic::setBroadcastProperty(bool value) {

	if (value) {
		m_properties = (esp_gatt_char_prop_t)(m_properties | ESP_GATT_CHAR_PROP_BIT_BROADCAST);
	} else {
		m_properties = (esp_gatt_char_prop_t)(m_properties & ~ESP_GATT_CHAR_PROP_BIT_BROADCAST);
	}
} 

void BLESimpleCharacteristic::setCallbacks(BLESimpleCharacteristicCallbacks* pCallbacks) {
	if (pCallbacks != nullptr){
		m_pCallbacks = pCallbacks;
	} else {
		m_pCallbacks = &defaultCallback;
	}
} 

void BLESimpleCharacteristic::setHandle(uint16_t handle) {
	m_handle = handle;
} 

void BLESimpleCharacteristic::setIndicateProperty(bool value) {

	if (value) {
		m_properties = (esp_gatt_char_prop_t)(m_properties | ESP_GATT_CHAR_PROP_BIT_INDICATE);
	} else {
		m_properties = (esp_gatt_char_prop_t)(m_properties & ~ESP_GATT_CHAR_PROP_BIT_INDICATE);
	}
} 

void BLESimpleCharacteristic::setNotifyProperty(bool value) {

	if (value) {
		m_properties = (esp_gatt_char_prop_t)(m_properties | ESP_GATT_CHAR_PROP_BIT_NOTIFY);
	} else {
		m_properties = (esp_gatt_char_prop_t)(m_properties & ~ESP_GATT_CHAR_PROP_BIT_NOTIFY);
	}
} 

void BLESimpleCharacteristic::setReadProperty(bool value) {

	if (value) {
		m_properties = (esp_gatt_char_prop_t)(m_properties | ESP_GATT_CHAR_PROP_BIT_READ);
	} else {
		m_properties = (esp_gatt_char_prop_t)(m_properties & ~ESP_GATT_CHAR_PROP_BIT_READ);
	}
} 

void BLESimpleCharacteristic::setValue(uint8_t* data, size_t length) {

	if (length > ESP_GATT_MAX_ATTR_LEN) 
		return;
	m_semaphoreSetValue.take();  
	m_value.setValue(data, length);
	m_semaphoreSetValue.give();  
} 



void BLESimpleCharacteristic::setValue(std::string value) {
	setValue((uint8_t*)(value.data()), value.length());
} 

void BLESimpleCharacteristic::setValue(uint16_t& data16) {
	uint8_t temp[2];
	temp[0] = data16;
	temp[1] = data16 >> 8;
	setValue(temp, 2);
} 

void BLESimpleCharacteristic::setValue(uint32_t& data32) {
	uint8_t temp[4];
	temp[0] = data32;
	temp[1] = data32 >> 8;
	temp[2] = data32 >> 16;
	temp[3] = data32 >> 24;
	setValue(temp, 4);
} 

void BLESimpleCharacteristic::setValue(int& data32) {
	uint8_t temp[4];
	temp[0] = data32;
	temp[1] = data32 >> 8;
	temp[2] = data32 >> 16;
	temp[3] = data32 >> 24;
	setValue(temp, 4);
} 

void BLESimpleCharacteristic::setValue(float& data32) {
	float temp = data32;
	setValue((uint8_t*)&temp, 4);
} 

void BLESimpleCharacteristic::setValue(double& data64) {
	double temp = data64;
	setValue((uint8_t*)&temp, 8);
} 

void BLESimpleCharacteristic::setWriteNoResponseProperty(bool value) {

	if (value) {
		m_properties = (esp_gatt_char_prop_t)(m_properties | ESP_GATT_CHAR_PROP_BIT_WRITE_NR);
	} else {
		m_properties = (esp_gatt_char_prop_t)(m_properties & ~ESP_GATT_CHAR_PROP_BIT_WRITE_NR);
	}
} 


void BLESimpleCharacteristic::setWriteProperty(bool value) {

	if (value) {
		m_properties = (esp_gatt_char_prop_t)(m_properties | ESP_GATT_CHAR_PROP_BIT_WRITE);
	} else {
		m_properties = (esp_gatt_char_prop_t)(m_properties & ~ESP_GATT_CHAR_PROP_BIT_WRITE);
	}
} 

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BLESimpleDescriptor* BLESimpleDescriptorMap::getByUUID(const char* uuid) {
	return getByUUID(BLESimpleUUID(uuid));
}

BLESimpleDescriptor* BLESimpleDescriptorMap::getByUUID(BLESimpleUUID uuid) {
	for (auto &myPair : m_uuidMap) {
		if (myPair.first->getUUID().equals(uuid)) {
			return myPair.first;
		}
	}
	return nullptr;
} 

BLESimpleDescriptor* BLESimpleDescriptorMap::getByHandle(uint16_t handle) {
	return m_handleMap.at(handle);
} 

void BLESimpleDescriptorMap::setByUUID(const char* uuid, BLESimpleDescriptor* pDescriptor){
	m_uuidMap.insert(std::pair<BLESimpleDescriptor*, std::string>(pDescriptor, uuid));
} 

void BLESimpleDescriptorMap::setByUUID(BLESimpleUUID uuid, BLESimpleDescriptor* pDescriptor) {
	m_uuidMap.insert(std::pair<BLESimpleDescriptor*, std::string>(pDescriptor, uuid.toString()));
} 

void BLESimpleDescriptorMap::setByHandle(uint16_t handle, BLESimpleDescriptor* pDescriptor) {
	m_handleMap.insert(std::pair<uint16_t, BLESimpleDescriptor*>(handle, pDescriptor));
} 


void BLESimpleDescriptorMap::handleGATTServerEvent(
		esp_gatts_cb_event_t      event,
		esp_gatt_if_t             gatts_if,
		esp_ble_gatts_cb_param_t* param) {
	for (auto &myPair : m_uuidMap) {
		myPair.first->handleGATTServerEvent(event, gatts_if, param);
	}
} 


BLESimpleDescriptor* BLESimpleDescriptorMap::getFirst() {
	m_iterator = m_uuidMap.begin();
	if (m_iterator == m_uuidMap.end()) return nullptr;
	BLESimpleDescriptor* pRet = m_iterator->first;
	m_iterator++;
	return pRet;
} 


BLESimpleDescriptor* BLESimpleDescriptorMap::getNext() {
	if (m_iterator == m_uuidMap.end()) return nullptr;
	BLESimpleDescriptor* pRet = m_iterator->first;
	m_iterator++;
	return pRet;
} 