#include "uart_protocol.h"

// Внутренние переменные
static uint8_t tx_buffer[TX_BUFFER_SIZE];
static volatile uint16_t tx_head = 0;
static volatile uint16_t tx_tail = 0;
static volatile uint16_t tx_count = 0;

// Состояние парсера
static struct {
    uint8_t state;
    request_frame_t request;
    uint8_t crc_calculated;
    uint8_t bytes_received;
} parser_state;

// Callback-функции для аппаратного уровня
static uart_send_byte_t hardware_send_byte = NULL;
static uart_enable_tx_interrupt_t hardware_enable_tx_interrupt = NULL;
static uart_is_tx_busy_t hardware_is_tx_busy = NULL;

// Расчет контрольной суммы CRC8
static uint8_t calculate_crc8(uint8_t data, uint8_t crc)
{
    uint8_t i = data ^ crc;
    crc = 0;
    
    if(i & 0x01) crc ^= 0x5E;
    if(i & 0x02) crc ^= 0xBC;
    if(i & 0x04) crc ^= 0x61;
    if(i & 0x08) crc ^= 0xC2;
    if(i & 0x10) crc ^= 0x9D;
    if(i & 0x20) crc ^= 0x23;
    if(i & 0x40) crc ^= 0x46;
    if(i & 0x80) crc ^= 0x8C;
    
    return crc;
}

// Добавление байта в буфер передачи
static void tx_buffer_put(uint8_t data)
{
    if (tx_count < TX_BUFFER_SIZE) {
        tx_buffer[tx_head] = data;
        tx_head = (tx_head + 1) % TX_BUFFER_SIZE;
        tx_count++;
    }
}

// Запуск процесса передачи данных
static void start_transmission(void)
{
    if (tx_count > 0 && hardware_send_byte && 
        hardware_enable_tx_interrupt && !hardware_is_tx_busy()) {
        hardware_enable_tx_interrupt();
    }
}

// Формирование и отправка ответа
static void send_response_internal(uint8_t address, int8_t temperature, 
                                  uint16_t level, uint16_t frequency)
{
    response_frame_t response;
    
    response.header = RESPONSE_HEADER;
    response.address = address;
    response.opcode = OPCODE_RESPONSE;
    response.temperature = temperature;
    response.level = level;
    response.frequency = frequency;
    
    // Расчет CRC
    uint8_t crc = 0;
    uint8_t *data = (uint8_t*)&response;
    
    for (uint8_t i = 0; i < sizeof(response_frame_t) - 1; i++) {
        crc = calculate_crc8(data[i], crc);
    }
    response.crc = crc;
    
    // Добавление в буфер передачи
    for (uint8_t i = 0; i < sizeof(response_frame_t); i++) {
        tx_buffer_put(data[i]);
    }
    
    start_transmission();
}

// Инициализация протокола
void uart_protocol_init(uart_send_byte_t send_cb, 
                       uart_enable_tx_interrupt_t enable_tx_cb,
                       uart_is_tx_busy_t is_tx_busy_cb)
{
    hardware_send_byte = send_cb;
    hardware_enable_tx_interrupt = enable_tx_cb;
    hardware_is_tx_busy = is_tx_busy_cb;
    
    parser_state.state = 0;
    parser_state.bytes_received = 0;
}

// Обработка принятого байта
void uart_protocol_process_byte(uint8_t data)
{
    switch (parser_state.state) {
        case 0: // Ожидание заголовка
            if (data == REQUEST_HEADER) {
                parser_state.request.header = data;
                parser_state.crc_calculated = calculate_crc8(data, 0);
                parser_state.bytes_received = 1;
                parser_state.state = 1;
            }
            break;
            
        case 1: // Ожидание адреса
            parser_state.request.address = data;
            parser_state.crc_calculated = calculate_crc8(data, parser_state.crc_calculated);
            parser_state.bytes_received++;
            parser_state.state = 2;
            break;
            
        case 2: // Ожидание кода операции
            parser_state.request.opcode = data;
            parser_state.crc_calculated = calculate_crc8(data, parser_state.crc_calculated);
            parser_state.bytes_received++;
            parser_state.state = 3;
            break;
            
        case 3: // Ожидание CRC
            parser_state.request.crc = data;
            parser_state.bytes_received++;
            
            if (parser_state.bytes_received == sizeof(request_frame_t) && 
                parser_state.request.opcode == OPCODE_REQUEST && 
                parser_state.crc_calculated == parser_state.request.crc) {
                
                // TODO: Реализовать прием данных с датчиков
                int8_t temperature = 50;
                uint16_t level = 0x1000;
                uint16_t frequency = 0x5000;
                
                send_response_internal(parser_state.request.address, 
                                     temperature, level, frequency);
            }
            
            parser_state.state = 0;
            parser_state.bytes_received = 0;
            break;
    }
}

// Получить данные для отправки
bool uart_protocol_get_tx_byte(uint8_t *data)
{
    if (tx_count > 0) {
        *data = tx_buffer[tx_tail];
        tx_tail = (tx_tail + 1) % TX_BUFFER_SIZE;
        tx_count--;
        return true;
    }
    return false;
}

// Уведомление об окончании передачи
void uart_protocol_tx_complete(void)
{
    // Если есть еще данные для передачи, продолжаем
    if (tx_count > 0 && hardware_enable_tx_interrupt) {
        hardware_enable_tx_interrupt();
    }
}