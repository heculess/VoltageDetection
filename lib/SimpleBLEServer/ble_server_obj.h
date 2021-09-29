#ifndef BLE_SIMPLE_SERVER_INCLUDED
#define BLE_SIMPLE_SERVER_INCLUDED

#include "ble_service.h" 

#ifdef __cplusplus
extern "C" {
#endif


class BLESimpleServerCallbacks;

typedef struct {
	void *peer_device;
	bool connected;	
	uint16_t mtu;
} conn_status_t;


class BLESimpleServiceMap {
public:
	BLESimpleService* getByHandle(uint16_t handle);
	BLESimpleService* getByUUID(const char* uuid);	
	BLESimpleService* getByUUID(BLESimpleUUID uuid, uint8_t inst_id = 0);
	void        handleGATTServerEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param);
	void        setByHandle(uint16_t handle, BLESimpleService* service);
	void        setByUUID(const char* uuid, BLESimpleService* service);
	void        setByUUID(BLESimpleUUID uuid, BLESimpleService* service);

	BLESimpleService* getFirst();
	BLESimpleService* getNext();
	void 		removeService(BLESimpleService *service);

private:
	std::map<uint16_t, BLESimpleService*>    m_handleMap;
	std::map<BLESimpleService*, std::string> m_uuidMap;
	std::map<BLESimpleService*, std::string>::iterator m_iterator;
};


class BLESimpleServer {
public:
	uint32_t        getConnectedCount();
	BLESimpleService*     createService(const char* uuid);	
	BLESimpleService*     createService(BLESimpleUUID uuid, uint32_t numHandles=15, uint8_t inst_id=0);

	void            setCallbacks(BLESimpleServerCallbacks* pCallbacks);

	void 			removeService(BLESimpleService* service);
	BLESimpleService* 	getServiceByUUID(const char* uuid);
	BLESimpleService* 	getServiceByUUID(BLESimpleUUID uuid);

	void 			disconnect(uint16_t connId);
	uint16_t		m_appId;

	std::map<uint16_t, conn_status_t> getPeerDevices(bool client);
	void addPeerDevice(void* peer, bool is_client, uint16_t conn_id);
	bool removePeerDevice(uint16_t conn_id, bool client);
	BLESimpleServer* getServerByConnId(uint16_t conn_id);
	void updatePeerMTU(uint16_t connId, uint16_t mtu);
	uint16_t getPeerMTU(uint16_t conn_id);
	uint16_t        getConnId();


private:
	BLESimpleServer();
	friend class BLESimpleService;
	friend class BLESimpleCharacteristic;
	friend class BLESimpleServerDevice;
	esp_ble_adv_data_t  m_adv_data;

	uint16_t			m_connId;
	uint32_t            m_connectedCount;
	uint16_t            m_gatts_if;
  	std::map<uint16_t, conn_status_t> m_connectedServersMap;

	Semaphore m_semaphoreRegisterAppEvt 	= Semaphore("RegisterAppEvt");
	Semaphore m_semaphoreCreateEvt 		= Semaphore("CreateEvt");
	Semaphore m_semaphoreOpenEvt   		= Semaphore("OpenEvt");
	BLESimpleServiceMap m_serviceMap;
	BLESimpleServerCallbacks* m_pServerCallbacks = nullptr;

	void            createApp(uint16_t appId);
	uint16_t        getGattsIf();
	void            handleGATTServerEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
	void            registerApp(uint16_t);
}; 


class BLESimpleServerCallbacks {
public:
	virtual ~BLESimpleServerCallbacks() {};
	virtual void onConnect(BLESimpleServer* pServer) {};
	virtual void onConnect(BLESimpleServer* pServer, esp_ble_gatts_cb_param_t *param) {};
	virtual void onDisconnect(BLESimpleServer* pServer) {};
}; 


#ifdef __cplusplus
}
#endif


#endif // BLE_SIMPLE_SERVER_INCLUDED