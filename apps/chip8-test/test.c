#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../chip8/vm.h"
#include "test.h"

uint32_t timestamp() {
  return clock() * 1000 / CLOCKS_PER_SEC;
}

void read_file(VM* vm, const char* file_name) {
  FILE* file = fopen(file_name, "r");

  for (word addr = 0; !feof(file); addr++) {
    const byte c = fgetc(file);
    vm_write_prog_to_memory(vm, addr, c);
  }
  printf("file '%s' successfully loaded\n", file_name);
}

void draw_screen(VM* vm) {
  const int screen_width = vm_get_screen_width(vm);
  const int screen_height = vm_get_screen_height(vm);

  printf("\n\n\n");
  for (uint8_t y = 0; y < screen_height; y++) {
    for (uint8_t x = 0; x < screen_width; x++) {
      const char* out = vm_get_pixel(vm, x, y) ? "x" : ".";
      printf("%s", out);
    }
    printf("\n");
  }
}

void run_test(const Config config) {
  VM* vm = vm_alloc();
  read_file(vm, config.name);

  vm_start(vm, timestamp());

  const uint32_t start_timestamp = timestamp();
  uint32_t last_draw_timestamp = start_timestamp;
  uint32_t last_input_timestamp = start_timestamp;
  int next_input = 0;

  while (timestamp() - start_timestamp < config.duration) {
    // handle input
    const bool is_input_pending = (next_input < VM_TEST_MAX_INPUTS);
    const bool is_input_initialized = config.input_keys[next_input] > 0;
    const bool is_input_overdue =
        timestamp() - last_input_timestamp > config.input_delays[next_input];
    const bool apply_input =
        is_input_pending && is_input_initialized && is_input_overdue;
    if (apply_input) {
      vm_set_keys(vm, config.input_keys[next_input]);
      next_input++;
      last_input_timestamp = timestamp();
    }
    // handle game status
    const bool game_status = vm_update(vm, timestamp());
    if (!game_status) {
      printf("game error\n");
    }
    // handle output
    if ((timestamp() - last_draw_timestamp) * 60 > 1000) {
      draw_screen(vm);
      last_draw_timestamp = timestamp();
    }
  }

  vm_free(vm);

  do {
    printf("\ndemo of chip-8 program '%s' ended\n\npress ENTER to continue ",
           config.name);
  } while (getchar() != '\n');
}