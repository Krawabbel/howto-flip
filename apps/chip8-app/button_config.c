#include <gui/gui.h>
#include <gui/view.h>
#include <storage/storage.h>
#include <toolbox/stream/stream.h>
#include <toolbox/stream/file_stream.h>

#include "button_config.h"

typedef struct ButtonConfig {
    View* view;
    bool key_used[16];
    bool any_key_used
} ButtonConfig;

void button_config_start(ButtonConfig* button_config, FuriString* path) {
    FURI_LOG_D("chip8", "loading file '%s'", furi_string_get_cstr(path));

    Storage* storage = furi_record_open(RECORD_STORAGE);
    Stream* stream = file_stream_alloc(storage);

    uint8_t c[2];
    furi_check(
        file_stream_open(stream, furi_string_get_cstr(path), FSAM_READ, FSOM_OPEN_EXISTING));

    for(size_t addr = 0; !stream_eof(stream); addr++) {
        if(stream_read(stream, c, 2) == 2) {
            const uint16_t opcode = ((uint16_t)(c[0]) << 8) | (uint16_t)(c[1]);
            const uint8_t x = (opcode & 0x0F00) >> 8;
            switch(opcode & 0xF0FF) {
            case 0xE09E:
                button_config->key_used[x] = true;
                break;
            case 0xE0A1:
                button_config->key_used[x] = true;
                break;
            case 0xF00E:
            }
        }
    }

    file_stream_close(stream);
    stream_free(stream);
    furi_record_close(RECORD_STORAGE);
}