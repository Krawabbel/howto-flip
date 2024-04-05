#include <stdio.h>
#include <time.h>

#include "../chip8/vm.h"

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

  printf("\n---\n\n");
  for (uint8_t y = 0; y < screen_height; y++) {
    for (uint8_t x = 0; x < screen_width; x++) {
      const char* out = vm_get_pixel(vm, x, y) ? "x" : ".";
      printf("%s", out);
    }
    printf("\n");
  }
}

int main() {
  VM* vm = vm_alloc();

  const char* file_name = "../chip8-roms/games/superpong.ch8";

  read_file(vm, file_name);

  vm_start(vm, timestamp());

  uint32_t last_draw = timestamp();

  while (!vm_is_game_over(vm)) {
    const bool game_status = vm_update(vm, timestamp());

    if (!game_status) {
      printf("game error\n");
    }

    if ((timestamp() - last_draw) * 60 > 1000) {
      draw_screen(vm);
      last_draw = timestamp();
    }
  }

  vm_free(vm);
}