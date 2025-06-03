// Bibliotecas necessárias
#include <string.h>                 // Para funções de string como strlen()
#include "pico/stdlib.h"            // Biblioteca padrão do Pico (GPIO, tempo, etc.)
#include "pico/cyw43_arch.h"        // Driver WiFi para Pico W
#include "wifi_conn.h"      // Funções personalizadas de conexão WiFi
#include "mqtt_comm.h"      // Funções personalizadas para MQTT
#include "xor_cipher.h"     // Funções de cifra XOR

#define WIFI_SSID "NOME-WIFI"  // Nome da rede WiFi
#define WIFI_PASSWORD "SENHA-WIFI" // Senha da rede WiFi

#define MOSQUITTO_USER "aluno"  // Usuário para autenticação MQTT
#define MOSQUITTO_PASSWORD "senha123" // Senha para autenticação MQTT

#define PORT 1883
#define IP "172.19.10.162"

#define DEBUG
#define SUBSCRIBER

int main() {
    // Inicializa todas as interfaces de I/O padrão (USB serial, etc.)
    stdio_init_all();
    sleep_ms(5000); // Aguarda 1 segundo para estabilizar a inicialização
    
    // Conecta à rede WiFi
    // Parâmetros: Nome da rede (SSID) e senha
    #ifdef DEBUG
    printf("Conectando ao WiFi...\n");
    #endif
    if (!connect_to_wifi(WIFI_SSID, WIFI_PASSWORD)) {
        while(true) {
            #ifdef DEBUG
            printf("Erro ao conectar ao WiFi!\n");
            #endif
            sleep_ms(1000); // Aguarda indefinidamente se a conexão falhar
        }
    }
    
    //connect_to_wifi("AP-ACCESS BLH", "Fin@ApointBlH");
    // Configura o cliente MQTT
    // Parâmetros: ID do cliente, IP do broker, usuário, senha
    #ifdef DEBUG
    printf("Configurando MQTT...\n");
    #endif
    if (mqtt_setup("bitdog1", IP, PORT, MOSQUITTO_USER, MOSQUITTO_PASSWORD)) {
        #ifdef DEBUG
        printf("MQTT configurado com sucesso!\n");
        #endif
    } else {
        while(true) {
            #ifdef DEBUG
            printf("Erro ao configurar MQTT!\n");
            #endif
            sleep_ms(1000); // Aguarda indefinidamente se a configuração falhar
        }
    }
    #ifdef DEBUG
    printf("MQTT configurado!\n");
    #endif

    #ifndef SUBSCRIBER
    const char *mensagem = "26.5";
    uint8_t criptografada[16];
    xor_encrypt((uint8_t *)mensagem, criptografada, strlen(mensagem), CIPHER_KEY);

    while (true) {
        #ifdef DEBUG
        printf("Publicando mensagem: %s\n", mensagem);
        #endif
    
        bool pub_success = mqtt_comm_publish("escola/sala1/temperatura", mensagem, strlen(mensagem));
        //bool pub_success = mqtt_comm_publish("escola/sala1/temperatura", criptografada, strlen(mensagem));
        #ifdef DEBUG
        if (pub_success) {
            printf("Mensagem publicada com sucesso!\n");
        } else {
            printf("Falha ao publicar mensagem!\n");
        }
        #endif
        sleep_ms(5000);
    }
    #else
    mqtt_comm_subscribe(
        "escola/sala1/temperatura",  // Tópico a ser assinado
        mqtt_on_request,                        // Callback de confirmação de assinatura
        mqtt_on_incoming_publish,                         // Callback de dados
        mqtt_on_message            // Callback para mensagens recebidas
    );
    while (true) {
        // Aguarda novas mensagens MQTT
        if (mqtt_has_new_data()) {
            float valor = mqtt_get_last_value();
            unsigned long int timestamp = mqtt_get_last_timestamp();
            const char *topico = mqtt_get_last_topic();
            #ifdef DEBUG
            printf("----------------\n");
            printf("Novo tópico recebido: %s\n", topico);
            printf("Novo valor recebido: %.2f (timestamp: %lu)\n", valor, timestamp);
            printf("----------------\n");
            #endif
        }
        sleep_ms(1000); // Aguarda 1 segundo antes de verificar novamente
    }
    #endif
    return 0;
}

/* 
 * Comandos de terminal para testar o MQTT:
 * 
 * Inicia o broker MQTT com logs detalhados:
 * mosquitto -c mosquitto.conf -v
 * 
 * Assina o tópico de temperatura (recebe mensagens):
 * mosquitto_sub -h localhost -p 1883 -t "escola/sala1/temperatura" -u "aluno" -P "senha123"
 * 
 * Publica mensagem de teste no tópico de temperatura:
 * mosquitto_pub -h localhost -p 1883 -t "escola/sala1/temperatura" -u "aluno" -P "senha123" -m "26.6"
 */