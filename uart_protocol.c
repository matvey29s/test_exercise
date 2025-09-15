/* uart_protocol.c */
#include "uart_protocol.h"
#include "stm32f4xx_ll_usart.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_rcc.h"

// Внутренние переменные
// Буфер для приема данных (кольцевой буфер)
static volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
static volatile uint16_t rx_head = 0;     // Указатель на запись
static volatile uint16_t rx_tail = 0;     // Указатель на чтение
static volatile uint16_t rx_count = 0;    // Количество байт в буфере

// Буфер для передачи данных (кольцевой буфер)
static uint8_t tx_buffer[TX_BUFFER_SIZE];
static volatile uint16_t tx_head = 0;     // Указатель на запись
static volatile uint16_t tx_tail = 0;     // Указатель на чтение
static volatile uint16_t tx_count = 0;    // Количество байт в буфере
static volatile bool tx_busy = false;     // Флаг занятости передачи

// Состояние парсера - отслеживает текущее состояние разбора принятых данных
static struct {
    uint8_t state;              // Текущее состояние парсера
    request_frame_t request;    // Структура для хранения разобранного запроса
    uint8_t crc_calculated;     // Рассчитанное значение CRC для проверки
    uint8_t bytes_received;     // Количество принятых байт текущего пакета
} parser_state;

// Расчет контрольной суммы CRC8 по заданному алгоритму
// data - новый байт для расчета, crc - текущее значение CRC
static uint8_t calculate_crc8(uint8_t data, uint8_t crc)
{
    uint8_t i = data ^ crc;  // XOR нового байта с текущим CRC
    crc = 0;  // Сброс CRC для расчета нового значения
    
    // Побитовый расчет CRC согласно заданному полиному
    if(i & 0x01) crc ^= 0x5E;  // Если установлен бит 0
    if(i & 0x02) crc ^= 0xBC;  // Если установлен бит 1
    if(i & 0x04) crc ^= 0x61;  // Если установлен бит 2
    if(i & 0x08) crc ^= 0xC2;  // Если установлен бит 3
    if(i & 0x10) crc ^= 0x9D;  // Если установлен бит 4
    if(i & 0x20) crc ^= 0x23;  // Если установлен бит 5
    if(i & 0x40) crc ^= 0x46;  // Если установлен бит 6
    if(i & 0x80) crc ^= 0x8C;  // Если установлен бит 7
    
    return crc;
}

// Добавление байта в буфер передачи (постановка в очередь)
static void tx_buffer_put(uint8_t data)
{
    if (tx_count < TX_BUFFER_SIZE) {  // Если есть место в буфере
        tx_buffer[tx_head] = data;    // Записываем данные
        tx_head = (tx_head + 1) % TX_BUFFER_SIZE;  // Увеличиваем указатель с закольцовыванием
        tx_count++;  // Увеличиваем счетчик байт в буфере
    }
}

// Запуск процесса передачи данных
static void start_transmission(void)
{
    if (tx_count > 0 && !tx_busy) {  // Если есть что передавать и передача не занята
        tx_busy = true;  // Устанавливаем флаг занятости
        LL_USART_EnableIT_TXE(USART2);  // Включаем прерывание по готовности к передаче
        // Это вызовет немедленное прерывание, если регистр передачи пуст
    }
}

// Внутренняя функция формирования и отправки ответа
static void send_response_internal(uint8_t address, int8_t temperature, uint16_t level, uint16_t frequency)
{
    response_frame_t response;  // Создаем структуру ответа
    
    // Заполняем поля ответа
    response.header = RESPONSE_HEADER;      // Заголовок ответа (3Eh)
    response.address = address;             // Адрес отправителя
    response.opcode = OPCODE_RESPONSE;      // Код операции (06h)
    response.temperature = temperature;     // Температура
    response.level = level;                 // Уровень (2 байта)
    response.frequency = frequency;         // Частота (2 байта)
    
    // Расчет контрольной суммы для ответа
    uint8_t crc = 0;  // Начальное значение CRC
    uint8_t *data = (uint8_t*)&response;  // Указатель на данные ответа
    
    // Расчет CRC для всех полей КРОМЕ самого CRC (sizeof-1)
    for (uint8_t i = 0; i < sizeof(response_frame_t) - 1; i++) {
        crc = calculate_crc8(data[i], crc);  // Последовательно добавляем каждый байт
    }
    response.crc = crc;  // Записываем рассчитанный CRC в структуру
    
    // Добавление всего ответа в буфер передачи
    for (uint8_t i = 0; i < sizeof(response_frame_t); i++) {
        tx_buffer_put(data[i]);  // Постановка каждого байта в очередь на передачу
    }
    
    // Запуск передачи
    start_transmission();
}

// Обработка принятого байта - автомат состояний для разбора протокола
static void process_received_byte(uint8_t data)
{
    switch (parser_state.state) {
        case 0: // Состояние 0: Ожидание заголовка пакета
            if (data == REQUEST_HEADER) {  // Если получен корректный заголовок (31h)
                parser_state.request.header = data;  // Сохраняем заголовок
                parser_state.crc_calculated = calculate_crc8(data, 0);  // Начинаем расчет CRC
                parser_state.bytes_received = 1;     // Первый байт принят
                parser_state.state = 1;              // Переходим к следующему состоянию
            }
            // Если заголовок не совпадает - байт игнорируется, остаемся в состоянии 0
            break;
            
        case 1: // Состояние 1: Ожидание байта адреса
            parser_state.request.address = data;  // Сохраняем адрес
            parser_state.crc_calculated = calculate_crc8(data, parser_state.crc_calculated);  // Продолжаем CRC
            parser_state.bytes_received++;        // Увеличиваем счетчик байт
            parser_state.state = 2;               // Переходим к следующему состоянию
            break;
            
        case 2: // Состояние 2: Ожидание кода операции
            parser_state.request.opcode = data;   // Сохраняем код операции
            parser_state.crc_calculated = calculate_crc8(data, parser_state.crc_calculated);  // Продолжаем CRC
            parser_state.bytes_received++;        // Увеличиваем счетчик байт
            parser_state.state = 3;               // Переходим к следующему состоянию
            break;
            
        case 3: // Состояние 3: Ожидание контрольной суммы
            parser_state.request.crc = data;      // Сохраняем принятую CRC
            parser_state.bytes_received++;        // Увеличиваем счетчик байт
            
            // ПРОВЕРКА ВАЛИДНОСТИ ПАКЕТА:
            // 1. Принято правильное количество байт
            // 2. Код операции соответствует ожидаемому (06h)
            // 3. Рассчитанная CRC совпадает с принятой
            if (parser_state.bytes_received == sizeof(request_frame_t) && 
                parser_state.request.opcode == OPCODE_REQUEST && 
                parser_state.crc_calculated == parser_state.request.crc) {
                
                // Пакет корректен - формируем и отправляем ответ

                int8_t temperature = 50;        // Пример: температура 50°C
                uint16_t level = 0x1000;        // Пример: уровень 1000
                uint16_t frequency = 0x5000;    // Пример: частота 5кГц
                
                // Отправка ответа
                send_response_internal(parser_state.request.address, temperature, level, frequency);
                
                // Уведомление о валидном запросе (callback функция)
                on_valid_request(parser_state.request.address);
            }
            // Если пакет невалиден - просто игнорируем его
            
            // Сброс состояния парсера для приема следующего пакета
            parser_state.state = 0;
            parser_state.bytes_received = 0;
            break;
    }
}

// Инициализация UART интерфейса
void uart_init(void)
{
    // Включение тактирования для USART2 и GPIOA
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
    
    // Настройка выводов GPIO для UART
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LL_GPIO_PIN_2 | LL_GPIO_PIN_3;  // PA2-TX, PA3-RX
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;        // Альтернативная функция
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;      // Высокая скорость
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL; // Push-Pull выход
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;               // Подтяжка к питанию
    GPIO_InitStruct.Alternate = LL_GPIO_AF_7;             // AF7 для USART2
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // Настройка параметров UART
    LL_USART_InitTypeDef USART_InitStruct = {0};
    USART_InitStruct.BaudRate = UART_BAUDRATE;           // 19200 бод
    USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;  // 8 бит данных
    USART_InitStruct.StopBits = LL_USART_STOPBITS_1;     // 1 стоп-бит
    USART_InitStruct.Parity = LL_USART_PARITY_NONE;      // Без четности
    USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX; // Прием и передача
    USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE; // Без аппаратного контроля
    USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16; // 16x oversampling
    LL_USART_Init(USART2, &USART_InitStruct);
    
    // Включение прерываний
    LL_USART_EnableIT_RXNE(USART2);   // Прерывание при приеме данных
    LL_USART_EnableIT_ERROR(USART2);  // Прерывание при ошибках
    
    // Настройка контроллера прерываний (NVIC)
    NVIC_SetPriority(USART2_IRQn, 0);    // Высокий приоритет
    NVIC_EnableIRQ(USART2_IRQn);         // Разрешение прерывания
    
    // Инициализация состояния парсера
    parser_state.state = 0;             // Начальное состояние
    parser_state.bytes_received = 0;    // Байт не принято
    
    // Включение UART
    LL_USART_Enable(USART2);
}

// Обработчик прерывания UART - вызывается при событиях на UART
void USART2_IRQHandler(void)
{
    // Обработка приема данных: флаг "Receive Register Not Empty"
    if (LL_USART_IsActiveFlag_RXNE(USART2) && LL_USART_IsEnabledIT_RXNE(USART2)) {
        uint8_t data = LL_USART_ReceiveData8(USART2);  // Чтение принятого байта
        
        // Немедленная обработка байта (разбор протокола)
        process_received_byte(data);
    }
    
    // Обработка передачи данных: флаг "Transmit Register Empty"
    if (LL_USART_IsActiveFlag_TXE(USART2) && LL_USART_IsEnabledIT_TXE(USART2)) {
        if (tx_count > 0) {  // Если есть данные для передачи
            // Передача следующего байта из буфера
            LL_USART_TransmitData8(USART2, tx_buffer[tx_tail]);
            tx_tail = (tx_tail + 1) % TX_BUFFER_SIZE;  // Увеличиваем указатель чтения
            tx_count--;  // Уменьшаем счетчик байт в буфере
        } else {
            // Буфер передачи пуст - отключаем прерывание передачи
            LL_USART_DisableIT_TXE(USART2);
            tx_busy = false;  // Сбрасываем флаг занятости передачи
        }
    }
    
    // Обработка ошибок UART
    if (LL_USART_IsActiveFlag_ORE(USART2) || LL_USART_IsActiveFlag_FE(USART2) || 
        LL_USART_IsActiveFlag_NE(USART2) || LL_USART_IsActiveFlag_PE(USART2)) {
        // Overrun Error - переполнение приемного буфера
        // Framing Error - ошибка стоп-бита
        // Noise Error - шумовая ошибка
        // Parity Error - ошибка четности
        
        // Сброс флагов ошибок
        LL_USART_ClearFlag_ORE(USART2);
        LL_USART_ClearFlag_FE(USART2);
        LL_USART_ClearFlag_NE(USART2);
        LL_USART_ClearFlag_PE(USART2);
        
        // Сброс состояния парсера при ошибке (начало поиска нового пакета)
        parser_state.state = 0;
        parser_state.bytes_received = 0;
    }
}