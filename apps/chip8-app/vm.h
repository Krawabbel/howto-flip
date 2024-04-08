#pragma once

#include <stdbool.h>
#include <stdint.h>

#define VM_NUM_KEYS 16
#define VM_CPU_TICKS_PER_SEC 500 // Hz (cpu speed)

typedef uint8_t byte;
typedef uint16_t word;

typedef struct VirtualMachine VM;

VM* vm_alloc();

void vm_free(VM* vm);

void vm_start(VM* vm, const uint32_t timestamp_world);
bool vm_update(VM* vm, const uint32_t timestamp_world);
bool vm_is_game_over(VM* vm);

void vm_write_prog_to_memory(VM* vm, const word addr, const byte data);

void vm_set_keys(VM* vm, const word key_bitfield);
word vm_get_keys(VM* vm);

bool vm_get_pixel(VM* vm, const int x_screen, const int y_screen);
bool vm_is_sound_playing(VM* vm);

uint32_t vm_calc_cpu_speed(VM* vm, const uint32_t timestamp_world);
uint32_t vm_calc_timer_speed(VM* vm, const uint32_t timestamp_world);

int vm_get_screen_width(VM* vm);
int vm_get_screen_height(VM* vm);