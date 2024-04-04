
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
#define MAX_SCREEN_WIDTH 128
#define MAX_SCREEN_HEIGHT 64

typedef enum {
    ScreenResolutionChip8,
    ScreenResolutionSuperChip8,
} ScreenResolution;

// clang-format off
byte SMALL_HEX_DIGITS[80] = {
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

byte LARGE_HEX_DIGITS[160] = {
    0x3C, 0x7E, 0xE7, 0xC3, 0xC3, 0xC3, 0xC3, 0xE7, 0x7E, 0x3C,
    0x18, 0x38, 0x58, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C,
    0x3E, 0x7F, 0xC3, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xFF, 0xFF,
    0x3C, 0x7E, 0xC3, 0x03, 0x0E, 0x0E, 0x03, 0xC3, 0x7E, 0x3C,
    0x06, 0x0E, 0x1E, 0x36, 0x66, 0xC6, 0xFF, 0xFF, 0x06, 0x06,
    0xFF, 0xFF, 0xC0, 0xC0, 0xFC, 0xFE, 0x03, 0xC3, 0x7E, 0x3C,
    0x3E, 0x7C, 0xE0, 0xC0, 0xFC, 0xFE, 0xC3, 0xC3, 0x7E, 0x3C,
    0xFF, 0xFF, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x60, 0x60,
    0x3C, 0x7E, 0xC3, 0xC3, 0x7E, 0x7E, 0xC3, 0xC3, 0x7E, 0x3C,
    0x3C, 0x7E, 0xC3, 0xC3, 0x7F, 0x3F, 0x03, 0x03, 0x3E, 0x7C,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // no hex chars!
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,    
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

    bool is_game_over;

    bool is_key_pressed[0x10];

    bool is_waiting_for_key;
    byte waiting_for_key_index;

    ScreenResolution screen_resolution;
    int scroll_horizontal, scroll_vertical;
    bool screen[MAX_SCREEN_WIDTH][MAX_SCREEN_HEIGHT];
} VM;

int vm_get_screen_width(VM* vm) {
    switch(vm->screen_resolution) {
    case ScreenResolutionSuperChip8:
        return 128;
    case ScreenResolutionChip8:
        return 64;
    }
    return -1;
}

int vm_get_screen_height(VM* vm) {
    switch(vm->screen_resolution) {
    case ScreenResolutionSuperChip8:
        return 64;
    case ScreenResolutionChip8:
        return 32;
    }
    return -1;
}

static bool fetch_is_key_pressed(VM* vm, const byte key_id) {
    const bool ret = vm->is_key_pressed[key_id];
    vm->is_key_pressed[key_id] = false;
    return ret;
}

static void clear_display(VM* vm) {
    for(int j = 0; j < vm_get_screen_width(vm); j++) {
        for(int k = 0; k < vm_get_screen_height(vm); k++) {
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
    for(size_t j = 0; j < 0x10; j++) vm->v[j] = 0x00;
    vm->delay_timer = 0;
    vm->sound_timer = 0;

    vm->timestamp_init = timestamp_world;
    vm->cpu_ticks = 0;
    vm->timer_ticks = 0;
    vm->is_game_over = false;

    vm->is_waiting_for_key = false;

    vm->screen_resolution = ScreenResolutionChip8;
    vm->scroll_horizontal = 0;
    vm->scroll_vertical = 0;

    for(size_t j = 0; j < 0x10; j++) vm->is_key_pressed[j] = false;
    for(size_t j = 0; j < 80; j++) vm->memory[j] = SMALL_HEX_DIGITS[j];
    for(size_t j = 0; j < 160; j++) vm->memory[80 + j] = LARGE_HEX_DIGITS[j];

    clear_display(vm);
}

static word join(const byte lo, const byte hi) {
    return ((word)(hi) << 8) | (word)(lo);
}

static word fetch(VM* vm) {
    const byte hi = vm->memory[vm->pc++];
    const byte lo = vm->memory[vm->pc++];
    return join(lo, hi);
}

static bool execute(VM* vm, word opcode) {
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
        case 0x00FB: // SCROLL_RIGHT
            vm->scroll_horizontal += 4;
            return true;
        case 0x00FC: // SCROLL_LEFT
            vm->scroll_horizontal -= 4;
            return true;
        case 0x00FE: //LORES
            vm->screen_resolution = ScreenResolutionChip8;
            return true;
        case 0x00FF: //HIRES
            vm->screen_resolution = ScreenResolutionSuperChip8;
            return true;
        default:
            if((opcode & 0x00C0) == 0x00C0) { // SCROLL_DOWN_N
                vm->scroll_vertical += n;
                return true;
            }
            return false;
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
        vm->v[0xF] = 0x00;
        const bool large_sprite = (n == 0);
        const byte sprite_height = large_sprite ? 16 : n;
        const byte bytes_per_row = large_sprite ? 2 : 1;
        word addr = vm->i;
        for(byte row = 0; row < sprite_height; row++) {
            const byte y_screen = (vm->v[y] + row) % vm_get_screen_height(vm);
            for(byte row_byte = 0; row_byte < bytes_per_row; row_byte++) {
                const byte b = vm->memory[addr];
                for(byte col = 0; col < 8; col++) {
                    const byte x_screen =
                        (vm->v[x] + col + 8 * row_byte) % vm_get_screen_width(vm);
                    const bool next = (b & (1 << (7 - col))) > 0;
                    if(next) {
                        const bool prev = vm->screen[x_screen][y_screen];
                        vm->screen[x_screen][y_screen] = (prev != next);
                        vm->v[0xF] = 0x01;
                    }
                }
                addr++;
            }
        }

        return true;
    case 0xE000:
        switch(opcode & 0x00FF) {
        case 0x009E: // SKP Vx
            if(fetch_is_key_pressed(vm, vm->v[x])) {
                vm->pc += 2;
            }
            return true;
        case 0x00A1: // SKNP Vx
            if(!fetch_is_key_pressed(vm, vm->v[x])) {
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
        case 0x0029: // LD SMALLHEX, Vx
            vm->i = 5 * vm->v[x];
            return true;
        case 0x0030: // LD BIGHEX, Vx
            vm->i = 80 + 10 * vm->v[x];
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
        case 0x0075: // SAVE FLAGS, Vx
            for(int j = 0; j <= x; j++) {
                // save vm->v[j] to persistent memory
            }
            return false;
        case 0x0085: // LOAD FLAGS, Vx
            for(int j = 0; j <= x; j++) {
                // load vm->v[j] from persistent memory
            }
            return false;
        case 0x00FD: // EXIT
            // vm->is_game_over = true;
            return true;
        }

        break;
    }
    return false;
}

static void reset_time(VM* vm, const uint32_t timestamp_world) {
    vm->timestamp_init = timestamp_world;
    vm->cpu_ticks = 0;
    vm->timer_ticks = 0;
}

static uint32_t vm_tick_speed(const uint64_t ticks, const uint32_t uptime_ms) {
    const uint32_t uptime_s = uptime_ms / 1000;
    return uptime_s > 0 ? ticks / uptime_s : 0;
}

static uint32_t uptime(VM* vm, const uint32_t timestamp_world) {
    return timestamp_world - vm->timestamp_init;
}

uint32_t vm_calc_timer_speed(VM* vm, const uint32_t timestamp_world) {
    return vm_tick_speed(vm->timer_ticks, uptime(vm, timestamp_world));
}

uint32_t vm_calc_cpu_speed(VM* vm, const uint32_t timestamp_world) {
    return vm_tick_speed(vm->cpu_ticks, uptime(vm, timestamp_world));
}

static void handle_input(VM* vm) {
    if(vm->is_waiting_for_key) {
        for(byte key_id = 0; key_id < 0x10; key_id++) {
            if(fetch_is_key_pressed(vm, key_id)) {
                vm->v[vm->waiting_for_key_index] = key_id;
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
        return tick_cpu(vm);
    } else {
        tick_timers(vm);
        return true;
    }
}

bool vm_update(VM* vm, const uint32_t timestamp_world) {
    // handle time
    if(timestamp_world < vm->timestamp_init) reset_time(vm, timestamp_world);

    handle_input(vm);

    while((!vm->is_game_over) &&
          ((timestamp_cpu(vm) < timestamp_world) || (timestamp_timers(vm) < timestamp_world))) {
        if(!handle_scheduling(vm)) {
            return false;
        }
    }

    return true;
}

bool vm_is_game_over(VM* vm) {
    return vm->is_game_over;
}

void vm_write_prog_to_memory(VM* vm, const word addr, const byte data) {
    vm->memory[PROG_START + addr] = data;
}

void vm_set_keys(VM* vm, const word key_bitfield) {
    for(size_t id = 0; id < VM_NUM_KEYS; id++) {
        vm->is_key_pressed[id] = (key_bitfield & (1 << id)) > 0;
    }
}

bool vm_get_pixel(VM* vm, const int x_screen, const int y_screen) {
    const int x = x_screen - vm->scroll_horizontal;
    const int y = y_screen - vm->scroll_vertical;
    if(x < 0 || x >= vm_get_screen_width(vm) || y < 0 || y >= vm_get_screen_height(vm)) {
        return false;
    }
    return vm->screen[x][y];
}

bool vm_is_sound_playing(VM* vm) {
    return vm->sound_timer > 0;
}