#include "test.h"

#define NO_KEY 0

uint16_t key(const uint8_t key_id) {
  return (1 << key_id);
}

int main() {
  // clang-format off
  Config configs[] = {
      {.name = "../chip8-roms/games/br8kout.ch8",               .duration = 6000,   .input_delays = {4000,    0,    0,    0}, .input_keys = {0xFF, NO_KEY, NO_KEY, NO_KEY}},
      {.name = "../chip8-roms/tests/6-keypad.ch8",              .duration = 6000,   .input_delays = {1000, 1000, 1000, 1000}, .input_keys = {key(1), key(0xD), key(0xF), key(0)}},
      {.name = "../chip8-roms/tests/6-keypad.ch8",              .duration = 6000,   .input_delays = {1000, 1000, 1000, 1000}, .input_keys = {key(2), key(0xD), key(0xF), key(0)}},
      {.name = "../chip8-roms/tests/6-keypad.ch8",              .duration = 6000,   .input_delays = {1000, 1000, 1000, 1000}, .input_keys = {key(3), key(0xD), key(0xF), key(0)}},
      {.name = "../chip8-roms/games/snake.ch8",                 .duration = 10000,  .input_delays = {8000, 1000,  100,  100}, .input_keys = {key(5), key(8), key(9), key(10)}},
      {.name = "../chip8-roms/games/horseyJump.ch8",            .duration = 7000,   .input_delays = {2000, 1000,    0,    0}, .input_keys = {0xFF,   0xFF,   NO_KEY, NO_KEY}},
      {.name = "../chip8-roms/tests/10-delay_timer_test.ch8",   .duration = 10000,  .input_delays = {1000, 1000, 1000, 1000}, .input_keys = {key(2), key(8), key(8), key(5)}},
      {.name = "../chip8-roms/tests/8-scrolling.ch8",           .duration = 6000,   .input_delays = {1000, 3000, 1000, 2000}, .input_keys = {key(1), key(1), key(1), key(1)}},
      {.name = "../chip8-roms/tests/8-scrolling.ch8",           .duration = 6000,   .input_delays = {1000, 1000, 1000, 2000}, .input_keys = {key(1), key(1), key(2), key(1)}},
      {.name = "../chip8-roms/tests/5-quirks.ch8",              .duration = 7000,   .input_delays = {1000, 1000,    0,    0}, .input_keys = {key(2), key(2), NO_KEY, NO_KEY}},
      {.name = "../chip8-roms/tests/5-quirks.ch8",              .duration = 7000,   .input_delays = {1000, 1000,    0,    0}, .input_keys = {key(2), key(1), NO_KEY, NO_KEY}},
      {.name = "../chip8-roms/tests/5-quirks.ch8",              .duration = 7000,   .input_delays = {1000,    0,    0,    0}, .input_keys = {key(1), NO_KEY, NO_KEY, NO_KEY}},
      {.name = "../chip8-roms/tests/0-corax89.ch8",             .duration = 1000,   .input_delays = {   0,    0,    0,    0}, .input_keys = {NO_KEY, NO_KEY, NO_KEY, NO_KEY}},
      {.name = "../chip8-roms/tests/1-chip8-logo.ch8",          .duration = 1000,   .input_delays = {   0,    0,    0,    0}, .input_keys = {NO_KEY, NO_KEY, NO_KEY, NO_KEY}},
      {.name = "../chip8-roms/tests/2-ibm-logo.ch8",            .duration = 1000,   .input_delays = {   0,    0,    0,    0}, .input_keys = {NO_KEY, NO_KEY, NO_KEY, NO_KEY}},
      {.name = "../chip8-roms/tests/3-corax+.ch8",              .duration = 1000,   .input_delays = {   0,    0,    0,    0}, .input_keys = {NO_KEY, NO_KEY, NO_KEY, NO_KEY}},
      {.name = "../chip8-roms/tests/4-flags.ch8",               .duration = 2000,   .input_delays = {   0,    0,    0,    0}, .input_keys = {NO_KEY, NO_KEY, NO_KEY, NO_KEY}},
      {.name = "../chip8-roms/tests/7-beep.ch8",                .duration = 4000,   .input_delays = {   0,    0,    0,    0}, .input_keys = {NO_KEY, NO_KEY, NO_KEY, NO_KEY}},
      {.name = "../chip8-roms/tests/9-morse_demo.ch8",          .duration = 10000,  .input_delays = {   0,    0,    0,    0}, .input_keys = {NO_KEY, NO_KEY, NO_KEY, NO_KEY}},
      {.name = "../chip8-roms/tests/11-heart_monitor.ch8",      .duration = 3000,   .input_delays = {   0,    0,    0,    0}, .input_keys = {NO_KEY, NO_KEY, NO_KEY, NO_KEY}},
      {.name = "../chip8-roms/tests/12-random_number_test.ch8", .duration = 6000,   .input_delays = {1000, 1000, 1000, 1000}, .input_keys = {key(0), key(1), key(2), key(3)}},
      {.name = "../chip8-roms/games/superpong.ch8",             .duration = 1000,   .input_delays = {   0,    0,    0,    0}, .input_keys = {NO_KEY, NO_KEY, NO_KEY, NO_KEY}},
      {.name = "../chip8-roms/games/mondrian.ch8",              .duration = 100000, .input_delays = {   0,    0,    0,    0}, .input_keys = {NO_KEY, NO_KEY, NO_KEY, NO_KEY}},
  };
  // clang-format on

  for (int i = 0; i < 20; i++) {
    run_test(configs[i]);
  }
}