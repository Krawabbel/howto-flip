#include "test.h"

int main() {
  // clang-format off
  Config configs[] = {
      {.name = "../chip8-roms/tests/10-delay_timer_test.ch8",   .duration = 10000,  .input_delays = {1000, 1000, 1000, 1000}, .input_keys = {(1<<0x02), (1<<0x08), (1<<0x08), (1<<0x05)}},
      {.name = "../chip8-roms/tests/8-scrolling.ch8",           .duration = 6000,  .input_delays = {1000, 1000, 1000, 2000}, .input_keys = {0x02, 0x02, 0x02, 0x02}},
      {.name = "../chip8-roms/tests/8-scrolling.ch8",           .duration = 6000,  .input_delays = {1000, 1000, 1000, 2000}, .input_keys = {0x02, 0x02, 0x04, 0x02}},
      {.name = "../chip8-roms/tests/5-quirks.ch8",              .duration = 7000,   .input_delays = {1000, 1000, 0000, 0000}, .input_keys = {0x04, 0x04, 0x00, 0x00}},
      {.name = "../chip8-roms/tests/5-quirks.ch8",              .duration = 7000,   .input_delays = {1000, 1000, 0000, 0000}, .input_keys = {0x04, 0x02, 0x00, 0x00}},
      {.name = "../chip8-roms/tests/5-quirks.ch8",              .duration = 7000,   .input_delays = {1000, 0000, 0000, 0000}, .input_keys = {0x02, 0x00, 0x00, 0x00}},
      {.name = "../chip8-roms/tests/0-corax89.ch8",             .duration = 1000,   .input_delays = {0000, 0000, 0000, 0000}, .input_keys = {0x00, 0x00, 0x00, 0x00}},
      {.name = "../chip8-roms/tests/1-chip8-logo.ch8",          .duration = 1000,   .input_delays = {0000, 0000, 0000, 0000}, .input_keys = {0x00, 0x00, 0x00, 0x00}},
      {.name = "../chip8-roms/tests/2-ibm-logo.ch8",            .duration = 1000,   .input_delays = {0000, 0000, 0000, 0000}, .input_keys = {0x00, 0x00, 0x00, 0x00}},
      {.name = "../chip8-roms/tests/3-corax+.ch8",              .duration = 1000,   .input_delays = {0000, 0000, 0000, 0000}, .input_keys = {0x00, 0x00, 0x00, 0x00}},
      {.name = "../chip8-roms/tests/4-flags.ch8",               .duration = 2000,   .input_delays = {0000, 0000, 0000, 0000}, .input_keys = {0x00, 0x00, 0x00, 0x00}},
      {.name = "../chip8-roms/tests/6-keypad.ch8",              .duration = 1000,   .input_delays = {0000, 0000, 0000, 0000}, .input_keys = {0x00, 0x00, 0x00, 0x00}},
      {.name = "../chip8-roms/tests/7-beep.ch8",                .duration = 4000,   .input_delays = {0000, 0000, 0000, 0000}, .input_keys = {0x00, 0x00, 0x00, 0x00}},
      {.name = "../chip8-roms/tests/9-morse_demo.ch8",          .duration = 10000,  .input_delays = {0000, 0000, 0000, 0000}, .input_keys = {0x00, 0x00, 0x00, 0x00}},
      {.name = "../chip8-roms/tests/11-heart_monitor.ch8",      .duration = 3000,   .input_delays = {0000, 0000, 0000, 0000}, .input_keys = {0x00, 0x00, 0x00, 0x00}},
      {.name = "../chip8-roms/tests/12-random_number_test.ch8", .duration = 6000,   .input_delays = {1000, 1000, 1000, 1000}, .input_keys = {0x01, 0x02, 0x04, 0x08}},
      {.name = "../chip8-roms/games/horseyJump.ch8",            .duration = 4000,   .input_delays = {2000, 0000, 0000, 0000}, .input_keys = {0xFF, 0x00, 0x00, 0x00}},
      {.name = "../chip8-roms/games/superpong.ch8",             .duration = 1000,   .input_delays = {0000, 0000, 0000, 0000}, .input_keys = {0x00, 0x00, 0x00, 0x00}},
      {.name = "../chip8-roms/games/snake.ch8",                 .duration = 1000,   .input_delays = {0000, 0000, 0000, 0000}, .input_keys = {0x00, 0x00, 0x00, 0x00}},
      {.name = "../chip8-roms/games/br8kout.ch8",               .duration = 3000,   .input_delays = {0000, 0000, 0000, 0000}, .input_keys = {0x00, 0x00, 0x00, 0x00}},
      {.name = "../chip8-roms/games/mondrian.ch8",              .duration = 100000, .input_delays = {0000, 0000, 0000, 0000}, .input_keys = {0x00, 0x00, 0x00, 0x00}},
  };
  // clang-format on

  for (int i = 0; i < 20; i++) {
    run_test(configs[i]);
  }
}