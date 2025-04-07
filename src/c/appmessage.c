#include "notes.h"

static bool s_connected = false;
static int s_note_index;

// TODO: subscribe to connection messages

static void prv_app_message_handle_incoming(DictionaryIterator *iter, void *context)
{
    NotesAppState *state = (NotesAppState*) context;

    Tuple *ready_tuple = dict_find(iter, MESSAGE_KEY_READY);
    if (ready_tuple) {
        s_connected = true;

        // TODO: do this with a timer to make it async
        // Begin note refreshing
        for (int i = 0; i < notes_data_get_count(state->notes); i++) {
            Note *note = notes_data_get_note(state->notes, i);
            if (note->text_overflow) {
                s_note_index = i;
                s_note_index++;
                storage_get_note_from_phone(i);
                note = NULL;
                break;
            }
            note = NULL;
        }
        return;
    }

    Tuple *command_tuple = dict_find(iter, MESSAGE_KEY_NOTES_LOAD);
    if (command_tuple) {
        Tuple *index_tuple = dict_find(iter, MESSAGE_KEY_NOTES_LOAD_INDEX);
        Tuple *time_tuple = dict_find(iter, MESSAGE_KEY_NOTES_LOAD_TIME);
        Tuple *string_tuple = dict_find(iter, MESSAGE_KEY_NOTES_LOAD_TEXT);

        if (notes_data_note_exists(state->notes, index_tuple->value->uint8)) {
            note_edit_text(notes_data_get_note(state->notes, index_tuple->value->uint8), string_tuple->value->cstring);
            state->notes->notes[index_tuple->value->uint8]->text_overflow = false;
        } else {
            Note *note = note_create(string_tuple->value->cstring, string_tuple->length, time_tuple->value->uint32, index_tuple->value->uint8, false);
            notes_data_add_full_note(state->notes, note);
            note = NULL;
        }

        menu_layer_reload_data(state->menu_layer);

        // TODO: do this with a timer to make it async
        if (s_note_index != -1) {
            for (int i = s_note_index; i < notes_data_get_count(state->notes); i++) {
                Note *note = notes_data_get_note(state->notes, i);
                if (note->text_overflow) {
                    s_note_index = i;
                    s_note_index++;

                    storage_get_note_from_phone(i);
                    note = NULL;
                    break;
                }
                note = NULL;
            }
        }
    }
}

static void prv_app_message_handle_outgoing(DictionaryIterator *iter, void *context)
{
}

static void prv_app_message_handle_outgoing_failed(DictionaryIterator *iter, AppMessageResult status, void *context)
{
}

void app_message_init(NotesAppState *state, uint32_t in_size, uint32_t out_size)
{
    state->appmessage_in_size = in_size;
    state->appmessage_out_size = out_size;

    app_message_register_inbox_received(prv_app_message_handle_incoming);
    app_message_register_outbox_sent(prv_app_message_handle_outgoing);
    app_message_register_outbox_failed(prv_app_message_handle_outgoing_failed);
    app_message_set_context(state);

    app_message_open(in_size, out_size);
}

bool app_message_connected()
{
    return s_connected;
}