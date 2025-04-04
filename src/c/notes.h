#ifndef NOTES_H
#define NOTES_H

#include <pebble.h>

/* The `Note` struct will be heavily enforced to be exactly 512 bytes at a given point,
 * hence a note can be 500 characters long at any given point.
*/
#define MAX_NOTE_LENGTH     500
#define KEY_NOTES_STORED_LENGTH    0xABCD
#define KEY_NOTES_LENGTH           0xBEEF

typedef struct __attribute__((packed)) Note {
    bool text_overflow;
    uint8_t index;
    char* note_text;
    uint16_t note_length;

    time_t reminder_time;
} Note;

typedef struct __attribute__((packed)) NotesData {
    int count;
    Note** notes;
} NotesData;

typedef struct __attribute__((packed)) NoteView {
    NotesData *data;
    Note *note;
    char* alt_text;

    Window *window;
    ScrollLayer *scroll_layer;
    TextLayer *text_layer;
    ActionBarLayer *action_layer;
    StatusBarLayer *status_layer;

    GBitmap *delete_icon;
    GBitmap *up_icon;
    GBitmap *down_icon;
} NoteView;

typedef struct __attribute__((packed)) NotesAppState {
    bool notes_loaded;
    int appmessage_in_size;
    int appmessage_out_size;

    DictationSession *dictation;
    NotesData *notes;
    NoteView *note_window;
} NotesAppState;

typedef void(*NoteCallback)(Note* note);

extern Note* note_create(char *text, int length, time_t time, int index, bool text_overflow);
extern void note_edit_text(Note *note, const char* text);

extern NotesData* notes_data_create();
extern void notes_data_destroy(NotesData* data);

extern int notes_data_get_count(NotesData *data);
extern void notes_data_add_note(NotesData *data, char *text, time_t time, bool text_overflow);
extern void notes_data_add_full_note(NotesData *data, Note *note);
extern void notes_data_remove_note(NotesData *data, int index);
extern Note* notes_data_get_note(NotesData *data, int index);
extern bool notes_data_note_exists(NotesData *data, int index);

extern void notes_data_for_each_note(NotesData *data, NoteCallback callback);

// App Message stuff
extern void app_message_init(NotesAppState *state, uint32_t in_size, uint32_t out_size);
extern bool app_message_connected();

// Note views
extern NoteView *note_view_create(Note *note, NotesData *data);
extern void note_view_destroy(NoteView *view);

// Storage
extern uint32_t storage_get_num_notes();
extern void storage_set_num_notes(uint32_t num_notes);

extern uint32_t storage_get_num_notes_stored();
extern void storage_set_num_notes_stored(uint32_t num_notes);

extern bool storage_store_note_on_watch(Note *note);
extern void storage_get_notes_from_watch(NotesData *data);

extern void storage_store_note_on_phone(Note *note);
extern void storage_get_note_from_phone(int index);
extern void storage_delete_note_from_watch(int index);

#endif // NOTES_H