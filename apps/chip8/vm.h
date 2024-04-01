#pragma once

#include <stdbool.h>
#include <stdint.h>

#define VM_SCREEN_WIDTH 64
#define VM_SCREEN_HEIGHT 32
#define VM_NUM_KEYS 16
#define VM_TICKS_PER_SEC 500 // Hz (cpu speed)

typedef uint8_t byte;
typedef uint16_t word;

typedef struct VirtualMachine VM;

VM* vm_alloc();

void vm_free(VM* vm);

void vm_start(VM* vm, const uint32_t timestamp_ms);

bool vm_update(VM* vm, const uint32_t timestamp_ms);

void vm_write_prog_to_memory(VM* vm, const word addr, const byte data);

void vm_set_key_input(VM* vm, const size_t key, const bool is_pressed);

bool vm_get_pixel(VM* vm, const size_t x, const size_t y);

bool vm_is_sound_playing(VM* vm);

uint32_t vm_speed(VM* vm, const uint32_t timestamp_ms);