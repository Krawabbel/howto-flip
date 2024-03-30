#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

typedef uint8_t byte;
typedef uint16_t word;

#define STACK_SIZE 0xFF
#define PROG_START 0x0200
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define TICKS_PER_SEC 500 // Hz (cpu speed)
#define COUNTS_PER_SEC 60 // Hz (delay and sound timer)
#define CLOCKS_PER_TICK (CLOCKS_PER_SEC / TICKS_PER_SEC)
#define CLOCKS_PER_COUNT (CLOCKS_PER_SEC / COUNTS_PER_SEC)

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
    byte memory[0x1000];
    word pc, i;

    byte v[0x10];

    word stack[STACK_SIZE];
    byte sp;

    byte delay_timer, sound_timer;
    clock_t last_clock;
    uint64_t ticks;

    bool is_key_pressed[0x10];

    bool is_waiting_for_key;
    byte waiting_for_key_index;

    bool had_error, is_draw_pending;

    bool screen[SCREEN_WIDTH][SCREEN_HEIGHT];
} VM;

void report_error(VM* vm, const char* msg) {
    vm->had_error = true;
    printf("[ERROR] %s\n", msg);
    printf("pc = %d:\n", vm->pc);
}

void report_warning(VM* vm, const char* msg) {
    printf("[WARNING] %s\n", msg);
}

void clear_display(VM* vm) {
    for(int j = 0; j < SCREEN_WIDTH; j++) {
        for(int k = 0; k < SCREEN_HEIGHT; k++) {
            vm->screen[j][k] = false;
        }
    }
    vm->is_draw_pending = true;
}

void load(VM* vm, const char* path) {
    vm->had_error = false;
    vm->is_draw_pending = true;

    vm->pc = PROG_START;
    vm->i = 0x0000;

    vm->sp = 0;

    vm->delay_timer = 0;
    vm->sound_timer = 0;

    vm->is_waiting_for_key = false;

    for(int j = 0; j < 0x10; j++) vm->v[j] = 0x00;

    for(int j = 0; j < STACK_SIZE; j++) vm->stack[j] = 0x0000;

    for(int j = 0; j < 0x10; j++) vm->is_key_pressed[j] = false;

    vm->last_clock = clock();
    vm->ticks = 0;
    clear_display(vm);

    for(int j = 0; j < 80; j++) vm->memory[j] = HEX_DIGITS[j];

    for(int j = 80; j < PROG_START; j++) vm->memory[j] = 0x00;

    FILE* fileptr = fopen(path, "rb"); // Open the file in binary mode
    if(fileptr == NULL) {
        report_error(vm, "file could not be read: maybe it does not exist");
    }

    fseek(fileptr, 0, SEEK_END); // Jump to the end of the file
    const long filelen = ftell(fileptr); // Get the current byte offset in the file
    rewind(fileptr); // Jump back to the beginning of the file

    if(filelen + PROG_START >= 0x1000) {
        report_error(vm, "file could not be read: too large");
    } else {
        char* buffer = (char*)malloc(filelen * sizeof(char));
        fread(buffer, filelen, 1, fileptr); // Read in the entire file
        for(int j = 0; j < filelen; j++) {
            vm->memory[0x200 + j] = buffer[j];
        }
    }
    fclose(fileptr); // Close the file
}

word join(const byte lo, const byte hi) {
    return ((word)(hi) << 8) | (word)(lo);
}

word fetch(VM* vm) {
    const byte hi = vm->memory[vm->pc++];
    const byte lo = vm->memory[vm->pc++];
    return join(lo, hi);
}

void execute(VM* vm, word opcode) {
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
            return;
        case 0x00EE: // RET
            vm->pc = vm->stack[--vm->sp];
            return;
        }
        report_error(vm, "'SYS CALL' not (yet) implemented");
        return;
    case 0x1000: // JP addr
        vm->pc = nnn;
        return;
    case 0x2000: // CALL addr
        if(vm->sp == STACK_SIZE) {
            report_error(vm, "stack overflow");
        }
        vm->stack[vm->sp++] = vm->pc;
        vm->pc = nnn;
        return;
    case 0x3000: // SE Vx, byte
        if((vm->v[x]) == kk) vm->pc += 2;
        return;
    case 0x4000: // SNE Vx, byte
        if(vm->v[x] != kk) vm->pc += 2;
        return;
    case 0x5000: // SE Vx, Vy
        if(vm->v[x] == vm->v[y]) vm->pc += 2;
        return;
    case 0x6000: // LD Vx, byte
        vm->v[x] = kk;
        return;
    case 0x7000: // ADD Vx, byte
        vm->v[x] += kk;
        return;
    case 0x8000:
        switch(opcode & 0x000F) {
        case 0x0000: // LD Vx, Vy
            vm->v[x] = vm->v[y];
            return;
        case 0x0001: // OR Vx, Vy
            vm->v[x] |= vm->v[y];
            return;
        case 0x0002: // AND Vx, Vy
            vm->v[x] &= vm->v[y];
            return;
        case 0x0003: // XOR Vx, Vy
            vm->v[x] ^= vm->v[y];
            return;
        case 0x0004: // ADD Vx, Vy
            vm->v[0xF] = vm->v[x] > (0xFF - vm->v[y]) ? 0x01 : 0x00;
            vm->v[x] += vm->v[y];
            return;
        case 0x0005: // SUB Vx, Vy
            vm->v[0xF] = (vm->v[x] > vm->v[y]) ? 0x01 : 0x00;
            vm->v[x] -= vm->v[y];
            return;
        case 0x0006: // SHR Vx {, Vy}
            vm->v[0xF] = vm->v[x] & 0x0001;
            vm->v[x] >>= 1;
            return;
        case 0x0007: // SUBN Vx, Vy
            vm->v[0xF] = vm->v[x] > vm->v[y] ? 0x01 : 0x00;
            vm->v[x] = vm->v[y] - vm->v[x];
            return;
        case 0x000E: // SHL Vx {, Vy}
            vm->v[0xF] = vm->v[x] >> 7;
            vm->v[x] <<= 1;
            return;
        }
    case 0x9000: // SNE Vx, Vy
        if(vm->v[x] != vm->v[y]) vm->pc += 2;
        return;
    case 0xA000: // LD I, addr
        vm->i = nnn;
        return;
    case 0xB000: // JP V0, addr
        vm->pc = nnn + vm->v[0x0];
        return;
    case 0xC000: // RND Vx, byte
        vm->v[x] = (rand() % 0x100) & kk;
        return;
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
        return;
    case 0xE000:
        switch(opcode & 0x00FF) {
        case 0x009E: // SKP Vx
            if((vm->is_key_pressed[vm->v[x]])) {
                vm->pc += 2;
            }
            return;
        case 0x00A1: // SKNP Vx
            if(!(vm->is_key_pressed[vm->v[x]])) {
                vm->pc += 2;
            }
            return;
        }
    case 0xF000:
        switch(opcode & 0x00FF) {
        case 0x0007: // LD Vx, DT
            vm->v[x] = vm->delay_timer;
            return;
        case 0x000A: // LD Vx, K
            vm->is_waiting_for_key = true;
            vm->waiting_for_key_index = x;
            return;
        case 0x0015: // LD DT, Vx
            vm->delay_timer = vm->v[x];
            return;
        case 0x0018: // LD ST, Vx
            vm->sound_timer = vm->v[x];
            return;
        case 0x001E: // ADD I, Vx
            vm->i = vm->i & vm->v[x];
            return;
        case 0x0029: // LD F, Vx
            vm->i = 5 * vm->v[x];
            return;
        case 0x0033: // LD B, Vx
            const byte vx = vm->v[x];
            vm->memory[vm->i] = vx / 100;
            vm->memory[vm->i + 1] = (vx % 100) / 10;
            vm->memory[vm->i + 2] = (vx % 10);
            return;
        case 0x0055: // LD [I], Vx
            for(int j = 0; j <= x; j++) {
                vm->memory[vm->i + j] = vm->v[j];
            }
            return;
        case 0x0065: // LD Vx, [I]
            for(int j = 0; j <= x; j++) {
                vm->v[j] = vm->memory[vm->i + j];
            }
            return;
        }
    }
    report_error(vm, "unexpected opcode");
    return;
}

void tick(VM* vm) {
    const word opcode = fetch(vm);
    execute(vm, opcode);
}

void draw(VM* vm) {
    for(int row = 0; row < SCREEN_HEIGHT; row++) {
        for(int col = 0; col < SCREEN_WIDTH; col++) {
            const bool status = vm->screen[col][row];
            printf(status ? "." : " ");
        }
        printf("\n");
    }
    printf("###\n");
}

void step(VM* vm) {
    // vm->is_key_pressed[0x05] = true;

    if(vm->is_waiting_for_key) {
        report_error(vm, "key inputs not yet implemented");
        for(byte key = 0; key < 0x10; key++) {
            if(vm->is_key_pressed[key]) {
                vm->v[vm->waiting_for_key_index] = key;
                vm->is_waiting_for_key = false;
                return;
            }
        }
        return;
    }

    const clock_t curr_clock = clock();
    const clock_t delta_clocks = curr_clock - vm->last_clock;

    if(delta_clocks > CLOCKS_PER_TICK * vm->ticks) {
        tick(vm);
        vm->ticks++;
    }

    if(delta_clocks > CLOCKS_PER_COUNT) {
        vm->ticks = 0;
        vm->last_clock = curr_clock;

        if(vm->delay_timer) vm->delay_timer--;

        if(vm->sound_timer > 0) vm->sound_timer--;

        if(vm->sound_timer > 0) {
            report_warning(vm, "sound not (yet) implemented");
        }

        if(vm->is_draw_pending) {
            draw(vm);
            vm->is_draw_pending = false;
        }
    }
}

int main(int argc, char* argv[]) {
    const char* path = argc > 1 ? argv[1] : "tests/heart_monitor.ch8";
    VM vm;
    load(&vm, path);
    while(!vm.had_error) {
        step(&vm);
    }
    return 0;
}