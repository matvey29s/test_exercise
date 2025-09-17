#ifndef UART_PROTOCOL_H
#define UART_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

// Конфигурация протокола
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

// Callback-функции для аппаратного уровня
typedef void (*uart_send_byte_t)(uint8_t data);
typedef void (*uart_enable_tx_interrupt_t)(void);
typedef bool (*uart_is_tx_busy_t)(void);

// Инициализация протокола
void uart_protocol_init(uart_send_byte_t send_cb, 
                       uart_enable_tx_interrupt_t enable_tx_cb,
                       uart_is_tx_busy_t is_tx_busy_cb);

// Обработка принятого байта
void uart_protocol_process_byte(uint8_t data);

// Получить данные для отправки (используется аппаратным уровнем)
bool uart_protocol_get_tx_byte(uint8_t *data);

// Уведомление об окончании передачи
void uart_protocol_tx_complete(void);

#endif /* UART_PROTOCOL_H */