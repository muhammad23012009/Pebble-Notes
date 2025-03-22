#include <pebble.h>
#include "notes.h"

void *prv_realloc(Note** ptr, int size, int index)
{
    if (index < size) {
        for (int i = index; i <= size; i++) {
            ptr[i] = ptr[i+1];
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
    note = NULL;

    APP_LOG(APP_LOG_LEVEL_DEBUG, "what about now %p", note);
}

void prv_decrement_index(Note *note)
{
    note->index = (note->index == 0) ? 0 : note->index - 1;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Decremented index, it is now %d", note->index);
}

Note *note_create(char* text, int length, int index)
{
    Note *note = (Note*) malloc(sizeof(Note));
    note->note_length = length;
    note->index = index;
    note->note_text = malloc(length);
    strncpy(note->note_text, text, length);
    note->note_text[length] = '\0';

    return note;
}

int notes_data_get_count(NotesData *data)
{
    return data ? data->count : 0;
}

void notes_data_add_note(NotesData *data, char* text)
{
    int note_length = strlen(text);
    note_length = note_length > MAX_NOTE_LENGTH ? MAX_NOTE_LENGTH : note_length;

    data->notes = (Note**) realloc(data->notes, (data->count + 1) * sizeof(Note*));
    data->notes[data->count] = note_create(text, note_length, data->count);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Note added!, testing it: %s, %d, %d", data->notes[data->count]->note_text, data->notes[data->count]->note_length, data->notes[data->count]->index);
    data->count++;
}

void notes_data_add_full_note(NotesData *data, Note *note)
{
    data->notes = (Note**) realloc(data->notes, (data->count + 1) * sizeof(Note*));
    data->notes[data->count] = note_create(note->note_text, note->note_length, note->index);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Note added v2!, testing it: %s, %d, %d", data->notes[data->count]->note_text, data->notes[data->count]->note_length, data->notes[data->count]->index);
    data->count++;

    free(note->note_text);
    free(note);
    note = NULL;
}

void notes_data_remove_note(NotesData *data, int index)
{
    if (data->count == 0 || index < 0)
        return;

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Destroying a note! %d %d", index, data->count);
    int count = data->count;
    Note *note = data->notes[index];
    prv_destroy_note(note);
    data->notes = (Note**) prv_realloc(data->notes, data->count - 1, index);
    data->count--;
    if (index < count)
        notes_data_for_each_note(data, prv_decrement_index);
}

Note *notes_data_get_note(NotesData *data, int index)
{
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Fetching a note at index %d, %p", index, data->notes[index]);
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

    data = NULL;
}
