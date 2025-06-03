#include "wifi_conn.h"         // Cabeçalho com a declaração da função de conexão Wi-Fi
#include "pico/cyw43_arch.h"           // Biblioteca para controle do chip Wi-Fi CYW43 no Raspberry Pi Pico W
#include <stdio.h>                     // Biblioteca padrão de entrada/saída (para usar printf)

/**
 * Função: connect_to_wifi
 * Objetivo: Inicializar o chip Wi-Fi da Pico W e conectar a uma rede usando SSID e senha fornecidos.
 */
bool connect_to_wifi(const char *ssid, const char *password) {
    // Inicializa o driver Wi-Fi (CYW43). Retorna 0 se for bem-sucedido.
    if (cyw43_arch_init() != 0) {
        #ifdef DEBUG_WIFI
        printf("Erro ao iniciar Wi-Fi\n");
        #endif
        return false;
    }

    // Habilita o modo estação (STA) para se conectar a um ponto de acesso.
    cyw43_arch_enable_sta_mode();

    // Tenta conectar à rede Wi-Fi com um tempo limite de 30 segundos (30000 ms).
    // Utiliza autenticação WPA2 com criptografia AES.
    bool connect_success = cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 30000) == 0;
    #ifdef DEBUG_WIFI
    if (connect_success) {
        printf("Conectado ao Wi-Fi\n");  // Se conectar com sucesso, exibe confirmação.
    } else {
        printf("Erro ao conectar\n");  // Se falhar, imprime mensagem de erro.
    }
    #endif
    return connect_success;  // Retorna o resultado da tentativa de conexão.
}
