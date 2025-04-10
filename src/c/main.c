#include <pebble.h>
#include "notes.h"

static Window *s_window;
static StatusBarLayer *s_status_bar;
static NotesAppState *s_state;

// TODO: somehow minimize the heap_bytes_free() calls we do
// TODO: only load full note when opened, otherwise only fetch the bare minimum notes for menu layer
// TODO: Display a message that tells user to add notes from phone for aplite
// TODO: Use notes_data_insert_note() when we receive notes from the phone too
// TODO: add a function for moving two notes

#if PBL_MICROPHONE
static void prv_dictation_callback(DictationSession *session, DictationSessionStatus status,
                                  char* transcription, void *context)
{
    NotesAppState *state = (NotesAppState*) context;

    if (status == DictationSessionStatusSuccess) {
        notes_data_insert_note(state->notes, transcription, time(NULL), 0);
        storage_store_note_on_phone(state->notes->notes[state->notes->count - 1]);
        menu_layer_reload_data(state->menu_layer);
    }
}
#endif

static void prv_select_click_handler(MenuLayer *layer, MenuIndex *cell_index, void *context) {
    NotesAppState *state = (NotesAppState*) context;

    if (cell_index->row == notes_data_get_count(state->notes) + 1)
        return;

#if PBL_MICROPHONE
    if (cell_index->row == 0) {
        if (heap_bytes_free() < MIN_HEAP_SPACE) {
            return;
        }
        dictation_session_start(state->dictation);

    } else {
#endif
        state->note_window = note_view_create(notes_data_get_note(state->notes, cell_index->row - PBL_IF_MICROPHONE_ELSE(1, 0)), state->notes);
        window_stack_push(state->note_window->window, true);
#if PBL_MICROPHONE
    }
#endif
}

static void prv_menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context)
{
    NotesAppState *state = (NotesAppState*) callback_context;

#if PBL_MICROPHONE
    if (cell_index->row == 0) {
        // Figure out if we have enough heap space
        if (heap_bytes_free() < MIN_HEAP_SPACE) {
            graphics_draw_text(ctx, "x", fonts_get_system_font(FONT_KEY_GOTHIC_28), layer_get_bounds(cell_layer),
                               GTextOverflowModeFill, GTextAlignmentCenter, NULL);
        } else {
            graphics_draw_text(ctx, "+", fonts_get_system_font(FONT_KEY_GOTHIC_28), layer_get_bounds(cell_layer),
                               GTextOverflowModeFill, GTextAlignmentCenter, NULL);
        }

        return;
    } else {
#endif
        if (cell_index->row == notes_data_get_count(state->notes) + 1) {
            graphics_draw_text(ctx, "Connect to your phone to view more notes!", fonts_get_system_font(FONT_KEY_GOTHIC_24), 
                                layer_get_bounds(cell_layer), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
            return;
        } else if (notes_data_get_count(state->notes) > 0) {
            Note *note = notes_data_get_note(state->notes, (int)cell_index->row - PBL_IF_MICROPHONE_ELSE(1, 0));

            time_t current_time_t = time(NULL);
            struct tm* note_time = localtime(&note->reminder_time);
            //struct tm* current_time = localtime(&current_time_t);

            // TODO: Add support for indicating day too
            char time_string[6];
            snprintf(time_string, 6, "%02d:%02d", note_time->tm_hour, note_time->tm_min);

            time_string[5] = '\0';
            menu_cell_basic_draw(ctx, cell_layer, note->note_text, time_string, NULL);
        }
#if PBL_MICROPHONE
    }
#endif
}

static uint16_t prv_menu_num_rows(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context)
{
    NotesAppState *state = (NotesAppState*) callback_context;

    if (storage_get_num_notes() > storage_get_num_notes_stored() && !app_message_connected())
        return notes_data_get_count(state->notes) + PBL_IF_MICROPHONE_ELSE(2, 1);
    else
        return notes_data_get_count(state->notes) + PBL_IF_MICROPHONE_ELSE(1, 0);
}

static int16_t prv_menu_cell_height(MenuLayer *menu_layer, MenuIndex *index, void *context)
{
    NotesAppState *state = (NotesAppState *) context;

    if (storage_get_num_notes() > storage_get_num_notes_stored() && 
        index->row == notes_data_get_count(state->notes) + 1)
    {
        return 76;
    }

    return 44;
}

static void prv_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect window_bounds = layer_get_bounds(window_layer);
    GRect menu_bounds = layer_get_bounds(status_bar_layer_get_layer(s_status_bar));

    GRect bounds = GRect(0, menu_bounds.size.h, window_bounds.size.w, window_bounds.size.h);
    s_state->menu_layer = menu_layer_create(bounds);
    menu_layer_set_callbacks(s_state->menu_layer, s_state, (MenuLayerCallbacks) {
        .draw_row = prv_menu_draw_row,
        .get_num_rows = prv_menu_num_rows,
        .select_click = prv_select_click_handler,
        .get_cell_height = prv_menu_cell_height
    });
    menu_layer_set_click_config_onto_window(s_state->menu_layer, window);
    menu_layer_set_normal_colors(s_state->menu_layer, GColorBlack, GColorWhite);
    menu_layer_set_highlight_colors(s_state->menu_layer,
                                    PBL_IF_COLOR_ELSE(GColorTiffanyBlue, GColorWhite),
                                    GColorBlack); 

    layer_add_child(window_layer, menu_layer_get_layer(s_state->menu_layer));
    layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar));
}

static void prv_window_unload(Window *window) {
    menu_layer_destroy(s_state->menu_layer);

    // Before destroying all our notes, store them on the watch
    int written_notes = 0;
    for (int i = 0; i < notes_data_get_count(s_state->notes); i++) {
        bool written = storage_store_note_on_watch(notes_data_get_note(s_state->notes, i));
        if (!written)
            break;
        
        written_notes++;
    }
    storage_set_num_notes_stored(written_notes);

    uint32_t signature = notes_data_signature(s_state->notes);
    // Only change number of notes if we're connected to the phone
    if (signature == storage_get_notes_signature()
        && notes_data_get_count(s_state->notes) == (int)storage_get_num_notes()
        && !connection_service_peek_pebble_app_connection() && !app_message_connected())
        return;

    storage_set_num_notes(notes_data_get_count(s_state->notes));
    storage_set_notes_signature(signature);
}

static void prv_init(void) {
    s_window = window_create();
    s_status_bar = status_bar_layer_create();
    status_bar_layer_set_colors(s_status_bar, GColorBlack, GColorWhite);
    s_state = malloc(sizeof(NotesAppState));
    s_state->notes = notes_data_create();

#if PBL_MICROPHONE
    s_state->dictation = dictation_session_create(MAX_NOTE_LENGTH, prv_dictation_callback, s_state);
#endif
    // Load notes from persistent storage
    storage_get_notes_from_watch(s_state->notes);

    app_message_init(s_state, 560, 560);

    window_set_window_handlers(s_window, (WindowHandlers) {
        .load = prv_window_load,
        .unload = prv_window_unload,
    });
    window_stack_push(s_window, true);
}

static void prv_deinit(void) {
    app_message_deregister_callbacks();
    window_destroy(s_window);
#if PBL_MICROPHONE
    dictation_session_destroy(s_state->dictation);
#endif

    status_bar_layer_destroy(s_status_bar);
    notes_data_destroy(s_state->notes);
    free(s_state);
    s_state = NULL;
}

int main(void) {
  prv_init();

  app_event_loop();
  prv_deinit();
}
