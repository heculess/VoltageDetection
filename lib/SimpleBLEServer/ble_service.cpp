#include "ble_service.h" 
#include "ble_server_obj.h" 

#define NULL_HANDLE (0xffff)

BLESimpleService::BLESimpleService(const char* uuid, uint16_t numHandles) : 
BLESimpleService(BLESimpleUUID(uuid), numHandles) {
}

BLESimpleService::BLESimpleService(BLESimpleUUID uuid, uint16_t numHandles) {
	m_uuid      = uuid;
	m_handle    = NULL_HANDLE;
	m_pServer   = nullptr;

	m_lastCreatedCharacteristic = nullptr;
	m_numHandles = numHandles;
} 

void BLESimpleService::executeCreate(BLESimpleServer* pServer) {

	m_pServer          = pServer;
	m_semaphoreCreateEvt.take("executeCreate"); // Take the mutex and release at event ESP_GATTS_CREATE_EVT

	esp_gatt_srvc_id_t srvc_id;
	srvc_id.is_primary = true;
	srvc_id.id.inst_id = m_instId;
	srvc_id.id.uuid    = *m_uuid.getNative();
	esp_err_t errRc = ::esp_ble_gatts_create_service(getServer()->getGattsIf(), &srvc_id, m_numHandles); 

	if (errRc != ESP_OK) 
		return;

	m_semaphoreCreateEvt.wait("executeCreate");
} 


void BLESimpleService::executeDelete() {

	m_semaphoreDeleteEvt.take("executeDelete"); // Take the mutex and release at event ESP_GATTS_DELETE_EVT

	esp_err_t errRc = ::esp_ble_gatts_delete_service(getHandle());

	if (errRc != ESP_OK) 
		return;

	m_semaphoreDeleteEvt.wait("executeDelete");
} 

BLESimpleUUID BLESimpleService::getUUID() {
	return m_uuid;
} 

void BLESimpleService::start() {

	if (m_handle == NULL_HANDLE) 
		return;

	BLESimpleCharacteristic *pCharacteristic = m_characteristicMap.getFirst();

	while (pCharacteristic != nullptr) {
		m_lastCreatedCharacteristic = pCharacteristic;
		pCharacteristic->executeCreate(this);

		pCharacteristic = m_characteristicMap.getNext();
	}

	m_semaphoreStartEvt.take("start");
	esp_err_t errRc = ::esp_ble_gatts_start_service(m_handle);

	if (errRc != ESP_OK)
		return;

	m_semaphoreStartEvt.wait("start");
} 

void BLESimpleService::stop() {

	if (m_handle == NULL_HANDLE) 
		return;

	m_semaphoreStopEvt.take("stop");
	esp_err_t errRc = ::esp_ble_gatts_stop_service(m_handle);

	if (errRc != ESP_OK) 
		return;

	m_semaphoreStopEvt.wait("stop");
} 

void BLESimpleService::setHandle(uint16_t handle) {

	if (m_handle != NULL_HANDLE) 
		return;

	m_handle = handle;
} 

uint16_t BLESimpleService::getHandle() {
	return m_handle;
} 

void BLESimpleService::addCharacteristic(BLESimpleCharacteristic* pCharacteristic) {
	m_characteristicMap.setByUUID(pCharacteristic, pCharacteristic->getUUID());
} 

BLESimpleCharacteristic* BLESimpleService::createCharacteristic(const char* uuid, uint32_t properties) {
	return createCharacteristic(BLESimpleUUID(uuid), properties);
}
	
BLESimpleCharacteristic* BLESimpleService::createCharacteristic(BLESimpleUUID uuid, uint32_t properties) {
	BLESimpleCharacteristic* pCharacteristic = new BLESimpleCharacteristic(uuid, properties);
	addCharacteristic(pCharacteristic);
	return pCharacteristic;
} 

void BLESimpleService::handleGATTServerEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param) {
	switch (event) {
		case ESP_GATTS_ADD_CHAR_EVT: {
			if (m_handle == param->add_char.service_handle) {
				BLESimpleCharacteristic *pCharacteristic = getLastCreatedCharacteristic();
				if (pCharacteristic == nullptr) 
					break;

				pCharacteristic->setHandle(param->add_char.attr_handle);
				m_characteristicMap.setByHandle(param->add_char.attr_handle, pCharacteristic);
				break;
			} 
			break;
		} 
		case ESP_GATTS_START_EVT: {
			if (param->start.service_handle == getHandle()) {
				m_semaphoreStartEvt.give();
			}
			break;
		}
		case ESP_GATTS_STOP_EVT: {
			if (param->stop.service_handle == getHandle()) {
				m_semaphoreStopEvt.give();
			}
			break;
		}
		case ESP_GATTS_CREATE_EVT: {
			if (getUUID().equals(BLESimpleUUID(param->create.service_id.id.uuid)) && m_instId == param->create.service_id.id.inst_id) {
				setHandle(param->create.service_handle);
				m_semaphoreCreateEvt.give();
			}
			break;
		} 

		case ESP_GATTS_DELETE_EVT: {
			if (param->del.service_handle == getHandle()) {
				m_semaphoreDeleteEvt.give();
			}
			break;
		} 

		default:
			break;
	} 
	m_characteristicMap.handleGATTServerEvent(event, gatts_if, param);
} 


BLESimpleCharacteristic* BLESimpleService::getCharacteristic(const char* uuid) {
	return getCharacteristic(BLESimpleUUID(uuid));
}


BLESimpleCharacteristic* BLESimpleService::getCharacteristic(BLESimpleUUID uuid) {
	return m_characteristicMap.getByUUID(uuid);
}

/*
std::string BLESimpleService::toString() {
	std::string res = "UUID: " + getUUID().toString();
	char hex[5];
	snprintf(hex, sizeof(hex), "%04x", getHandle());
	res += ", handle: 0x";
	res += hex;
	return res;
} 
*/
BLESimpleCharacteristic* BLESimpleService::getLastCreatedCharacteristic() {
	return m_lastCreatedCharacteristic;
} 

BLESimpleServer* BLESimpleService::getServer() {
	return m_pServer;
} 

BLESimpleCharacteristic* BLESimpleCharacteristicMap::getByHandle(uint16_t handle) {
	return m_handleMap.at(handle);
} 


BLESimpleCharacteristic* BLESimpleCharacteristicMap::getByUUID(const char* uuid) {
    return getByUUID(BLESimpleUUID(uuid));
}


BLESimpleCharacteristic* BLESimpleCharacteristicMap::getByUUID(BLESimpleUUID uuid) {
	for (auto &myPair : m_uuidMap) {
		if (myPair.first->getUUID().equals(uuid)) {
			return myPair.first;
		}
	}
	return nullptr;
} 

BLESimpleCharacteristic* BLESimpleCharacteristicMap::getFirst() {
	m_iterator = m_uuidMap.begin();
	if (m_iterator == m_uuidMap.end()) return nullptr;
	BLESimpleCharacteristic* pRet = m_iterator->first;
	m_iterator++;
	return pRet;
} 

BLESimpleCharacteristic* BLESimpleCharacteristicMap::getNext() {
	if (m_iterator == m_uuidMap.end()) return nullptr;
	BLESimpleCharacteristic* pRet = m_iterator->first;
	m_iterator++;
	return pRet;
} 

void BLESimpleCharacteristicMap::handleGATTServerEvent(esp_gatts_cb_event_t event, 
    esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param) {
	for (auto& myPair : m_uuidMap) {
		myPair.first->handleGATTServerEvent(event, gatts_if, param);
	}
} 

void BLESimpleCharacteristicMap::setByHandle(uint16_t handle, BLESimpleCharacteristic* characteristic) {
	m_handleMap.insert(std::pair<uint16_t, BLESimpleCharacteristic*>(handle, characteristic));
} 

void BLESimpleCharacteristicMap::setByUUID(BLESimpleCharacteristic* pCharacteristic, BLESimpleUUID uuid) {
	m_uuidMap.insert(std::pair<BLESimpleCharacteristic*, std::string>(pCharacteristic, uuid.toString()));
} 
