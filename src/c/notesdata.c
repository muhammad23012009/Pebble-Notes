#include <pebble.h>
#include "notes.h"

void prv_decrement_index(Note *note)
{
    if (note) {
        note->index = (note->index == 0) ? 0 : note->index - 1;
    }
}

void *prv_realloc(Note** ptr, int size, int index)
{
    if (index < size) {
        for (int i = index; i < size; i++) {
            ptr[i] = ptr[i+1];
            prv_decrement_index(ptr[i]);
        }
    }

    void* nptr = realloc(ptr, sizeof(void*) * size);
    return nptr;
}

uint32_t prv_hash(const void *buffer, size_t len)
{
    const uint8_t* bytes = (const uint8_t*)buffer;
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < len; i++) {
        hash ^= bytes[i];
        hash *= 16777619;
    }
    return hash;
}

void notes_data_for_each_note(NotesData *data, NoteCallback callback)
{
    for (int i = 0; i < data->count; i++) {
        callback(data->notes[i]);
    }
}

void prv_destroy_note(Note *note)
{
    free(note->note_text);
    free(note);

    note->note_text = NULL;
    note = NULL;
}

Note *note_create(char* text, int length, time_t time, int index, bool text_overflow)
{
    Note *note = (Note*) malloc(sizeof(Note));
    note->note_length = length;
    note->index = index;
    note->reminder_time = time;
    note->text_overflow = text_overflow;
    note->note_text = malloc(length + 1);
    strncpy(note->note_text, text, length);
    note->note_text[length] = '\0';

    return note;
}

void note_edit_text(Note *note, const char* text)
{
    int text_length = strlen(text);
    if (text_length == note->note_length)
        return;

    note->note_text = realloc(note->note_text, text_length + 1);
    strncpy(note->note_text, text, text_length);
    note->note_text[text_length] = '\0';
    note->note_length = text_length;
}

int notes_data_get_count(NotesData *data)
{
    return data ? data->count : 0;
}

void notes_data_add_note(NotesData *data, char* text, time_t time, bool text_overflow)
{
    int note_length = strlen(text);
    note_length = note_length > MAX_NOTE_LENGTH ? MAX_NOTE_LENGTH : note_length;

    if (!data->notes)
        data->notes = malloc(sizeof(Note*));
    else
        data->notes = (Note**) realloc(data->notes, (data->count + 1) * sizeof(Note*));

    data->notes[data->count] = note_create(text, note_length, time, data->count, text_overflow);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Note added!, testing it: %s, %d, %d", data->notes[data->count]->note_text, data->notes[data->count]->note_length, data->notes[data->count]->index);
    data->count++;
}

void notes_data_add_full_note(NotesData *data, Note *note)
{
    bool exists = notes_data_note_exists(data, note->index);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Does the note exist? %d", exists);
    if (!exists) {
        if (!data->notes)
            data->notes = malloc(sizeof(Note*));
        else
            data->notes = (Note**) realloc(data->notes, (data->count + 1) * sizeof(Note*));
    }

    data->notes[data->count] = note_create(note->note_text, note->note_length, note->reminder_time, note->index, note->text_overflow);
    if (!exists)
        data->count++;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Testing a note! %s, %d, %d", note->note_text, note->index, note->reminder_time);

    prv_destroy_note(note);
}

void notes_data_remove_note(NotesData *data, int index)
{
    if (data->count == 0 || index < 0)
        return;

    Note *note = data->notes[index];
    prv_destroy_note(note);

    if (!(data->count - 1)) {
        free(data->notes);
        data->notes = NULL;
    } else {
        data->notes = (Note**) prv_realloc(data->notes, data->count - 1, index);
    }

    data->count--;
}

Note *notes_data_get_note(NotesData *data, int index)
{
    return notes_data_note_exists(data, index) ? data->notes[index] : NULL;
}

bool notes_data_note_exists(NotesData *data, int index)
{
    if (index + 1 > data->count)
        return false;

    return data->notes[index] != NULL;
}

uint32_t notes_data_signature(NotesData *data)
{
    // 4 byte signature of the entire NotesData structure
    // Useful for figuring out if the structure changed
    uint32_t hash = prv_hash(&data->count, sizeof(data->count));

    for (int i = 0; i < data->count; i++) {
        Note* note = data->notes[i];
        if (!note) continue;

        // Hash fixed-size portion before note_text
        hash ^= prv_hash(note, offsetof(Note, note_text));

        // Hash only first few bytes of note_text
        if (note->note_text && note->note_length > 0) {
            size_t sample_len = note->note_length < 4
                              ? note->note_length
                              : 4;
            hash ^= prv_hash(note->note_text, sample_len);
        }

        // Add length and reminder_time for better uniqueness
        hash ^= prv_hash(&note->note_length, sizeof(note->note_length));
        hash ^= prv_hash(&note->reminder_time, sizeof(note->reminder_time));
    }

    return hash;
}

NotesData* notes_data_create()
{
    NotesData *data = malloc(sizeof(NotesData));
    data->count = 0;
    data->notes = (Note**) malloc(sizeof(Note*));

    return data;
}

void notes_data_destroy(NotesData *data)
{
    notes_data_for_each_note(data, prv_destroy_note);
    data->count = 0;
    free(data->notes);
    free(data);

    data->notes = NULL;
    data = NULL;
}
