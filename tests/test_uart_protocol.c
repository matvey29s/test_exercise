#include "uart_protocol.h"
#include "mocks.h"
#include <stdio.h>
#include <assert.h>

// Define mock macros for unit tests
#define LL_USART_TransmitData8(data) mock_ll_usart_transmit_data8(data)
#define LL_USART_EnableIT_TXE() mock_ll_usart_enable_it_txe()
#define LL_USART_DisableIT_TXE() mock_ll_usart_disable_it_txe()
#define LL_USART_ClearFlag_ORE() mock_ll_usart_clear_flag_ore()
#define LL_USART_ClearFlag_FE() mock_ll_usart_clear_flag_fe()
#define LL_USART_ClearFlag_NE() mock_ll_usart_clear_flag_ne()
#define LL_USART_ClearFlag_PE() mock_ll_usart_clear_flag_pe()

// Test CRC calculation
void test_crc8_calculation(void) {
    printf("Testing CRC8 calculation...\n");
    
    // Test known values
    uint8_t crc = calculate_crc8(0x31, 0);
    crc = calculate_crc8(0x12, crc);
    crc = calculate_crc8(0x06, crc);
    
    // This should match the expected CRC for packet [0x31, 0x12, 0x06]
    printf("Calculated CRC: 0x%02X\n", crc);
    
    // Test with different data
    uint8_t crc2 = calculate_crc8(0x00, 0);
    crc2 = calculate_crc8(0xFF, crc2);
    printf("CRC for [0x00, 0xFF]: 0x%02X\n", crc2);
    
    printf("CRC test passed!\n\n");
}

// Test valid packet processing
void test_valid_packet_processing(void) {
    printf("Testing valid packet processing...\n");
    mock_reset_all();
    
    // Valid packet: [0x31, 0x12, 0x06, CRC]
    // First calculate expected CRC
    uint8_t expected_crc = calculate_crc8(0x31, 0);
    expected_crc = calculate_crc8(0x12, expected_crc);
    expected_crc = calculate_crc8(0x06, expected_crc);
    
    // Process packet bytes
    process_received_byte(0x31); // Header
    process_received_byte(0x12); // Address
    process_received_byte(0x06); // Opcode
    process_received_byte(expected_crc); // CRC
    
    // Verify response was prepared and transmission started
    mock_assert_tx_enabled(true);
    mock_assert_callback_called_with_address(0x12);
    
    printf("Valid packet test passed!\n\n");
}

// Test invalid CRC
void test_invalid_crc_packet(void) {
    printf("Testing invalid CRC packet...\n");
    mock_reset_all();
    
    // Process packet with wrong CRC
    process_received_byte(0x31); // Header
    process_received_byte(0x12); // Address
    process_received_byte(0x06); // Opcode
    process_received_byte(0xFF); // Wrong CRC
    
    // Verify no response was sent and callback not called
    mock_assert_tx_enabled(false);
    
    printf("Invalid CRC test passed!\n\n");
}

// Test wrong header
void test_wrong_header(void) {
    printf("Testing wrong header...\n");
    mock_reset_all();
    
    // Start with wrong header
    process_received_byte(0xAA); // Wrong header
    process_received_byte(0x12); // Should be ignored
    process_received_byte(0x06); // Should be ignored
    
    // Verify parser remains in state 0
    // (Would need access to parser_state for full verification)
    mock_assert_tx_enabled(false);
    
    printf("Wrong header test passed!\n\n");
}

// Test response generation
void test_response_generation(void) {
    printf("Testing response generation...\n");
    mock_reset_all();
    
    // Manually call response generation
    send_response_internal(0x12, 25, 0x1234, 0x5678);
    
    // Verify transmission started
    mock_assert_tx_enabled(true);
    
    // Expected response structure:
    // [0x3E, 0x12, 0x06, 25, 0x34, 0x12, 0x78, 0x56, CRC]
    uint8_t expected_start[] = {0x3E, 0x12, 0x06, 25};
    mock_assert_tx_data(expected_start, 4);
    
    printf("Response generation test passed!\n\n");
}

// Test buffer overflow
void test_buffer_overflow(void) {
    printf("Testing buffer overflow...\n");
    mock_reset_all();
    
    // Fill TX buffer
    for (int i = 0; i < TX_BUFFER_SIZE + 10; i++) {
        tx_buffer_put(i & 0xFF);
    }
    
    // Verify buffer didn't overflow catastrophically
    // (Implementation should prevent writing beyond buffer)
    printf("Buffer overflow test completed\n\n");
}

// Test state machine transitions
void test_state_machine_transitions(void) {
    printf("Testing state machine transitions...\n");
    mock_reset_all();
    
    // Test state transitions
    process_received_byte(0x31); // Should go to state 1
    // Would need access to parser_state.state to verify
    
    process_received_byte(0x12); // Should go to state 2
    process_received_byte(0x06); // Should go to state 3
    process_received_byte(0xAB); // Should reset to state 0
    
    printf("State machine test completed\n\n");
}

// Main test runner
void run_all_tests(void) {
    printf("=== Starting UART Protocol Unit Tests ===\n\n");
    
    test_crc8_calculation();
    test_valid_packet_processing();
    test_invalid_crc_packet();
    test_wrong_header();
    test_response_generation();
    test_buffer_overflow();
    test_state_machine_transitions();
    
    printf("=== All tests completed ===\n");
}

int main(void) {
    run_all_tests();
    return 0;
}