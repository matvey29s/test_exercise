#ifndef UART_HARDWARE_H
#define UART_HARDWARE_H

#include <stdint.h>
#include <stdbool.h>

// Конфигурация аппаратного UART
#define UART_PERIPHERAL USART2
#define UART_IRQn USART2_IRQn
#define UART_IRQHandler USART2_IRQHandler
#define UART_BAUDRATE 19200

// GPIO конфигурация
// TODO написать необходимые номера портов для UART
#define UART_GPIO_PORT GPIOA
#define UART_TX_PIN LL_GPIO_PIN_2
#define UART_RX_PIN LL_GPIO_PIN_3
#define UART_GPIO_AF LL_GPIO_AF_7

// Тактирование
#define UART_CLOCK_ENABLE() LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2)
#define UART_GPIO_CLOCK_ENABLE() LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA)

// Инициализация аппаратного UART
void uart_hardware_init(void);

// Обработчик прерывания UART
void UART_IRQHandler(void);

#endif /* UART_HARDWARE_H */