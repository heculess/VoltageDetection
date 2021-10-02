#include "report_mqtt.h"
#include <WiFi.h>
#include "device_core.h"
#include "hal_misc.h"
#include <esp_log.h>
#include "json_util.h"


static const char *TAG = "ReportMQTT";


static std::string mqttBroker("mqtt://");
static std::string mqttUserName;
static std::string mqttPassword;

static std::function<void(const char *)> _msg_callback = NULL;

static xQueueHandle _mqtt_evt_queue = xQueueCreate(2, sizeof(ReportMQTT::PublishBufferHelper *));
static xQueueHandle _mqtt_publish_queue = xQueueCreate(2, sizeof(ReportMQTT::PublishBufferHelper *));


ReportMQTT::PublishBufferHelper::PublishBufferHelper(const char *payload) : PsramBuffer(strlen(payload))
{
  memcpy(ptr(), payload, length());
}

ReportMQTT::PublishBufferHelper::PublishBufferHelper(const char *payload, unsigned int length) : PsramBuffer(length)
{
  memcpy(ptr(), payload, length);
}

std::string ReportMQTT::PublishBufferHelper::get_payload()
{
  return std::string(as_char_ptr(), length());
}

void ReportMQTT::mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
  esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
  ReportMQTT *_mqtt = (ReportMQTT *)(event->user_context);
  switch ((esp_mqtt_event_id_t)event_id)
  {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
    if (_mqtt)
    {
      _mqtt->on_connect();
    }
    break;
  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
    if (_mqtt)
    {
      _mqtt->on_disconnect();
    }
    break;
  case MQTT_EVENT_SUBSCRIBED:
    ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
    break;
  case MQTT_EVENT_UNSUBSCRIBED:
    ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
    break;
  case MQTT_EVENT_PUBLISHED:
    ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
    break;
  case MQTT_EVENT_DATA:
    ESP_LOGI(TAG, "MQTT_EVENT_DATA");
    ReportMQTT::on_topic_callback(std::string(event->topic, event->topic_len).c_str(),
                                  event->data, event->data_len);
    break;
  case MQTT_EVENT_ERROR:
    ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
    if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
    {
      ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
      ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
      ESP_LOGI(TAG, "Last captured errno : %d (%s)", event->error_handle->esp_transport_sock_errno,
               strerror(event->error_handle->esp_transport_sock_errno));
    }
    else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED)
    {
      if (_mqtt)
      {
        _mqtt->set_connect_state(event->error_handle->connect_return_code);
      }
      ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
    }
    else
    {
      ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
    }
    break;
  default:
    ESP_LOGI(TAG, "Other event id:%d", event->event_id);
    break;
  }
}

ReportMQTT::ReportMQTT() : _mqtt_client(NULL),
                           _connect_code(MQTT_CONNECTION_ACCEPTED),
                           _connected(false)
{
  esp_mqtt_client_config_t mqtt_cfg;
  memset(&mqtt_cfg, 0, sizeof(mqtt_cfg));
  //mqtt_cfg.uri = mqttBroker.c_str();
  mqtt_cfg.host = DeviceCore::get_mqtt_broker().c_str();
  mqtt_cfg.port = DeviceCore::get_mqtt_port();
  mqtt_cfg.client_id = DeviceCore::device_id().c_str();
  mqtt_cfg.username = mqttUserName.c_str();
  mqtt_cfg.password = mqttPassword.c_str();
  mqtt_cfg.keepalive = 15;
  mqtt_cfg.user_context = this;

  _mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(_mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
}

ReportMQTT::~ReportMQTT()
{
  disconnect();

  if (_mqtt_client)
  {
    esp_mqtt_client_stop(_mqtt_client);
    esp_mqtt_client_destroy(_mqtt_client);
    _mqtt_client = NULL;
  }
}

bool ReportMQTT::connect()
{
  if (mqttBroker.empty())
    return false;

  uint8_t mqttConnectTryCnt = 3;
  while (!connected() && mqttConnectTryCnt > 0)
  {
    ESP_LOGI(TAG, "Connecting to MQTT Server ...");
    if (esp_mqtt_client_start(_mqtt_client) == ESP_OK)
    {
      ESP_LOGI(TAG, "MQTT start!\r\n");
      return true;
    }
    else
    {
      int errCode = connect_state();
      ESP_LOGI(TAG, "MQTT connect failed, error code: %d", errCode);
      if (errCode != MQTT_CONNECTION_ACCEPTED)
      {
        ESP_LOGE(TAG, "No need to try again.");
        break;
      }
      vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
    mqttConnectTryCnt -= 1;
  }

  return false;
}

void ReportMQTT::disconnect()
{
  esp_mqtt_client_disconnect(_mqtt_client);
}

bool ReportMQTT::publish(const char *payload)
{
  PublishBufferHelper *pHelper = new PublishBufferHelper(payload);
  if (!pHelper->is_valid())
    return false;

  xQueueSend(_mqtt_publish_queue, &pHelper, portMAX_DELAY);

  return true;
}

void ReportMQTT::publish(const char *topic, const char *payload)
{
  esp_mqtt_client_publish(_mqtt_client, topic, payload, strnlen(payload, 1024), 0, 0);
}

void ReportMQTT::MQTTMessagePatch(void *pvParameters)
{
  WatchDog::buy_dog();
  while (true)
  {
    PublishBufferHelper *receive_msg = NULL;
    if (xQueueReceive(_mqtt_evt_queue, &receive_msg, 0) && receive_msg)
    {
      if (_msg_callback)
      {
        _msg_callback(receive_msg->get_payload().c_str());
      }

      delete receive_msg;
      receive_msg = NULL;
    }
    WatchDog::feed_dog();
    vTaskDelay(100);
  }
}

void ReportMQTT::on_topic_callback(const char *topic, char *payload, unsigned int length)
{
  PublishBufferHelper *string_send = new PublishBufferHelper((char *)payload, length);
  xQueueSend(_mqtt_evt_queue, &string_send, portMAX_DELAY);
}

void ReportMQTT::CreateMQTTService(void *pvParameters)
{
  if (!MqttPrepare())
  {
    ESP_LOGE(TAG, "MQTT init failed");
    DeviceCore::GotoDeepSleepAndExit(10);
  }

  xTaskCreate(MQTTMessagePatch, "MQTTMessagePatch", 4096, NULL,
              configMAX_PRIORITIES - 1, NULL);

  ReportMQTT mqtt_client;
  WatchDog::buy_dog();

  while (true)
  {
    WatchDog::feed_dog();
    if (WiFi.status() == WL_CONNECTED)
    {
      if (!mqtt_client.connected())
        mqtt_client.connect();
      
      mqtt_client.publish_msg();
    }
    vTaskDelay(200);
  }

  vTaskDelete(NULL);
}

void ReportMQTT::InitMQTT(int core_index, MQTT_MSG_CALLBACK)
{
  _msg_callback = callback;
  xTaskCreate(CreateMQTTService, "MQTTService", 4096, NULL,
              configMAX_PRIORITIES - 1, NULL);
}

bool ReportMQTT::MqttPrepare()
{

  mqttBroker += DeviceCore::get_mqtt_broker();
  mqttUserName = DeviceCore::get_mqtt_username();
  mqttPassword = DeviceCore::get_mqtt_password();

  printf("%s\r\n%s\r\n%s\r\n", mqttBroker.c_str(), mqttUserName.c_str(),mqttPassword.c_str());

  return true;
}

void ReportMQTT::publish_msg()
{
  PublishBufferHelper *buffer_helper = NULL;
  if (!xQueueReceive(_mqtt_publish_queue, &buffer_helper, 0) || (!buffer_helper))
    return;
  
  publish(DeviceCore::get_mqtt_topic().c_str(), buffer_helper->get_payload().c_str());
 
  delete buffer_helper;
  buffer_helper = NULL;
}

void ReportMQTT::on_connect()
{
  set_connected(true);
}

void ReportMQTT::on_disconnect()
{
  set_connected(false);
}

void ReportMQTT::set_connected(bool connected)
{
  _connected = connected;
}

bool ReportMQTT::connected()
{
  return _connected;
}

void ReportMQTT::set_connect_state(esp_mqtt_connect_return_code_t state)
{
  _connect_code = state;
}

int ReportMQTT::connect_state()
{
  return _connect_code;
}