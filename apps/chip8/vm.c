
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>

#include "vm.h"

#define PROG_START 0x0200
#define MEMORY_SIZE 0x1000
#define STACK_SIZE 0xFF
#define TIMER_TICKS_PER_SEC 60
#define MS_PER_CPU_TICK (1000 / VM_CPU_TICKS_PER_SEC)
#define MS_PER_TIMER_TICK (1000 / TIMER_TICKS_PER_SEC)

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

typedef struct VirtualMachine {
    byte memory[MEMORY_SIZE];
    word pc, i;

    byte v[0x10];

    word stack[STACK_SIZE];
    byte sp;

    byte delay_timer, sound_timer;
    uint32_t timestamp_init;
    uint64_t cpu_ticks, timer_ticks;

    bool is_key_pressed[0x10];

    bool is_waiting_for_key;
    byte waiting_for_key_index;

    bool screen[VM_SCREEN_WIDTH][VM_SCREEN_HEIGHT];
} VM;

void clear_display(VM* vm) {
    for(int j = 0; j < VM_SCREEN_WIDTH; j++) {
        for(int k = 0; k < VM_SCREEN_HEIGHT; k++) {
            vm->screen[j][k] = false;
        }
    }
}

VM* vm_alloc() {
    VM* vm = malloc(sizeof(VM));
    return vm;
}

void vm_free(VM* vm) {
    free(vm);
}

void vm_start(VM* vm, const uint32_t timestamp_world) {
    vm->pc = PROG_START;
    vm->i = 0x0000;
    vm->sp = 0;
    vm->delay_timer = 0;
    vm->sound_timer = 0;
    vm->is_waiting_for_key = false;
    vm->timestamp_init = timestamp_world;
    vm->cpu_ticks = 0;
    vm->timer_ticks = 0;

    for(size_t j = 0; j < 0x10; j++) vm->v[j] = 0x00;
    for(size_t j = 0; j < STACK_SIZE; j++) vm->stack[j] = 0x0000;
    for(size_t j = 0; j < 0x10; j++) vm->is_key_pressed[j] = false;
    for(size_t j = 0; j < 80; j++) vm->memory[j] = HEX_DIGITS[j];
    for(size_t j = 80; j < PROG_START; j++) vm->memory[j] = 0x00;

    clear_display(vm);
}

word join(const byte lo, const byte hi) {
    return ((word)(hi) << 8) | (word)(lo);
}

word fetch(VM* vm) {
    const byte hi = vm->memory[vm->pc++];
    const byte lo = vm->memory[vm->pc++];
    return join(lo, hi);
}

bool execute(VM* vm, word opcode) {
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
            return true;
        case 0x00EE: // RET
            vm->pc = vm->stack[--vm->sp];
            return true;
        }
        break;
    case 0x1000: // JP addr
        vm->pc = nnn;
        return true;
    case 0x2000: // CALL addr
        if(vm->sp == STACK_SIZE) {
            return false;
        }
        vm->stack[vm->sp++] = vm->pc;
        vm->pc = nnn;
        return true;
    case 0x3000: // SE Vx, byte
        if((vm->v[x]) == kk) vm->pc += 2;
        return true;
    case 0x4000: // SNE Vx, byte
        if(vm->v[x] != kk) vm->pc += 2;
        return true;
    case 0x5000: // SE Vx, Vy
        if(vm->v[x] == vm->v[y]) vm->pc += 2;
        return true;
    case 0x6000: // LD Vx, byte
        vm->v[x] = kk;
        return true;
    case 0x7000: // ADD Vx, byte
        vm->v[x] += kk;
        return true;
    case 0x8000:
        switch(opcode & 0x000F) {
        case 0x0000: // LD Vx, Vy
            vm->v[x] = vm->v[y];
            return true;
        case 0x0001: // OR Vx, Vy
            vm->v[x] |= vm->v[y];
            return true;
        case 0x0002: // AND Vx, Vy
            vm->v[x] &= vm->v[y];
            return true;
        case 0x0003: // XOR Vx, Vy
            vm->v[x] ^= vm->v[y];
            return true;
        case 0x0004: // ADD Vx, Vy
            vm->v[0xF] = vm->v[x] > (0xFF - vm->v[y]) ? 0x01 : 0x00;
            vm->v[x] += vm->v[y];
            return true;
        case 0x0005: // SUB Vx, Vy
            vm->v[0xF] = (vm->v[x] > vm->v[y]) ? 0x01 : 0x00;
            vm->v[x] -= vm->v[y];
            return true;
        case 0x0006: // SHR Vx {, Vy}
            vm->v[0xF] = vm->v[x] & 0x0001;
            vm->v[x] >>= 1;
            return true;
        case 0x0007: // SUBN Vx, Vy
            vm->v[0xF] = vm->v[x] > vm->v[y] ? 0x01 : 0x00;
            vm->v[x] = vm->v[y] - vm->v[x];
            return true;
        case 0x000E: // SHL Vx {, Vy}
            vm->v[0xF] = vm->v[x] >> 7;
            vm->v[x] <<= 1;
            return true;
        }
        break;
    case 0x9000: // SNE Vx, Vy
        if(vm->v[x] != vm->v[y]) vm->pc += 2;
        return true;
    case 0xA000: // LD I, addr
        vm->i = nnn;
        return true;
    case 0xB000: // JP V0, addr
        vm->pc = nnn + vm->v[0x0];
        return true;
    case 0xC000: // RND Vx, byte
        vm->v[x] = (rand() % 0x100) & kk;
        return true;
    case 0xD000: // DRW Vx, Vy, nibble
        bool collision = false;
        for(byte xline = 0; xline < 8; xline++) {
            for(byte yline = 0; yline < n; yline++) {
                const byte col = (vm->v[x] + xline) % VM_SCREEN_WIDTH;
                const byte row = (vm->v[y] + yline) % VM_SCREEN_HEIGHT;
                const bool next = (vm->memory[vm->i + yline] & (1 << (8 - xline))) > 0;
                collision = collision || (vm->screen[col][row] && next);
                vm->screen[col][row] ^= next;
            }
        }
        vm->v[0xF] = collision ? 0x01 : 0x00;

        return true;
    case 0xE000:
        switch(opcode & 0x00FF) {
        case 0x009E: // SKP Vx
            if((vm->is_key_pressed[vm->v[x]])) {
                vm->pc += 2;
            }
            return true;
        case 0x00A1: // SKNP Vx
            if(!(vm->is_key_pressed[vm->v[x]])) {
                vm->pc += 2;
            }
            return true;
        }
        break;
    case 0xF000:
        switch(opcode & 0x00FF) {
        case 0x0007: // LD Vx, DT
            vm->v[x] = vm->delay_timer;
            return true;
        case 0x000A: // LD Vx, K
            vm->is_waiting_for_key = true;
            vm->waiting_for_key_index = x;
            return true;
        case 0x0015: // LD DT, Vx
            vm->delay_timer = vm->v[x];
            return true;
        case 0x0018: // LD ST, Vx
            vm->sound_timer = vm->v[x];
            return true;
        case 0x001E: // ADD I, Vx
            vm->i = vm->i & vm->v[x];
            return true;
        case 0x0029: // LD F, Vx
            vm->i = 5 * vm->v[x];
            return true;
        case 0x0033: // LD B, Vx
            const byte vx = vm->v[x];
            vm->memory[vm->i] = vx / 100;
            vm->memory[vm->i + 1] = (vx % 100) / 10;
            vm->memory[vm->i + 2] = (vx % 10);
            return true;
        case 0x0055: // LD [I], Vx
            for(int j = 0; j <= x; j++) {
                vm->memory[vm->i + j] = vm->v[j];
            }
            return true;
        case 0x0065: // LD Vx, [I]
            for(int j = 0; j <= x; j++) {
                vm->v[j] = vm->memory[vm->i + j];
            }
            return true;
        }
        break;
    }
    return false;
}

void reset_time(VM* vm, const uint32_t timestamp_world) {
    vm->timestamp_init = timestamp_world;
    vm->cpu_ticks = 0;
    vm->timer_ticks = 0;
}

uint32_t vm_tick_speed(const uint64_t ticks, const uint32_t uptime_ms) {
    const uint32_t uptime_s = uptime_ms / 1000;
    return uptime_s > 0 ? ticks / uptime_s : 0;
}

uint32_t vm_uptime(VM* vm, const uint32_t timestamp_world) {
    return timestamp_world - vm->timestamp_init;
}

uint32_t vm_timer_speed(VM* vm, const uint32_t timestamp_world) {
    return vm_tick_speed(vm->timer_ticks, vm_uptime(vm, timestamp_world));
}

uint32_t vm_cpu_speed(VM* vm, const uint32_t timestamp_world) {
    return vm_tick_speed(vm->cpu_ticks, vm_uptime(vm, timestamp_world));
}

static void handle_input(VM* vm) {
    if(vm->is_waiting_for_key) {
        for(byte key = 0; key < 0x10; key++) {
            if(vm->is_key_pressed[key]) {
                vm->v[vm->waiting_for_key_index] = key;
                vm->is_waiting_for_key = false;
            }
        }
    }
}

static bool tick_cpu(VM* vm) {
    vm->cpu_ticks++;
    if(vm->is_waiting_for_key) {
        return true;
    }
    const word opcode = fetch(vm);
    return execute(vm, opcode);
}

static void tick_timers(VM* vm) {
    if(vm->delay_timer > 0) vm->delay_timer--;
    if(vm->sound_timer > 0) vm->sound_timer--;
    vm->timer_ticks++;
}

static uint32_t timestamp_cpu(VM* vm) {
    return vm->timestamp_init + vm->cpu_ticks * MS_PER_CPU_TICK;
}

static uint32_t timestamp_timers(VM* vm) {
    return vm->timestamp_init + vm->timer_ticks * MS_PER_TIMER_TICK;
}

static bool handle_scheduling(VM* vm) {
    if(timestamp_cpu(vm) <= timestamp_timers(vm)) {
        if(!tick_cpu(vm)) {
            return false;
        }
    } else {
        tick_timers(vm);
    }
    return true;
}

bool vm_update(VM* vm, const uint32_t timestamp_world) {
    // handle time
    if(timestamp_world < vm->timestamp_init) reset_time(vm, timestamp_world);

    handle_input(vm);

    while((timestamp_cpu(vm) < timestamp_world) || (timestamp_timers(vm) < timestamp_world)) {
        if(!handle_scheduling(vm)) {
            return false;
        }
    }
    return true;
}

void vm_write_prog_to_memory(VM* vm, const word addr, const byte data) {
    vm->memory[PROG_START + addr] = data;
}

void vm_set_keys(VM* vm, const word keys) {
    for(size_t id = 0; id < VM_NUM_KEYS; id++) {
        vm->is_key_pressed[id] = (keys & (1 << id)) > 0;
    }
}

word vm_get_keys(VM* vm) {
    word keys = 0;
    for(size_t id = 0; id < VM_NUM_KEYS; id++) {
        if(vm->is_key_pressed[id]) {
            keys |= (1 << id);
        }
    }
    return keys;
}

bool vm_get_pixel(VM* vm, const size_t x, const size_t y) {
    return vm->screen[x][y];
}

bool vm_is_sound_playing(VM* vm) {
    return vm->sound_timer > 0;
}