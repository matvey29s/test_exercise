#include "uart_hardware.h"
#include "uart_protocol.h"
#include "stm32f4xx_ll_usart.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_rcc.h"

#define UART_BAUDRATE 19200

static volatile bool tx_busy = false;

// Callback-функции для протокола
static void hardware_send_byte(uint8_t data)
{
    LL_USART_TransmitData8(USART2, data);
}

static void hardware_enable_tx_interrupt(void)
{
    LL_USART_EnableIT_TXE(USART2);
}

static bool hardware_is_tx_busy(void)
{
    return tx_busy;
}

// Инициализация аппаратного UART
void uart_hardware_init(void)
{
    // Включение тактирования
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
    
    // Настройка выводов GPIO
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LL_GPIO_PIN_2 | LL_GPIO_PIN_3;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // Настройка параметров UART
    LL_USART_InitTypeDef USART_InitStruct = {0};
    USART_InitStruct.BaudRate = UART_BAUDRATE;
    USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
    USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
    USART_InitStruct.Parity = LL_USART_PARITY_NONE;
    USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
    USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
    LL_USART_Init(USART2, &USART_InitStruct);
    
    // Включение прерываний
    LL_USART_EnableIT_RXNE(USART2);
    LL_USART_EnableIT_ERROR(USART2);
    
    // Настройка NVIC
    NVIC_SetPriority(USART2_IRQn, 0);
    NVIC_EnableIRQ(USART2_IRQn);
    
    // Инициализация протокола с callback-функциями
    uart_protocol_init(hardware_send_byte, 
                      hardware_enable_tx_interrupt, 
                      hardware_is_tx_busy);
    
    // Включение UART
    LL_USART_Enable(USART2);
}

// Обработчик прерывания UART
void USART2_IRQHandler(void)
{
    // Обработка приема данных
    if (LL_USART_IsActiveFlag_RXNE(USART2) && LL_USART_IsEnabledIT_RXNE(USART2)) {
        uint8_t data = LL_USART_ReceiveData8(USART2);
        uart_protocol_process_byte(data);
    }
    
    // Обработка передачи данных
    if (LL_USART_IsActiveFlag_TXE(USART2) && LL_USART_IsEnabledIT_TXE(USART2)) {
        uint8_t data;
        if (uart_protocol_get_tx_byte(&data)) {
            LL_USART_TransmitData8(USART2, data);
            tx_busy = true;
        } else {
            LL_USART_DisableIT_TXE(USART2);
            tx_busy = false;
            uart_protocol_tx_complete();
        }
    }
    
    // Обработка ошибок
    if (LL_USART_IsActiveFlag_ORE(USART2) || LL_USART_IsActiveFlag_FE(USART2) || 
        LL_USART_IsActiveFlag_NE(USART2) || LL_USART_IsActiveFlag_PE(USART2)) {
        
        LL_USART_ClearFlag_ORE(USART2);
        LL_USART_ClearFlag_FE(USART2);
        LL_USART_ClearFlag_NE(USART2);
        LL_USART_ClearFlag_PE(USART2);
        
        // Можно добавить функцию сброса состояния парсера при необходимости
    }
}