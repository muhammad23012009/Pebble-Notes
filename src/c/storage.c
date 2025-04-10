#include <pebble.h>
#include "notes.h"

#define MAX_STRING_LENGTH   249
#define MAX_STORAGE_SIZE    4080
#define BUFFER_SIZE(x)      ((sizeof(uint8_t) * 2) + sizeof(time_t) + sizeof(bool) + x)

uint16_t s_data_size = 0;

uint8_t *prv_create_buffer(int index, int length, time_t time, char* text, bool text_overflow)
{
    int buffer_length = BUFFER_SIZE(length);
    uint8_t *buffer = malloc(buffer_length);
    memcpy(buffer, &index, 1);
    memcpy(buffer + 1, &length, 1);
    memcpy(buffer + 2, &text_overflow, 1);
    memcpy(buffer + 3, &time, 4);
    memcpy(buffer + 7, text, length);

    return buffer;
}

// This function returns the actual number of notes stored in persistent storage
uint32_t storage_get_num_notes_stored()
{
    return persist_read_int(KEY_NOTES_STORED_LENGTH);
}

void storage_set_num_notes_stored(uint32_t num_notes)
{
    if (num_notes == storage_get_num_notes_stored())
        return;

    persist_write_int(KEY_NOTES_STORED_LENGTH, num_notes);
}

// This function returns the expected number of notes
uint32_t storage_get_num_notes()
{
    return persist_read_int(KEY_NOTES_LENGTH);
}

void storage_set_num_notes(uint32_t num_notes)
{
    if (num_notes == storage_get_num_notes())
        return;
    
    persist_write_int(KEY_NOTES_LENGTH, num_notes);
}

uint32_t storage_get_notes_signature()
{
    return persist_read_int(KEY_NOTES_SIGNATURE);
}

void storage_set_notes_signature(uint32_t signature)
{
    persist_write_int(KEY_NOTES_SIGNATURE, signature);
}

bool storage_store_note_on_watch(Note *note)
{
    bool text_overflow = note->note_length > MAX_STRING_LENGTH;
    uint8_t note_length = text_overflow ? MAX_STRING_LENGTH : note->note_length;
    uint16_t buffer_size = BUFFER_SIZE(note_length);

    if (persist_exists(note->index) && persist_get_size(note->index) == buffer_size) {
        s_data_size += buffer_size;
        return true;
    }

    if (buffer_size + s_data_size > MAX_STORAGE_SIZE)
        return false;

    uint8_t *buffer = prv_create_buffer(note->index, note_length, note->reminder_time, note->note_text, text_overflow);

    int written_bytes = persist_write_data(note->index, buffer, buffer_size);
    if (written_bytes != buffer_size)
        return false;

    s_data_size += buffer_size;

    free(buffer);
    buffer = NULL;

    return true;
}

void storage_get_notes_from_watch(NotesData *data)
{
    for (uint32_t i = 0; i < storage_get_num_notes_stored(); i++) {
        uint8_t *info = malloc(persist_get_size(i));
        persist_read_data(i, info, persist_get_size(i));

        uint8_t index = *info;
        uint8_t text_length = *(info + 1);
        bool text_overflow = *(info + 2);
        time_t reminder_time = 0;
        memcpy(&reminder_time, info + 3, 4);
        char text[text_length + 1];
        strncpy(text, (const char*)(info + 7), text_length);

        Note* note = note_create(text, text_length, reminder_time, index, text_overflow);
        notes_data_add_full_note(data, note);

        free(info);
        info = NULL;
        note = NULL;
    }
}

void storage_delete_note_from_watch(int index)
{
    persist_delete(index);
}

void storage_delete_note_from_phone(int index)
{
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_uint8(iter, MESSAGE_KEY_NOTES_DELETE, index);
    dict_write_end(iter);

    app_message_outbox_send();
}

void storage_store_note_on_phone(Note *note)
{
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_uint8(iter, MESSAGE_KEY_NOTES_STORE_INDEX, note->index);
    dict_write_uint16(iter, MESSAGE_KEY_NOTES_STORE_LENGTH, note->note_length);
    dict_write_uint32(iter, MESSAGE_KEY_NOTES_STORE_TIME, note->reminder_time);
    dict_write_cstring(iter, MESSAGE_KEY_NOTES_STORE_TEXT, note->note_text);
    dict_write_end(iter);

    app_message_outbox_send();
}

void storage_get_notes_from_phone()
{
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_int8(iter, MESSAGE_KEY_NOTES_LOAD, -1);
}

void storage_get_note_from_phone(int index)
{
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    dict_write_int8(iter, MESSAGE_KEY_NOTES_LOAD, index);
    dict_write_end(iter);

    app_message_outbox_send();
}