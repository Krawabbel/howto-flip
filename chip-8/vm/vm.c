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
#define CPU_SPEED 500 // Hz

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

#define NNN(opcode) ((opcode) & 0x0FFF)
#define KK(opcode) ((byte)(opcode))
#define N(opcode) ((byte)(((opcode) & 0x000F)))
#define X(opcode) ((byte)(((opcode) & 0x0F00) >> 2))
#define Y(opcode) ((byte)(((opcode) & 0x00F0) >> 1))

#define NEXT_BYTE(vm) vm->mem[vm->pc++]

typedef struct VirtualMachine {
    byte mem[0x1000];
    word pc, i;

    byte v[0x10];

    word stack[STACK_SIZE];
    byte sp;

    byte delay_timer, sound_timer;
    clock_t last_tick;

    bool is_key_pressed[0x10];

    bool wait_for_key;
    byte store_key_index;

    bool had_error;

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
}

void load(VM* vm, const char* path) {
    vm->had_error = false;

    vm->pc = PROG_START;
    vm->i = 0x0000;

    vm->sp = 0;

    vm->delay_timer = 0;
    vm->sound_timer = 0;

    vm->wait_for_key = false;

    for(int j = 0; j < 80; j++) {
        vm->mem[j] = HEX_DIGITS[j];
    }

    for(int j = 80; j < PROG_START; j++) {
        vm->mem[j] = 0x00;
    }

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
            vm->mem[0x200 + j] = buffer[j];
        }
    }
    fclose(fileptr); // Close the file

    for(int j = 0; j < 0x10; j++) {
        vm->v[j] = 0x00;
    }

    for(int j = 0; j < STACK_SIZE; j++) {
        vm->stack[j] = 0x0000;
    }

    for(int j = 0; j < 0x10; j++) {
        vm->is_key_pressed[j] = false;
    }

    vm->last_tick = clock();
    clear_display(vm);
}

word join(const byte lo, const byte hi) {
    return ((word)(hi) << 8) | (word)(lo);
}

word fetch(VM* vm) {
    const byte hi = NEXT_BYTE(vm);
    const byte lo = NEXT_BYTE(vm);
    return join(lo, hi);
}

void execute(VM* vm, word opcode) {
    // unique opcodes
    switch(opcode) {
    case 0x00E0: // CLS
        clear_display(vm);
        return;
    case 0x00EE: // RET
        vm->pc = vm->stack[--vm->sp];
        return;
    }

    switch(opcode & 0xF000) {
    case 0x0000: // SYS addr
        report_error(vm, "'SYS' not (yet) implemented");
        return;
    case 0x1000: // JP addr
        vm->pc = NNN(opcode);
        return;
    case 0x2000: // CALL addr
        if(vm->sp == STACK_SIZE) {
            report_error(vm, "stack overflow");
        }
        vm->stack[vm->sp++] = vm->pc;
        vm->pc = NNN(opcode);
        return;
    case 0x3000: // SE Vx, byte
        if((vm->v[X(opcode)]) == KK(opcode)) {
            vm->pc += 2;
        }
        return;
    case 0x4000: // SNE Vx, byte
        if(vm->v[X(opcode)] != KK(opcode)) {
            vm->pc += 2;
        }
        return;
    case 0x5000: // SE Vx, Vy
        if(vm->v[X(opcode)] == vm->v[Y(opcode)]) {
            vm->pc += 2;
        }
        return;
    case 0x6000: // LD Vx, byte
        vm->v[X(opcode)] = KK(opcode);
        return;
    case 0x7000: // ADD Vx, byte
        vm->v[X(opcode)] += KK(opcode);
        return;
    case 0x8000:
        switch(opcode & 0x000F) {
        case 0x0000: // LD Vx, Vy
            vm->v[X(opcode)] = vm->v[Y(opcode)];
            return;
        case 0x0001: // OR Vx, Vy
            vm->v[X(opcode)] |= vm->v[Y(opcode)];
            return;
        case 0x0002: // AND Vx, Vy
            vm->v[X(opcode)] &= vm->v[Y(opcode)];
            return;
        case 0x0003: // XOR Vx, Vy
            vm->v[X(opcode)] ^= vm->v[Y(opcode)];
            return;
        case 0x0004: // ADD Vx, Vy
            vm->v[0xF] = vm->v[X(opcode)] > 0xFF - vm->v[Y(opcode)] ? 1 : 0;
            vm->v[X(opcode)] += vm->v[Y(opcode)];
            return;
        case 0x0005: // SUB Vx, Vy
            vm->v[0xF] = vm->v[X(opcode)] > vm->v[Y(opcode)] ? 1 : 0;
            vm->v[X(opcode)] -= vm->v[Y(opcode)];
            return;
        case 0x0006: // SHR Vx {, Vy}
            vm->v[0xF] = vm->v[X(opcode)] & 0x0001;
            vm->v[X(opcode)] >>= 1;
            return;
        case 0x0007: // SUBN Vx, Vy
            vm->v[0xF] = vm->v[X(opcode)] > vm->v[Y(opcode)] ? 1 : 0;
            vm->v[X(opcode)] = vm->v[Y(opcode)] - vm->v[X(opcode)];
            return;
        case 0x000E: // SHL Vx {, Vy}
            vm->v[0xF] = (vm->v[X(opcode)] & (1 << 7)) > 0 ? 1 : 0;
            vm->v[X(opcode)] <<= 1;
            return;
        }
    case 0x9000: // SNE Vx, Vy
        if(vm->v[X(opcode)] != vm->v[Y(opcode)]) {
            vm->pc += 2;
        }
        return;
    case 0xA000: // LD I, addr
        vm->i = NNN(opcode);
        return;
    case 0xB000: // JP V0, addr
        vm->pc = NNN(opcode) + vm->v[0x0];
        return;
    case 0xC000: // RND Vx, byte
        vm->v[X(opcode)] = rand() & KK(opcode);
        return;
    case 0xD000: // DRW Vx, Vy, nibble
        const byte n = N(opcode);
        for(byte r = 0; r < 8; r++) {
            for(byte s = 0; s < n; s++) {
                const byte x = (vm->v[X(opcode)] + r) % SCREEN_WIDTH;
                const byte y = (vm->v[Y(opcode)] + s) % SCREEN_HEIGHT;
                const bool status = (vm->mem[vm->i + s] & (1 << r)) > 0;
                vm->v[0xF] = vm->screen[x][y] && status;
                vm->screen[x][y] ^= status;
            }
        }
        return;
    case 0xE000:
        switch(opcode & 0x00FF) {
        case 0x009E: // SKP Vx
            if((vm->is_key_pressed[vm->v[X(opcode)]])) {
                vm->pc += 2;
            }
            return;
        case 0x00A1: // SKNP Vx
            if(!(vm->is_key_pressed[vm->v[X(opcode)]])) {
                vm->pc += 2;
            }
            return;
        }
    case 0xF000:
        switch(opcode & 0x00FF) {
        case 0x0007: // LD Vx, DT
            vm->v[X(opcode)] = vm->delay_timer;
            return;
        case 0x000A: // LD Vx, K
            vm->wait_for_key = true;
            vm->store_key_index = X(opcode);
            return;
        case 0x0015: // LD DT, Vx
            vm->delay_timer = vm->v[X(opcode)];
            return;
        case 0x0018: // LD ST, Vx
            vm->v[X(opcode)] = vm->sound_timer;
            return;
        case 0x001E: // ADD I, Vx
            vm->i = (vm->i & vm->v[X(opcode)]) & 0xFFF;
            return;
        case 0x0029: // LD F, Vx
            vm->i = 5 * vm->v[X(opcode)];
            return;
        case 0x0033: // LD B, Vx
            const byte vx = vm->v[X(opcode)];
            vm->mem[vm->i] = vx / 100;
            vm->mem[vm->i + 1] = (vx % 100) / 10;
            vm->mem[vm->i + 2] = (vx % 10);
            return;
        case 0x0055: // LD [I], Vx
            for(int j = 0; j <= X(opcode); j++) {
                vm->mem[vm->i + j] = vm->v[j];
            }
            return;
        case 0x0065: // LD Vx, [I]
            for(int j = 0; j <= X(opcode); j++) {
                vm->v[j] = vm->mem[vm->i + j];
            }
            return;
        }
    }
    report_error(vm, "unexpected opcode");
    return;
}

void cycle(VM* vm) {
    if(vm->wait_for_key) {
        report_error(vm, "waiting for key input: not yet implemented");
    }

    // handle input

    const word opcode = fetch(vm);
    execute(vm, opcode);
}

void draw(VM* vm) {
    for(int k = 0; k < SCREEN_HEIGHT; k++) {
        for(int j = 0; j < SCREEN_WIDTH; j++) {
            const bool status = vm->screen[j][k];
            printf(status ? "." : " ");
        }
        printf("\n");
    }
    printf("###\n");
}

void tick(VM* vm) {
    for(int t = 0; t < 5; t++) {
        cycle(vm);
    }

    if(vm->delay_timer) {
        vm->delay_timer--;
    }

    if(vm->sound_timer > 0) {
        vm->sound_timer--;
        report_warning(vm, "sound not yet implemented");
    }
    draw(vm);
}

void step(VM* vm) {
    const clock_t curr_tick = clock();
    if((curr_tick - vm->last_tick) > CLOCKS_PER_SEC / 60) {
        tick(vm);
        vm->last_tick = curr_tick;
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