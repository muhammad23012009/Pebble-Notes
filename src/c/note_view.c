#include "notes.h"

static void prv_select_handler(ClickRecognizerRef ref, void *context)
{
    NoteView *view = (NoteView*) context;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Testing select handler, %p, %p, %d", view, view->note, view->note->index);
    notes_data_remove_note(view->data, view->note->index);
    window_stack_pop(true);
}

static void prv_click_config_provider(void *context)
{
    window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_handler);
}

static void prv_window_load(Window *window)
{
    NoteView *view = window_get_user_data(window);

    GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    Layer *window_layer = window_get_root_layer(window);
    GRect window_bounds = layer_get_bounds(window_layer);

    view->status_layer = status_bar_layer_create();
    status_bar_layer_set_colors(view->status_layer, GColorClear, GColorClear);
    GRect status_bounds = layer_get_bounds(status_bar_layer_get_layer(view->status_layer));

    GRect bounds = GRect(0, status_bounds.size.h, window_bounds.size.w, window_bounds.size.h - status_bounds.size.h);
    view->scroll_layer = scroll_layer_create(bounds);

    GRect text_bounds = GRect(3, 0, bounds.size.w - 30, bounds.size.h);
    GSize text_size = graphics_text_layout_get_content_size(view->note->note_text, font, text_bounds, GTextOverflowModeWordWrap, GTextAlignmentLeft);

    view->text_layer = text_layer_create(text_bounds);
    text_layer_set_text(view->text_layer, view->note->note_text);
    text_layer_set_font(view->text_layer, font);
    text_layer_set_overflow_mode(view->text_layer, GTextOverflowModeWordWrap);
    scroll_layer_set_content_size(view->scroll_layer, text_size);

    layer_add_child(scroll_layer_get_layer(view->scroll_layer), text_layer_get_layer(view->text_layer));

    view->action_layer = action_bar_layer_create();
    action_bar_layer_set_context(view->action_layer, view);
    action_bar_layer_set_click_config_provider(view->action_layer, prv_click_config_provider);

    view->delete_icon = gbitmap_create_with_resource(RESOURCE_ID_DELETE_ICON);
    action_bar_layer_set_icon(view->action_layer, BUTTON_ID_SELECT, view->delete_icon);

    layer_add_child(window_layer, scroll_layer_get_layer(view->scroll_layer));
    action_bar_layer_add_to_window(view->action_layer, window);
    layer_add_child(window_layer, status_bar_layer_get_layer(view->status_layer));
}

static void prv_window_unload(Window *window)
{
    NoteView *view = window_get_user_data(window);
    text_layer_destroy(view->text_layer);
    scroll_layer_destroy(view->scroll_layer);
    status_bar_layer_destroy(view->status_layer);
    action_bar_layer_destroy(view->action_layer);

    gbitmap_destroy(view->delete_icon);

    free(view);
}

NoteView *note_view_create(Note *note, NotesData *data)
{
    NoteView *view = malloc(sizeof(NoteView));
    view->note = note;
    view->data = data;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "meow meow meow %p", view->note);
    view->window = window_create();

    window_set_user_data(view->window, view);
    window_set_window_handlers(view->window, (WindowHandlers) {
        .load = prv_window_load,
        .unload = prv_window_unload
    });

    return view;
}