#ifndef CHIP8_VM_H
#define CHIP8_VM_H

#include <stdbool.h>

#define PROG_START 0x0200
#define MEMORY_SIZE 0x1000
#define STACK_SIZE 0xFF
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define CPU_TICKS_PER_SEC 500 // Hz (cpu speed)

typedef uint8_t byte;
typedef uint16_t word;

typedef enum {
    Chip8StatusOK,
    Chip8StatusFileError,
    Chip8StatusUnknownOpcodeError,
    Chip8StatusStackOverflowError,
    Chip8StatusUninitializedVM,
} Chip8Status;

typedef struct VirtualMachine {
    byte memory[MEMORY_SIZE];
    word pc, i;

    byte v[0x10];

    word stack[STACK_SIZE];
    byte sp;

    byte delay_timer, sound_timer;
    uint32_t init_timestamp;
    uint64_t cpu_ticks, timer_ticks;

    bool is_key_pressed[0x10];

    bool is_waiting_for_key;
    byte waiting_for_key_index;

    bool is_draw_pending;

    bool screen[SCREEN_WIDTH][SCREEN_HEIGHT];
} VM;

Chip8Status init(VM* vm, const uint32_t timestamp_ms);

Chip8Status update(VM* vm, const uint32_t timestamp_ms);

#endif // CHIP8_VM_H