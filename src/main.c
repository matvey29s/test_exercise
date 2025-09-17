#include "uart_protocol.h"
#include "uart_hw.c"

int main(void)
{
    // Инициализация аппаратного UART
    uart_hardware_init();
    
    while (1) {
        // Основной цикл - обработка в прерываниях
    }
}