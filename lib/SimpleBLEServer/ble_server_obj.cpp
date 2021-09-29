#include "ble_server_obj.h" 
#include "ble_server_device.h" 

BLESimpleService* BLESimpleServiceMap::getByUUID(const char* uuid) {
	return getByUUID(BLESimpleUUID(uuid));
}


BLESimpleService* BLESimpleServiceMap::getByUUID(BLESimpleUUID uuid, uint8_t inst_id) {
	for (auto &myPair : m_uuidMap) {
		if (myPair.first->getUUID().equals(uuid)) {
			return myPair.first;
		}
	}
	return nullptr;
} 

BLESimpleService* BLESimpleServiceMap::getByHandle(uint16_t handle) {
	return m_handleMap.at(handle);
} 

void BLESimpleServiceMap::setByUUID(BLESimpleUUID uuid, BLESimpleService* service) {
	m_uuidMap.insert(std::pair<BLESimpleService*, std::string>(service, uuid.toString()));
} 

void BLESimpleServiceMap::setByHandle(uint16_t handle, BLESimpleService* service) {
	m_handleMap.insert(std::pair<uint16_t, BLESimpleService*>(handle, service));
} 

void BLESimpleServiceMap::handleGATTServerEvent(
		esp_gatts_cb_event_t      event,
		esp_gatt_if_t             gatts_if,
		esp_ble_gatts_cb_param_t* param) {

	for (auto &myPair : m_uuidMap) {
		myPair.first->handleGATTServerEvent(event, gatts_if, param);
	}
}

BLESimpleService* BLESimpleServiceMap::getFirst() {
	m_iterator = m_uuidMap.begin();
	if (m_iterator == m_uuidMap.end()) return nullptr;
	BLESimpleService* pRet = m_iterator->first;
	m_iterator++;
	return pRet;
} 


BLESimpleService* BLESimpleServiceMap::getNext() {
	if (m_iterator == m_uuidMap.end()) return nullptr;
	BLESimpleService* pRet = m_iterator->first;
	m_iterator++;
	return pRet;
} 

void BLESimpleServiceMap::removeService(BLESimpleService* service) {
	m_handleMap.erase(service->getHandle());
	m_uuidMap.erase(service);
} 


BLESimpleServer::BLESimpleServer() {
	m_appId            = ESP_GATT_IF_NONE;
	m_gatts_if         = ESP_GATT_IF_NONE;
	m_connectedCount   = 0;
	m_connId           = ESP_GATT_IF_NONE;
	m_pServerCallbacks = nullptr;
} 


void BLESimpleServer::createApp(uint16_t appId) {
	m_appId = appId;
	registerApp(appId);
} 

BLESimpleService* BLESimpleServer::createService(const char* uuid) {
	return createService(BLESimpleUUID(uuid));
}

BLESimpleService* BLESimpleServer::createService(BLESimpleUUID uuid, uint32_t numHandles, uint8_t inst_id) {

	m_semaphoreCreateEvt.take("createService");

	BLESimpleService* pService = new BLESimpleService(uuid, numHandles);
	pService->m_instId = inst_id;
	m_serviceMap.setByUUID(uuid, pService); // Save a reference to this service being on this server.
	pService->executeCreate(this);          // Perform the API calls to actually create the service.

	m_semaphoreCreateEvt.wait("createService");

	return pService;
} 


BLESimpleService* BLESimpleServer::getServiceByUUID(const char* uuid) {
	return m_serviceMap.getByUUID(uuid);
}

BLESimpleService* BLESimpleServer::getServiceByUUID(BLESimpleUUID uuid) {
	return m_serviceMap.getByUUID(uuid);
}

uint16_t BLESimpleServer::getConnId() {
	return m_connId;
}


uint32_t BLESimpleServer::getConnectedCount() {
	return m_connectedCount;
} 

uint16_t BLESimpleServer::getGattsIf() {
	return m_gatts_if;
}

void BLESimpleServer::handleGATTServerEvent(esp_gatts_cb_event_t event, 
	esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param) {

	switch(event) {
		case ESP_GATTS_ADD_CHAR_EVT: {
			break;
		} 
		case ESP_GATTS_MTU_EVT:
			updatePeerMTU(param->mtu.conn_id, param->mtu.mtu);
			break;
		case ESP_GATTS_CONNECT_EVT: {
			m_connId = param->connect.conn_id;
			addPeerDevice((void*)this, false, m_connId);
			if (m_pServerCallbacks != nullptr) {
				m_pServerCallbacks->onConnect(this);
				m_pServerCallbacks->onConnect(this, param);			
			}
			m_connectedCount++;   // Increment the number of connected devices count.	
			break;
		} // ESP_GATTS_CONNECT_EVT
		case ESP_GATTS_CREATE_EVT: {
			BLESimpleService* pService = m_serviceMap.getByUUID(param->create.service_id.id.uuid, param->create.service_id.id.inst_id);  
			m_serviceMap.setByHandle(param->create.service_handle, pService);
			m_semaphoreCreateEvt.give();
			break;
		} 
		case ESP_GATTS_DISCONNECT_EVT: {
			if (m_pServerCallbacks != nullptr) {
				m_pServerCallbacks->onDisconnect(this);
			}
            if(m_connId == ESP_GATT_IF_NONE) {
                return;
            }
			if(removePeerDevice(param->disconnect.conn_id, false)) {
                m_connectedCount--; 
            }
            break;
		} 
		case ESP_GATTS_READ_EVT: {
			break;
		} 
		case ESP_GATTS_REG_EVT: {
			m_gatts_if = gatts_if;
			m_semaphoreRegisterAppEvt.give(); 
			break;
		} 
		case ESP_GATTS_WRITE_EVT: {
			break;
		}
		case ESP_GATTS_OPEN_EVT:
			m_semaphoreOpenEvt.give(param->open.status);
			break;

		default:
			break;
	}


	m_serviceMap.handleGATTServerEvent(event, gatts_if, param);
} 

void BLESimpleServer::registerApp(uint16_t m_appId) {
	m_semaphoreRegisterAppEvt.take("registerApp"); 
	::esp_ble_gatts_app_register(m_appId);
	m_semaphoreRegisterAppEvt.wait("registerApp");
} 


void BLESimpleServer::setCallbacks(BLESimpleServerCallbacks* pCallbacks) {
	m_pServerCallbacks = pCallbacks;
} 

void BLESimpleServer::removeService(BLESimpleService* service) {
	service->stop();
	service->executeDelete();	
	m_serviceMap.removeService(service);
}

void BLESimpleServer::updatePeerMTU(uint16_t conn_id, uint16_t mtu) {

	const std::map<uint16_t, conn_status_t>::iterator it = m_connectedServersMap.find(conn_id);
	if (it != m_connectedServersMap.end()) {
		it->second.mtu = mtu;
		std::swap(m_connectedServersMap[conn_id], it->second);
	}
}

std::map<uint16_t, conn_status_t> BLESimpleServer::getPeerDevices(bool _client) {
	return m_connectedServersMap;
}

uint16_t BLESimpleServer::getPeerMTU(uint16_t conn_id) {
	return m_connectedServersMap.find(conn_id)->second.mtu;
}

void BLESimpleServer::addPeerDevice(void* peer, bool _client, uint16_t conn_id) {
	conn_status_t status = {
		.peer_device = peer,
		.connected = true,
		.mtu = 23
	};

	m_connectedServersMap.insert(std::pair<uint16_t, conn_status_t>(conn_id, status));	
}

bool BLESimpleServer::removePeerDevice(uint16_t conn_id, bool _client) {
	return m_connectedServersMap.erase(conn_id) > 0;
}

void BLESimpleServer::disconnect(uint16_t connId) {
	esp_ble_gatts_close(m_gatts_if, connId);
}