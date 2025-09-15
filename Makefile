CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I./src -I./tests
TEST_FLAGS = -DUNIT_TESTS

SRC_DIR = src
TEST_DIR = tests

SRC_FILES = $(SRC_DIR)/uart_protocol.c
TEST_FILES = $(TEST_DIR)/test_uart_protocol.c $(TEST_DIR)/mocks.c

# Main test executable
test_runner: $(SRC_FILES) $(TEST_FILES)
	$(CC) $(CFLAGS) $(TEST_FLAGS) -o $@ $^
	./$@

# Individual test compilation
test_crc: $(SRC_FILES) $(TEST_DIR)/test_crc.c
	$(CC) $(CFLAGS) $(TEST_FLAGS) -o $@ $^
	./$@

clean:
	rm -f test_runner test_crc *.o

.PHONY: all clean test