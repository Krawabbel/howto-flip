
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "vm.h"

#define MS_PER_CPU_TICK (1000 / CPU_TICKS_PER_SEC)
#define MS_PER_TIMER_TICK (1000 / 60)

// clang-format off
byte HEX_DIGITS[5 * 16] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80, // F
};
// clang-format on

void clear_display(VM* vm) {
    for(int j = 0; j < SCREEN_WIDTH; j++) {
        for(int k = 0; k < SCREEN_HEIGHT; k++) {
            vm->screen[j][k] = false;
        }
    }
    vm->is_draw_pending = true;
}

Chip8Status init(VM* vm, const uint32_t timestamp_ms) {
    if(vm == NULL) return Chip8StatusUninitializedVM;

    vm->is_draw_pending = true;
    vm->pc = PROG_START;
    vm->i = 0x0000;
    vm->sp = 0;
    vm->delay_timer = 0;
    vm->sound_timer = 0;
    vm->is_waiting_for_key = false;
    vm->init_timestamp = timestamp_ms;
    vm->cpu_ticks = 0;
    vm->timer_ticks = 0;

    for(size_t j = 0; j < 0x10; j++) vm->v[j] = 0x00;

    for(size_t j = 0; j < STACK_SIZE; j++) vm->stack[j] = 0x0000;

    for(size_t j = 0; j < 0x10; j++) vm->is_key_pressed[j] = false;

    for(size_t j = 0; j < 80; j++) vm->memory[j] = HEX_DIGITS[j];
    for(size_t j = 80; j < MEMORY_SIZE; j++) vm->memory[j] = 0x00;

    clear_display(vm);

    return Chip8StatusOK;
}

word join(const byte lo, const byte hi) {
    return ((word)(hi) << 8) | (word)(lo);
}

word fetch(VM* vm) {
    const byte hi = vm->memory[vm->pc++];
    const byte lo = vm->memory[vm->pc++];
    return join(lo, hi);
}

Chip8Status execute(VM* vm, word opcode) {
    const word nnn = opcode & 0x0FFF;
    const byte kk = opcode & 0xFF;
    const byte n = opcode & 0x0F;
    const byte x = (opcode & 0x0F00) >> 8;
    const byte y = (opcode & 0x00F0) >> 4;

    switch(opcode & 0xF000) {
    case 0x0000: // SYS addr
        switch(opcode) {
        case 0x00E0: // CLS
            clear_display(vm);
            return Chip8StatusOK;
        case 0x00EE: // RET
            vm->pc = vm->stack[--vm->sp];
            return Chip8StatusOK;
        }
        break;
    case 0x1000: // JP addr
        vm->pc = nnn;
        return Chip8StatusOK;
    case 0x2000: // CALL addr
        if(vm->sp == STACK_SIZE) {
            return Chip8StatusStackOverflowError;
        }
        vm->stack[vm->sp++] = vm->pc;
        vm->pc = nnn;
        return Chip8StatusOK;
    case 0x3000: // SE Vx, byte
        if((vm->v[x]) == kk) vm->pc += 2;
        return Chip8StatusOK;
    case 0x4000: // SNE Vx, byte
        if(vm->v[x] != kk) vm->pc += 2;
        return Chip8StatusOK;
    case 0x5000: // SE Vx, Vy
        if(vm->v[x] == vm->v[y]) vm->pc += 2;
        return Chip8StatusOK;
    case 0x6000: // LD Vx, byte
        vm->v[x] = kk;
        return Chip8StatusOK;
    case 0x7000: // ADD Vx, byte
        vm->v[x] += kk;
        return Chip8StatusOK;
    case 0x8000:
        switch(opcode & 0x000F) {
        case 0x0000: // LD Vx, Vy
            vm->v[x] = vm->v[y];
            return Chip8StatusOK;
        case 0x0001: // OR Vx, Vy
            vm->v[x] |= vm->v[y];
            return Chip8StatusOK;
        case 0x0002: // AND Vx, Vy
            vm->v[x] &= vm->v[y];
            return Chip8StatusOK;
        case 0x0003: // XOR Vx, Vy
            vm->v[x] ^= vm->v[y];
            return Chip8StatusOK;
        case 0x0004: // ADD Vx, Vy
            vm->v[0xF] = vm->v[x] > (0xFF - vm->v[y]) ? 0x01 : 0x00;
            vm->v[x] += vm->v[y];
            return Chip8StatusOK;
        case 0x0005: // SUB Vx, Vy
            vm->v[0xF] = (vm->v[x] > vm->v[y]) ? 0x01 : 0x00;
            vm->v[x] -= vm->v[y];
            return Chip8StatusOK;
        case 0x0006: // SHR Vx {, Vy}
            vm->v[0xF] = vm->v[x] & 0x0001;
            vm->v[x] >>= 1;
            return Chip8StatusOK;
        case 0x0007: // SUBN Vx, Vy
            vm->v[0xF] = vm->v[x] > vm->v[y] ? 0x01 : 0x00;
            vm->v[x] = vm->v[y] - vm->v[x];
            return Chip8StatusOK;
        case 0x000E: // SHL Vx {, Vy}
            vm->v[0xF] = vm->v[x] >> 7;
            vm->v[x] <<= 1;
            return Chip8StatusOK;
        }
        break;
    case 0x9000: // SNE Vx, Vy
        if(vm->v[x] != vm->v[y]) vm->pc += 2;
        return Chip8StatusOK;
    case 0xA000: // LD I, addr
        vm->i = nnn;
        return Chip8StatusOK;
    case 0xB000: // JP V0, addr
        vm->pc = nnn + vm->v[0x0];
        return Chip8StatusOK;
    case 0xC000: // RND Vx, byte
        vm->v[x] = (rand() % 0x100) & kk;
        return Chip8StatusOK;
    case 0xD000: // DRW Vx, Vy, nibble
        bool collision = false;
        for(byte xline = 0; xline < 8; xline++) {
            for(byte yline = 0; yline < n; yline++) {
                const byte col = (vm->v[x] + xline) % SCREEN_WIDTH;
                const byte row = (vm->v[y] + yline) % SCREEN_HEIGHT;
                const bool next = (vm->memory[vm->i + yline] & (1 << (8 - xline))) > 0;
                collision = collision || (vm->screen[col][row] && next);
                vm->screen[col][row] ^= next;
            }
        }
        vm->v[0xF] = collision ? 0x01 : 0x00;
        vm->is_draw_pending = true;
        return Chip8StatusOK;
    case 0xE000:
        switch(opcode & 0x00FF) {
        case 0x009E: // SKP Vx
            if((vm->is_key_pressed[vm->v[x]])) {
                vm->pc += 2;
            }
            return Chip8StatusOK;
        case 0x00A1: // SKNP Vx
            if(!(vm->is_key_pressed[vm->v[x]])) {
                vm->pc += 2;
            }
            return Chip8StatusOK;
        }
        break;
    case 0xF000:
        switch(opcode & 0x00FF) {
        case 0x0007: // LD Vx, DT
            vm->v[x] = vm->delay_timer;
            return Chip8StatusOK;
        case 0x000A: // LD Vx, K
            vm->is_waiting_for_key = true;
            vm->waiting_for_key_index = x;
            return Chip8StatusOK;
        case 0x0015: // LD DT, Vx
            vm->delay_timer = vm->v[x];
            return Chip8StatusOK;
        case 0x0018: // LD ST, Vx
            vm->sound_timer = vm->v[x];
            return Chip8StatusOK;
        case 0x001E: // ADD I, Vx
            vm->i = vm->i & vm->v[x];
            return Chip8StatusOK;
        case 0x0029: // LD F, Vx
            vm->i = 5 * vm->v[x];
            return Chip8StatusOK;
        case 0x0033: // LD B, Vx
            const byte vx = vm->v[x];
            vm->memory[vm->i] = vx / 100;
            vm->memory[vm->i + 1] = (vx % 100) / 10;
            vm->memory[vm->i + 2] = (vx % 10);
            return Chip8StatusOK;
        case 0x0055: // LD [I], Vx
            for(int j = 0; j <= x; j++) {
                vm->memory[vm->i + j] = vm->v[j];
            }
            return Chip8StatusOK;
        case 0x0065: // LD Vx, [I]
            for(int j = 0; j <= x; j++) {
                vm->v[j] = vm->memory[vm->i + j];
            }
            return Chip8StatusOK;
        }
        break;
    }
    return Chip8StatusUnknownOpcodeError;
}

void reset_time(VM* vm, const uint32_t timestamp_ms) {
    vm->init_timestamp = timestamp_ms;
    vm->cpu_ticks = 0;
    vm->timer_ticks = 0;
}

bool is_error(const Chip8Status status) {
    return status != Chip8StatusOK;
}

Chip8Status update(VM* vm, const uint32_t timestamp_ms) {
    if(vm == NULL) return Chip8StatusUninitializedVM;

    // handle time
    if(timestamp_ms < vm->init_timestamp) reset_time(vm, timestamp_ms);
    const uint32_t delta_time = timestamp_ms - vm->init_timestamp;

    // handle input
    if(vm->is_waiting_for_key) {
        for(byte key = 0; key < 0x10; key++) {
            if(vm->is_key_pressed[key]) {
                vm->v[vm->waiting_for_key_index] = key;
                vm->is_waiting_for_key = false;
                return Chip8StatusOK;
            }
        }
        return Chip8StatusOK;
    }

    // fetch and execute opcode
    while(delta_time > vm->cpu_ticks * MS_PER_CPU_TICK) {
        const word opcode = fetch(vm);
        const Chip8Status status = execute(vm, opcode);
        if(is_error(status)) {
            return status;
        }

        vm->cpu_ticks++;
        if(vm->cpu_ticks == 0) reset_time(vm, timestamp_ms);
    }

    // handle output
    while(delta_time > vm->timer_ticks * MS_PER_TIMER_TICK) {
        if(vm->delay_timer > 0) vm->delay_timer--;
        if(vm->sound_timer > 0) vm->sound_timer--;

        vm->timer_ticks++;
        if(vm->timer_ticks == 0) reset_time(vm, timestamp_ms);
    }

    return Chip8StatusOK;
}