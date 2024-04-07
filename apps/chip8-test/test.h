#include <stdbool.h>
#include <stdint.h>

#define VM_TEST_MAX_INPUTS 4

typedef struct {
  char* name;
  uint32_t duration;
  uint32_t input_delays[VM_TEST_MAX_INPUTS];
  uint16_t input_keys[VM_TEST_MAX_INPUTS];
} Config;

void run_test(const Config config);