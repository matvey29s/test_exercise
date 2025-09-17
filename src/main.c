#include "uart_protocol.h"
#include "uart_hardware.h"

//Остальная инициализация

int main(void)
{
    //Остальная инициализация
    // Инициализация аппаратного UART
    uart_hardware_init();
    
    while (1) {
        // Основной цикл - обработка в прерываниях
    }
}