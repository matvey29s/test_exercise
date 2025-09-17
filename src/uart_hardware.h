#ifndef UART_HARDWARE_H
#define UART_HARDWARE_H

#include <stdint.h>
#include <stdbool.h>

// Инициализация аппаратного UART
void uart_hardware_init(void);

// Обработчик прерывания UART (должен быть объявлен в коде прерываний)
void USART2_IRQHandler(void);

#endif /* UART_HARDWARE_H */