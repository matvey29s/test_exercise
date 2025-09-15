/* main.c */
#include "uart_protocol.h"
#include "stm32f4xx.h"


int main(void)
{
    // Инициализация систем
    SystemCoreClockUpdate();
    
    // Инициализация UART
    uart_init();
    
    while (1) {
        // Вся обработка происходит в прерываниях
    }
}