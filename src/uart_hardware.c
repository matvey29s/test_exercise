#include "uart_hardware.h"
#include "uart_protocol.h"
#include "stm32g0xx_ll_usart.h"
#include "stm32g0xx_ll_gpio.h"
#include "stm32g0xx_ll_bus.h"
#include "stm32g0xx_ll_rcc.h"


static volatile bool tx_busy = false;

// Callback-функции для протокола
static void hardware_send_byte(uint8_t data)
{
    LL_USART_TransmitData8(UART_PERIPHERAL, *(uint8_t*) data);
}

static void hardware_enable_tx_interrupt(void)
{
    LL_USART_EnableIT_TXE(UART_PERIPHERAL);
}

static bool hardware_is_tx_busy(void)
{
    return tx_busy;
}

// Инициализация аппаратного UART
void uart_hardware_init(void)
{
    // Включение тактирования
    UART_CLOCK_ENABLE();
    UART_GPIO_CLOCK_ENABLE();
    
    // Настройка выводов GPIO
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = UART_TX_PIN | UART_RX_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Alternate = UART_GPIO_AF;
    LL_GPIO_Init(UART_GPIO_PORT, &GPIO_InitStruct);
    
    // Настройка параметров UART
    LL_USART_InitTypeDef USART_InitStruct = {0};
    USART_InitStruct.BaudRate = UART_BAUDRATE;
    USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
    USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
    USART_InitStruct.Parity = LL_USART_PARITY_NONE;
    USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
    USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
    LL_USART_Init(UART_PERIPHERAL, &USART_InitStruct);
    
    // Включение прерываний
    LL_USART_EnableIT_RXNE(UART_PERIPHERAL);
    LL_USART_EnableIT_ERROR(UART_PERIPHERAL);
    
    // Настройка NVIC
    NVIC_SetPriority(UART_IRQn, 0);
    NVIC_EnableIRQ(UART_IRQn);
    
    // Инициализация протокола с callback-функциями
    uart_protocol_init(hardware_send_byte, 
                      hardware_enable_tx_interrupt, 
                      hardware_is_tx_busy);
    
    // Включение UART
    LL_USART_Enable(UART_PERIPHERAL);
}

// Обработчик прерывания UART
void UART_IRQHandler(void)
{
    // Обработка приема данных
    if (LL_USART_IsActiveFlag_RXNE(UART_PERIPHERAL) && LL_USART_IsEnabledIT_RXNE(UART_PERIPHERAL)) {
        uint8_t data = LL_USART_ReceiveData8(UART_PERIPHERAL);
        uart_protocol_process_byte(data);
    }
    
    // Обработка передачи данных
    if (LL_USART_IsActiveFlag_TXE(UART_PERIPHERAL) && LL_USART_IsEnabledIT_TXE(UART_PERIPHERAL)) {
        uint8_t data;
        if (uart_protocol_get_tx_byte(&data)) {
            LL_USART_TransmitData8(UART_PERIPHERAL, data);
            tx_busy = true;
        } else {
            LL_USART_DisableIT_TXE(UART_PERIPHERAL);
            tx_busy = false;
            uart_protocol_tx_complete();
        }
    }
    
    // Обработка ошибок
    if (LL_USART_IsActiveFlag_ORE(UART_PERIPHERAL) || LL_USART_IsActiveFlag_FE(UART_PERIPHERAL) || 
        LL_USART_IsActiveFlag_NE(UART_PERIPHERAL) || LL_USART_IsActiveFlag_PE(UART_PERIPHERAL)) {
        
        LL_USART_ClearFlag_ORE(UART_PERIPHERAL);
        LL_USART_ClearFlag_FE(UART_PERIPHERAL);
        LL_USART_ClearFlag_NE(UART_PERIPHERAL);
        LL_USART_ClearFlag_PE(UART_PERIPHERAL);
    }
}