#ifndef MOCKS_H
#define MOCKS_H

#include <stdint.h>
#include <stdbool.h>

// Mock variables для отслеживания вызовов
extern uint8_t mock_transmitted_data[256];
extern uint16_t mock_tx_count;
extern bool mock_tx_enabled;
extern bool mock_rx_enabled;
extern uint32_t mock_error_flags;

// Mock functions для замены LL функций
void mock_ll_usart_transmit_data8(uint8_t data);
void mock_ll_usart_enable_it_txe(void);
void mock_ll_usart_disable_it_txe(void);
void mock_ll_usart_clear_flag_ore(void);
void mock_ll_usart_clear_flag_fe(void);
void mock_ll_usart_clear_flag_ne(void);
void mock_ll_usart_clear_flag_pe(void);

// Сброс всех mock переменных
void mock_reset_all(void);

// Assert functions
void mock_assert_tx_data(const uint8_t* expected, uint16_t length);
void mock_assert_tx_enabled(bool expected);
void mock_assert_callback_called_with_address(uint8_t expected_address);

#endif /* MOCKS_H */