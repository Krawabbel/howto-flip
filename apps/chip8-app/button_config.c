#include <furi.h>
#include <storage/storage.h>
#include <toolbox/stream/stream.h>
#include <toolbox/stream/file_stream.h>

#include "button_config.h"

typedef struct ButtonConfig
{
    View *view;
    bool key_used[16];
    bool any_key_used
} ButtonConfig;

static void example_apps_data_print_file_content(Storage *storage, const char *path)
{
    Stream *stream = file_stream_alloc(storage);
    FuriString *line = furi_string_alloc();

    FURI_LOG_I(TAG, "----------------------------------------");
    FURI_LOG_I(TAG, "File \"%s\" content:", path);
    if (file_stream_open(stream, path, FSAM_READ, FSOM_OPEN_EXISTING))
    {
        while (stream_read_line(stream, line))
        {
            furi_string_replace_all(line, "\r", "");
            furi_string_replace_all(line, "\n", "");
            FURI_LOG_I(TAG, "%s", furi_string_get_cstr(line));
        }
    }
    else
    {
        FURI_LOG_E(TAG, "Failed to open file");
    }
    FURI_LOG_I(TAG, "----------------------------------------");

    furi_string_free(line);
    file_stream_close(stream);
    stream_free(stream);
}

// Application entry point
int32_t example_apps_assets_main(void *p)
{
    // Mark argument as unused
    UNUSED(p);

    // Open storage
    Storage *storage = furi_record_open(RECORD_STORAGE);

    example_apps_data_print_file_content(storage, APP_ASSETS_PATH("test_asset.txt"));
    example_apps_data_print_file_content(storage, APP_ASSETS_PATH("poems/a jelly-fish.txt"));
    example_apps_data_print_file_content(storage, APP_ASSETS_PATH("poems/theme in yellow.txt"));
    example_apps_data_print_file_content(storage, APP_ASSETS_PATH("poems/my shadow.txt"));

    // Close storage
    furi_record_close(RECORD_STORAGE);

    return 0;
}

void button_config_start(ButtonConfig *button_config, FuriString *path)
{
    FURI_LOG_D("chip8", "loading config file '%s'", furi_string_get_cstr(path));

    Storage *storage = furi_record_open(RECORD_STORAGE);
    Stream *stream = file_stream_alloc(storage);

    uint8_t c[2];
    furi_check(
        file_stream_open(stream, furi_string_get_cstr(path), FSAM_READ, FSOM_OPEN_EXISTING));

    for (size_t addr = 0; !stream_eof(stream); addr++)
    {
        if (stream_read(stream, c, 2) == 2)
        {
            const uint16_t opcode = ((uint16_t)(c[0]) << 8) | (uint16_t)(c[1]);
            const uint8_t x = (opcode & 0x0F00) >> 8;
            switch (opcode & 0xF0FF)
            {
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