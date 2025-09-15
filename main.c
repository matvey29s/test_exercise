/* main.c */
#include "uart_protocol.h"
#include "stm32f4xx.h"

// Callback функция для уведомления о валидном запросе
void on_valid_request(uint8_t address)
{
    // Здесь можно выполнить дополнительные действия
    // Например, обновить статус или логировать запрос
}

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