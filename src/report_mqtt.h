#ifndef ALIYUN_MQTT_INCLUDED
#define ALIYUN_MQTT_INCLUDED

#include <functional>
#include <string>
#include <map>
#include "psram_buffer.h"
#include "mqtt_client.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MQTT_MSG_CALLBACK std::function<void(const char *)> callback

    class ReportMQTT
    {
    public:
        static void InitMQTT(int core_index, MQTT_MSG_CALLBACK);
        
        ReportMQTT();
        virtual ~ReportMQTT();
        
        static bool publish(const char *payload);
        static bool publish_p2p(const char*payload);
    
        bool connect();
        void disconnect();

        void on_connect();
        void on_disconnect();

        bool connected();

        int connect_state();

        class PublishBufferHelper : public PsramBuffer
        {
        public:
            PublishBufferHelper(const char *payload);
            PublishBufferHelper(const char *payload, unsigned int length);

            std::string get_payload();
        };

    private:
        esp_mqtt_client_handle_t _mqtt_client;
        esp_mqtt_connect_return_code_t _connect_code;
        bool _connected;

    private:
        static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
        static void CreateMQTTService(void *pvParameters);
        static void MQTTMessagePatch(void *pvParameters);

        static void on_topic_callback(const char *topic, char *payload, unsigned int length);

        static bool MqttPrepare();

        void publish_msg();

        void publish(const char* topic, const char* payload);
        void set_connected(bool connected);

        void set_connect_state(esp_mqtt_connect_return_code_t state);
    };

#ifdef __cplusplus
}
#endif

#endif // ALIYUN_MQTT_INCLUDED