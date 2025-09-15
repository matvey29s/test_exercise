#include "mocks.h"
#include <string.h>

// Mock variables
uint8_t mock_transmitted_data[256] = {0};
uint16_t mock_tx_count = 0;
bool mock_tx_enabled = false;
bool mock_rx_enabled = false;
uint32_t mock_error_flags = 0;

// Track callback calls
static uint8_t callback_address = 0;
static bool callback_called = false;

// Mock implementation
void mock_ll_usart_transmit_data8(uint8_t data) {
    if (mock_tx_count < 256) {
        mock_transmitted_data[mock_tx_count++] = data;
    }
}

void mock_ll_usart_enable_it_txe(void) {
    mock_tx_enabled = true;
}

void mock_ll_usart_disable_it_txe(void) {
    mock_tx_enabled = false;
}

void mock_ll_usart_clear_flag_ore(void) {
    mock_error_flags &= ~(1 << 0);
}

void mock_ll_usart_clear_flag_fe(void) {
    mock_error_flags &= ~(1 << 1);
}

void mock_ll_usart_clear_flag_ne(void) {
    mock_error_flags &= ~(1 << 2);
}

void mock_ll_usart_clear_flag_pe(void) {
    mock_error_flags &= ~(1 << 3);
}

void mock_reset_all(void) {
    memset(mock_transmitted_data, 0, sizeof(mock_transmitted_data));
    mock_tx_count = 0;
    mock_tx_enabled = false;
    mock_rx_enabled = false;
    mock_error_flags = 0;
    callback_address = 0;
    callback_called = false;
}

void mock_assert_tx_data(const uint8_t* expected, uint16_t length) {
    for (uint16_t i = 0; i < length; i++) {
        if (mock_transmitted_data[i] != expected[i]) {
            printf("TX data mismatch at index %d: expected 0x%02X, got 0x%02X\n", 
                   i, expected[i], mock_transmitted_data[i]);
            // Здесь должен быть assert или тест фреймворк
        }
    }
}

void mock_assert_tx_enabled(bool expected) {
    if (mock_tx_enabled != expected) {
        printf("TX enabled mismatch: expected %d, got %d\n", expected, mock_tx_enabled);
    }
}

// Callback function for tests
void on_valid_request(uint8_t address) {
    callback_called = true;
    callback_address = address;
}

void mock_assert_callback_called_with_address(uint8_t expected_address) {
    if (!callback_called) {
        printf("Callback was not called\n");
        return;
    }
    if (callback_address != expected_address) {
        printf("Callback address mismatch: expected 0x%02X, got 0x%02X\n", 
               expected_address, callback_address);
    }
}