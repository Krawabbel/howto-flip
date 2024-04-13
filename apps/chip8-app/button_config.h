#pragma once

#include <gui/gui.h>
#include <storage/storage.h>

#define BUTTON_CONFIG_PATH (EXT_PATH("chip8/chip8.config"))

typedef struct ButtonConfig ButtonConfig;

ButtonConfig* button_config_alloc(const char* path);

void button_config_free(ButtonConfig* button_config, const char* path);

uint16_t button_config_map_input_to_keys(ButtonConfig* button_config, InputEvent* input_event);
