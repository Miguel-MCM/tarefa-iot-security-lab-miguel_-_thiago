#ifndef XOR_CIPHER_H
#define XOR_CIPHER_H
#include <stdint.h>
#include <stddef.h>
#define CIPHER_KEY 42  // Chave de 1 byte para cifra XOR (0-255)
void xor_encrypt(const uint8_t *input, uint8_t *output, size_t len, uint8_t key);
#endif