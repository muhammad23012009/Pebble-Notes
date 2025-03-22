#include "notes.h"

static void prv_app_message_handle_incoming(DictionaryIterator *iter, void *context)
{
    NotesAppState *state = (NotesAppState*) context;

    Tuple *data_tuple = dict_find(iter, MESSAGE_KEY_NOTES_LOAD);
    if (data_tuple) {
        const char *data = data_tuple->value->cstring;
        APP_LOG(APP_LOG_LEVEL_DEBUG, "damn a message came through! %s", data);
        notes_data_add_note(state->notes, (char*)data);
    }
}

void app_message_init(NotesAppState *state, uint32_t in_size, uint32_t out_size)
{
    state->appmessage_in_size = in_size;
    state->appmessage_out_size = out_size;

    app_message_register_inbox_received(prv_app_message_handle_incoming);
    app_message_set_context(state);

    app_message_open(in_size, out_size);
}