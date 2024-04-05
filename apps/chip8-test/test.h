#include <stdbool.h>
#include <stdint.h>

typedef struct {
  char* name;
  uint32_t duration;
  uint32_t input_delay;
  bool input_repeat;
  uint16_t input_keys;
} Config;

void run_test(const Config config);