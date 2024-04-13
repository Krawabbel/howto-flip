#include <furi.h>
#include <furi_hal.h>
#include <toolbox/stream/stream.h>
#include <toolbox/stream/file_stream.h>

#include "button_config.h"
#include "vm.h"

typedef struct ButtonConfig {
    bool input_to_key_map[InputKeyMAX][VM_NUM_KEYS];
} ButtonConfig;

static void button_config_connect_input_to_key(
    ButtonConfig* button_config,
    InputKey input_key,
    size_t key_id) {
    const size_t input_id = input_key;
    button_config->input_to_key_map[input_id][key_id] = true;
}

static bool button_config_match_input_key(FuriString* line, InputKey* input_key) {
    furi_assert(line);
    furi_assert(input_key);

    for(size_t i = 0; i < 5; i++) {
        *input_key = (InputKey)(i);
        if(furi_string_end_with_str(line, input_get_key_name(*input_key))) {
            return true;
        }
    }
    return false;
}

static bool button_config_match_key_id(FuriString* line, size_t* key_id) {
    furi_assert(line);
    furi_assert(key_id);

    if(furi_string_empty(line)) {
        return false;
    }

    const char start = furi_string_get_char(line, 0);

    for(size_t i = 0; i < 10; i++) {
        if(start == '0' + i) {
            *key_id = i;
            return true;
        }
    }

    for(size_t i = 0; i < 6; i++) {
        if((start == 'A' + i) || (start == 'a' + i)) {
            *key_id = 10 + i;
            return true;
        }
    }

    return false;
}

ButtonConfig* button_config_alloc(const char* path) {
    ButtonConfig* button_config = malloc(sizeof(ButtonConfig));

    Storage* storage = furi_record_open(RECORD_STORAGE);
    Stream* stream = file_stream_alloc(storage);

    FURI_LOG_D("chip8", "reading config file \"%s\":", path);
    if(file_stream_open(stream, path, FSAM_READ, FSOM_OPEN_EXISTING)) {
        FuriString* line = furi_string_alloc();
        while(stream_read_line(stream, line)) {
            furi_string_replace_all(line, "\r", "");
            furi_string_replace_all(line, "\n", "");
            FURI_LOG_D("chip8", "%s", furi_string_get_cstr(line));

            size_t key_id;
            if(button_config_match_key_id(line, &key_id)) {
                InputKey input_key;
                if(button_config_match_input_key(line, &input_key)) {
                    button_config_connect_input_to_key(button_config, input_key, key_id);
                }
            }
        }
        furi_string_free(line);
        file_stream_close(stream);
    } else {
        FURI_LOG_E("chip8", "failed to open config file, using default config");
        for(size_t key_id = 0; key_id < VM_NUM_KEYS; key_id++) {
            switch(key_id) {
            case 0x5:
                button_config_connect_input_to_key(button_config, InputKeyUp, key_id);
                break;
            case 0x7:
                button_config_connect_input_to_key(button_config, InputKeyLeft, key_id);
                break;
            case 0x8:
                button_config_connect_input_to_key(button_config, InputKeyDown, key_id);
                break;
            case 0x9:
                button_config_connect_input_to_key(button_config, InputKeyRight, key_id);
                break;
            default:
                button_config_connect_input_to_key(button_config, InputKeyOk, key_id);
                break;
            }
        }
    }

    stream_free(stream);
    furi_record_close(RECORD_STORAGE);
    return button_config;
}

void button_config_free(ButtonConfig* button_config, const char* path) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    Stream* stream = file_stream_alloc(storage);

    FURI_LOG_D("chip8", "saving config file \"%s\":", path);
    if(file_stream_open(stream, path, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        for(size_t key_id = 0; key_id < VM_NUM_KEYS; key_id++) {
            for(size_t input_id = 0; input_id < InputKeyMAX; input_id++) {
                if(button_config->input_to_key_map[input_id][key_id]) {
                    const InputKey input_key = (InputKey)(input_id);
                    stream_write_format(
                        stream, "%X -> %s\n", key_id, input_get_key_name(input_key));
                }
            }
        }
        file_stream_close(stream);
    } else {
        FURI_LOG_E("chip8", "failed to save config file");
    }

    stream_free(stream);
    furi_record_close(RECORD_STORAGE);

    free(button_config);
}

uint16_t button_config_map_input_to_keys(ButtonConfig* button_config, InputEvent* input_event) {
    const bool is_key_pressed = (input_event->type == InputTypePress);
    const size_t input_id = (size_t)(input_event->key);
    uint16_t keys = 0x00;
    for(size_t key_id = 0; key_id < VM_NUM_KEYS; key_id++) {
        const int bit = is_key_pressed && button_config->input_to_key_map[input_id][key_id];
        keys |= (bit << key_id);
    }
    return keys;
}