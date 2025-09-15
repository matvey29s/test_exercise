#include <stdio.h>

// Test function declarations
void test_crc8_calculation(void);
void test_valid_packet_processing(void);
void test_invalid_crc_packet(void);
void test_wrong_header(void);
void test_response_generation(void);
void test_buffer_overflow(void);
void test_state_machine_transitions(void);

int main(void) {
    printf("Unit Test Runner for UART Protocol\n");
    printf("==================================\n\n");
    
    // Run all tests
    test_crc8_calculation();
    test_valid_packet_processing();
    test_invalid_crc_packet();
    test_wrong_header();
    test_response_generation();
    test_buffer_overflow();
    test_state_machine_transitions();
    
    printf("\nAll tests completed successfully!\n");
    return 0;
}