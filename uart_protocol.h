/* uart_protocol.h */
#ifndef UART_PROTOCOL_H
#define UART_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

// Конфигурация протокола
#define UART_BAUDRATE 19200
#define RX_BUFFER_SIZE 256
#define TX_BUFFER_SIZE 256

// Коды операций
#define OPCODE_REQUEST 0x06
#define OPCODE_RESPONSE 0x06

// Заголовки пакетов
#define REQUEST_HEADER 0x31
#define RESPONSE_HEADER 0x3E

// Структура запроса
typedef struct {
    uint8_t header;
    uint8_t address;
    uint8_t opcode;
    uint8_t crc;
} __attribute__((packed)) request_frame_t;

// Структура ответа
typedef struct {
    uint8_t header;
    uint8_t address;
    uint8_t opcode;
    int8_t temperature;
    uint16_t level;
    uint16_t frequency;
    uint8_t crc;
} __attribute__((packed)) response_frame_t;

// Инициализация UART
void uart_init(void);

// Callback для обработки валидного запроса
extern void on_valid_request(uint8_t address);

#endif /* UART_PROTOCOL_H */