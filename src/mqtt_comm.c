#include "mqtt_comm.h"    // Header file com as declarações locais
// Base: https://github.com/BitDogLab/BitDogLab-C/blob/main/wifi_button_and_led/lwipopts.h
#include "lwipopts.h"             // Configurações customizadas do lwIP
#include "xor_cipher.h"
#include "string.h"

/* Variável global estática para armazenar a instância do cliente MQTT
 * 'static' limita o escopo deste arquivo */
static mqtt_client_t *client;
static bool is_connected = false;  // Flag para verificar conexão
static unsigned long int last_timestamp_received = 0;
static float last_value_received = 0.0;
static char last_topic_received[MAX_BYTES_RECEIVED];  // Tópico da última mensagem recebida
static bool has_new_value = false;
static bool has_new_topic = false;

/* Callback de conexão MQTT - chamado quando o status da conexão muda
 * Parâmetros:
 *   - client: instância do cliente MQTT
 *   - arg: argumento opcional (não usado aqui)
 *   - status: resultado da tentativa de conexão */
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    is_connected = (status == MQTT_CONNECT_ACCEPTED);  // Atualiza flag de conexão
    #ifdef DEBUG_MQTT
    if (is_connected) {
        printf("Conectado ao broker MQTT com sucesso!\n");
    } else {
        printf("Falha ao conectar ao broker, código: %d\n", status);
    }
    #endif
}

/* Função para configurar e iniciar a conexão MQTT
 * Parâmetros:
 *   - client_id: identificador único para este cliente
 *   - broker_ip: endereço IP do broker como string (ex: "192.168.1.1")
 *   - user: nome de usuário para autenticação (pode ser NULL)
 *   - pass: senha para autenticação (pode ser NULL) */
bool mqtt_setup(const char *client_id, const char *broker_ip, const unsigned int port, const char *user, const char *pass) {
    ip_addr_t broker_addr;  // Estrutura para armazenar o IP do broker
    
    // Converte o IP de string para formato numérico
    if (ip4addr_aton(broker_ip, &broker_addr) == 0) {
        #ifdef DEBUG_MQTT
        printf("Erro no IP\n");
        #endif
        return false;
    }

    // Cria uma nova instância do cliente MQTT
    client = mqtt_client_new();
    if (client == NULL) {
        #ifdef DEBUG_MQTT
        printf("Falha ao criar o cliente MQTT\n");
        #endif
        return false;
    }

    // Configura as informações de conexão do cliente
    struct mqtt_connect_client_info_t ci = {
        .client_id = client_id,  // ID do cliente
        .client_user = user,     // Usuário (opcional)
        .client_pass = pass      // Senha (opcional)
    };

    // Inicia a conexão com o broker
    // Parâmetros:
    //   - client: instância do cliente
    //   - &broker_addr: endereço do broker
    //   - 1883: porta padrão MQTT
    //   - mqtt_connection_cb: callback de status
    //   - NULL: argumento opcional para o callback
    //   - &ci: informações de conexão
    return mqtt_client_connect(client, &broker_addr, port, mqtt_connection_cb, NULL, &ci) == ERR_OK;
}

/* Callback de confirmação de publicação
 * Chamado quando o broker confirma recebimento da mensagem (para QoS > 0)
 * Parâmetros:
 *   - arg: argumento opcional
 *   - result: código de resultado da operação */
void mqtt_pub_request_cb(void *arg, err_t result) {
    #ifdef DEBUG_MQTT
    if (result == ERR_OK) {
        printf("Publicação MQTT enviada com sucesso!\n");
    } else {
        printf("Erro ao publicar via MQTT: %d\n", result);
    }
    #endif
}

void mqtt_on_request(void *arg, err_t err) {
    if (err == ERR_OK) {
        #ifdef DEBUG_MQTT
        printf("Publicação MQTT bem-sucedida!\n");
        #endif
    } else {
        #ifdef DEBUG_MQTT
        printf("Erro na publicação MQTT: %d\n", err);
        #endif
    }
}

void mqtt_on_message(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    unsigned long int new_timestamp = 0;
    char msg_decrypted[MAX_BYTES_RECEIVED];
    float value = 0.0;
    #ifdef DEBUG_MQTT
    printf("Data received (mqtt_on_message): %.*s\n", (int)len, data);
    #endif
    
    xor_encrypt(data, msg_decrypted, len, CIPHER_KEY); // Exemplo de criptografia
    if (sscanf(msg_decrypted, "{\"valor\":%f,\"ts\":%lu}", &value, &new_timestamp) != 2) {
        #ifdef DEBUG_MQTT
        printf("Erro no parse da mensagem! (%.1f) (%lu)\n", value, new_timestamp);
        #endif
        return;
    }

    if (new_timestamp > last_timestamp_received) {
        last_timestamp_received = new_timestamp;
        #ifdef DEBUG_MQTT
        printf("(mqtt_on_message) Nova leitura: %.2f (ts: %lu)\n", value, new_timestamp);
        #endif
       
        // --> Processar dados aqui <--
        has_new_value = true;  // Marca que há novos dados disponíveis
        last_value_received = value;  // Atualiza o último valor recebido
    } else {
        #ifdef DEBUG_MQTT
        printf("Replay detectado (ts: %lu <= %lu)\n",
              new_timestamp, last_timestamp_received);
        #endif
    }
}

float mqtt_get_last_value(void) {
    // Retorna o último valor recebido
    has_new_value = false;  // Reseta a flag de novos dados
    return last_value_received;
}

unsigned long int mqtt_get_last_timestamp(void) {
    // Retorna o timestamp do último valor recebido
    return last_timestamp_received;
}

/* Função para publicar dados em um tópico MQTT
 * Parâmetros:
 *   - topic: nome do tópico (ex: "sensor/temperatura")
 *   - data: payload da mensagem (bytes)
 *   - len: tamanho do payload */
bool mqtt_comm_publish(const char *topic, const uint8_t *data, size_t len) {
    // Envia a mensagem MQTT
    err_t status = mqtt_publish(
        client,              // Instância do cliente
        topic,               // Tópico de publicação
        data,                // Dados a serem enviados
        len,                 // Tamanho dos dados
        0,                   // QoS 0 (nenhuma confirmação)
        0,                   // Não reter mensagem
        mqtt_pub_request_cb, // Callback de confirmação
        NULL                 // Argumento para o callback
    );

    bool is_success = (status == ERR_OK);  // Verifica se a publicação foi bem-sucedida
    #ifdef DEBUG_MQTT
    if (!is_success) {
        printf("mqtt_publish falhou ao ser enviada: %d\n", status);
    }
    #endif
    return is_success;  // Retorna o status da operação
}

bool mqtt_comm_subscribe(
    const char *topic,
    mqtt_request_cb_t cb_resp_sub,
    mqtt_incoming_publish_cb_t pub_cb,
    mqtt_incoming_data_cb_t data_cb
) {
    // Assina o tópico MQTT
    err_t status = mqtt_subscribe(
        client,              // Instância do cliente
        topic,              // Tópico a ser assinado
        0,                   // QoS 0 (nenhuma confirmação)
        cb_resp_sub,           // Callback para confirmação de assinatura?
        NULL                // Argumento para o callback
    );
    bool is_success = (status == ERR_OK);  // Verifica se a assinatura foi bem-sucedida
    #ifdef DEBUG_MQTT
    if (!is_success) {
        printf("Assinatura do tópico '%s' falhou: %d\n", topic, status);
        return false;  // Retorna falso se a assinatura falhar
    } else {
        printf("Assinatura do tópico '%s' bem-sucedida!\n", topic);
    }
    #endif

    mqtt_set_inpub_callback(
        client,              // Instância do cliente
        pub_cb,     // Callback para mensagens recebidas
        data_cb,                // Callback de dados (não usado aqui)
        NULL                 // Argumento para o callback
    );

    return true;
}

void mqtt_on_incoming_publish(void *arg, const char *topic, u32_t tot_len) {
    // Callback chamado quando uma nova mensagem é publicada no tópico
    #ifdef DEBUG_MQTT
    printf("Nova mensagem recebida no tópico '%s' com tamanho %u bytes\n", topic, tot_len);
    #endif
    // Aqui você pode processar a mensagem recebida
    has_new_topic = true;  // Marca que há um novo tópico
    strcpy(last_topic_received, topic);  // Armazena o tópico da última mensagem recebida
}

bool mqtt_is_connected(void) {
    // Retorna o status da conexão MQTT
    return is_connected;
}

const char* mqtt_get_last_topic(void) {
    // Retorna o tópico da última mensagem recebida
    has_new_topic = false;  // Reseta a flag de novos dados
    return last_topic_received;
}

bool mqtt_has_new_data(void) {
    // Retorna se há novos dados disponíveis
    return has_new_topic && has_new_value;
}
