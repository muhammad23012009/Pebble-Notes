#include <pebble.h>
#include "notes.h"

#define MAX_STRING_LENGTH   246

uint32_t storage_get_num_notes()
{
    return persist_read_int(KEY_NOTES_LENGTH);
}

void storage_set_num_notes(uint32_t num_notes)
{
    persist_write_int(KEY_NOTES_LENGTH, num_notes);
}

void storage_store_note_on_watch(Note *note)
{
    //info.reminder_time = note->reminder_time;

    uint8_t note_length = note->note_length > MAX_STRING_LENGTH ? MAX_STRING_LENGTH : note->note_length;
    uint8_t *buffer = malloc((sizeof(uint8_t) * 2) + sizeof(time_t) + note_length);
    memcpy(buffer, &note->index, 1);
    memcpy(buffer + 1, &note_length, 1);
    memcpy(buffer + 2, note->note_text, note_length);
    persist_write_data(note->index, buffer, (sizeof(uint8_t) * 2) + sizeof(time_t) + note_length);

    free(buffer);
}

void storage_get_notes_from_watch(NotesData *data)
{
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Testing! %d", storage_get_num_notes());
    for (uint32_t i = 0; i < storage_get_num_notes(); i++) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Actual size is %d!", persist_get_size(i));
        uint8_t *info = malloc(persist_get_size(i));
        persist_read_data(i, info, persist_get_size(i));

        uint8_t index = *info;
        uint8_t text_length = *(info + 1);
        char text[text_length + 1];
        strncpy(text, (const char*)(info + 2), text_length);
        APP_LOG(APP_LOG_LEVEL_DEBUG, "We managed to read all buffers! %s %d %d", text, index, text_length);

        Note* note = note_create(text, text_length, index);
        note->reminder_time = 0; //info->reminder_time;
        notes_data_add_full_note(data, note);

        free(info);
    }
}