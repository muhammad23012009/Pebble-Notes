#include <pebble.h>
#include "notes.h"

static Window *s_window;
static MenuLayer *s_menu_layer;
static StatusBarLayer *s_status_bar;
static NotesAppState *s_state;

static GBitmap *s_add_icon;

static void prv_dictation_callback(DictationSession *session, DictationSessionStatus status,
                                  char* transcription, void *context)
{
    NotesAppState *state = (NotesAppState*) context;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Heap space before adding note is %d", heap_bytes_free());
    notes_data_add_note(state->notes, transcription);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Heap space after adding note is %d", heap_bytes_free());
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Returned from adding note");
    menu_layer_reload_data(s_menu_layer);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Menu layer reloaded!");
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Heap space after reload is %d", heap_bytes_free());
}

static void prv_select_click_handler(MenuLayer *layer, MenuIndex *cell_index, void *context) {
    NotesAppState *state = (NotesAppState*) context;

    if (cell_index->row == 0)
        dictation_session_start(state->dictation);
    else {
        state->note_window = note_view_create(notes_data_get_note(state->notes, cell_index->row - 1), state->notes);
        window_stack_push(state->note_window->window, true);
    }
}

static void prv_menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context)
{
    NotesAppState *state = (NotesAppState*) callback_context;

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Draw row called!");
    if (cell_index->row == 0) {
        // This is the "Add note" row
        GRect box;

        GRect bounds = layer_get_bounds(cell_layer);
        GRect icon_bounds = gbitmap_get_bounds(s_add_icon);
        box.origin = GPoint((bounds.size.w - icon_bounds.size.w) / 2,
                            (bounds.size.h - icon_bounds.size.h) / 2);
        box.size = icon_bounds.size;
        graphics_context_set_compositing_mode(ctx, GCompOpSet);
        graphics_draw_bitmap_in_rect(ctx, s_add_icon, box);
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Heap space after drawing row 0 is %d", heap_bytes_free());
        return;
    } else {
        if (notes_data_get_count(state->notes) > 0) {
            APP_LOG(APP_LOG_LEVEL_DEBUG, "Have some notes!");
            Note *note = notes_data_get_note(state->notes, (int)cell_index->row - 1);

            char text[22];
            strncpy(text, note->note_text, 22);
            APP_LOG(APP_LOG_LEVEL_DEBUG, "meow %s", text);
            menu_cell_basic_draw(ctx, cell_layer, text, NULL, NULL);
        }
    }

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Heap space after drawing row is %d", heap_bytes_free());
}

static uint16_t prv_menu_num_rows(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context)
{
    NotesAppState *state = (NotesAppState*) callback_context;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "NUM ROWS CALLED %d", notes_data_get_count(state->notes));
    return notes_data_get_count(state->notes) + 1;
}

static int16_t prv_menu_cell_height(MenuLayer *menu_layer, MenuIndex *index, void *context)
{
    return 44;
}

static void prv_window_load(Window *window) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Window load called!");
    Layer *window_layer = window_get_root_layer(window);
    GRect window_bounds = layer_get_bounds(window_layer);
    GRect menu_bounds = layer_get_bounds(status_bar_layer_get_layer(s_status_bar));

    GRect bounds = GRect(0, menu_bounds.size.h, window_bounds.size.w, window_bounds.size.h - menu_bounds.size.h);
    s_menu_layer = menu_layer_create(bounds);
    menu_layer_set_callbacks(s_menu_layer, s_state, (MenuLayerCallbacks) {
        .draw_row = prv_menu_draw_row,
        .get_num_rows = prv_menu_num_rows,
        .select_click = prv_select_click_handler,
        .get_cell_height = prv_menu_cell_height
    });
    menu_layer_set_click_config_onto_window(s_menu_layer, window);
    menu_layer_set_highlight_colors(s_menu_layer, GColorBabyBlueEyes, GColorBlack); 

    layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
    layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar));
}

static void prv_window_unload(Window *window) {
    menu_layer_destroy(s_menu_layer);
}

static void prv_init(void) {
    s_window = window_create();
    s_status_bar = status_bar_layer_create();
    status_bar_layer_set_colors(s_status_bar, GColorClear, GColorClear);
    s_state = malloc(sizeof(NotesAppState));
    s_state->notes = notes_data_create();
    s_state->dictation = dictation_session_create(MAX_NOTE_LENGTH, prv_dictation_callback, s_state);

    s_add_icon = gbitmap_create_with_resource(RESOURCE_ID_PLUS_ICON);

    //storage_set_num_notes(0);
    // Load notes from persistent storage
    storage_get_notes_from_watch(s_state->notes);

    app_message_init(s_state, 512, 512);

    window_set_window_handlers(s_window, (WindowHandlers) {
        .load = prv_window_load,
        .unload = prv_window_unload,
    });
    const bool animated = true;
    window_stack_push(s_window, animated);
}

static void prv_deinit(void) {
    app_message_deregister_callbacks();
    window_destroy(s_window);
    dictation_session_destroy(s_state->dictation);
    status_bar_layer_destroy(s_status_bar);
    gbitmap_destroy(s_add_icon);

    // Before destroying all our notes, store them on the watch
    storage_set_num_notes(s_state->notes->count);
    notes_data_for_each_note(s_state->notes, storage_store_note_on_watch);

    notes_data_destroy(s_state->notes);
    free(s_state);
}

int main(void) {
  prv_init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

  app_event_loop();
  prv_deinit();
}
