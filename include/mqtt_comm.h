#ifndef MQTT_COMM_H
#define MQTT_COMM_H
#include "lwip/apps/mqtt.h"
#define DEBUG_MQTT
#define MAX_BYTES_RECEIVED 64
bool mqtt_setup(const char *client_id, const char *broker_ip, const unsigned int port, const char *user, const char *pass);
bool mqtt_is_connected(void);
bool mqtt_comm_publish(const char *topic, const uint8_t *data, size_t len);
bool mqtt_comm_subscribe(
  const char *topic,
  mqtt_request_cb_t cb_resp_sub,
  mqtt_incoming_publish_cb_t pub_cb,
  mqtt_incoming_data_cb_t data_cb
);
void mqtt_on_message(void *arg, const u8_t *data, u16_t len, u8_t flags);
void mqtt_on_request(void *arg, err_t err);
void mqtt_on_incoming_publish(void *arg, const char *topic, u32_t tot_len);
float mqtt_get_last_value(void);
unsigned long int mqtt_get_last_timestamp(void);
const char* mqtt_get_last_topic(void);
bool mqtt_has_new_data(void);
#endif