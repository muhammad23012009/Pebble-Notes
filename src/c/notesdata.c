#include <pebble.h>
#include "notes.h"

void prv_decrement_index(Note *note)
{
    if (note) {
        note->index = (note->index == 0) ? 0 : note->index - 1;
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Decremented index, it is now %d", note->index);
    }
}

void *prv_realloc(Note** ptr, int size, int index)
{
    APP_LOG(APP_LOG_LEVEL_DEBUG, "prv_realloc called! %d %d", size, index);
    if (index < size) {
        for (int i = index; i < size; i++) {
            APP_LOG(APP_LOG_LEVEL_DEBUG, "iterator is %d", i);
            ptr[i] = ptr[i+1];
            prv_decrement_index(ptr[i]);
        }
    }

    void* nptr = realloc(ptr, sizeof(void*) * size);
    return nptr;
}

void notes_data_for_each_note(NotesData *data, NoteCallback callback)
{
    for (int i = 0; i < data->count; i++) {
        callback(data->notes[i]);
    }
}

void prv_destroy_note(Note *note)
{
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Is it even valid? %p", note);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Destroying a note! %s %d", note->note_text, note->note_length);
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
    data->notes = (Note**) realloc(data->notes, (data->count + 1) * sizeof(Note*));
    data->notes[data->count] = note_create(note->note_text, note->note_length, note->reminder_time, note->index, note->text_overflow);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Note added v2!, testing it: %s, %d, %d", data->notes[data->count]->note_text, data->notes[data->count]->note_length, data->notes[data->count]->index);
    data->count++;

    prv_destroy_note(note);
}

void notes_data_remove_note(NotesData *data, int index)
{
    if (data->count == 0 || index < 0)
        return;

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Destroying a note! %d %d", index, data->count);
    int count = data->count;
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
    return data->notes[index] ? data->notes[index] : NULL;
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
