#include "notes.h"

static void prv_select_handler(ClickRecognizerRef ref, void *context)
{
    NoteView *view = (NoteView*) context;
    storage_delete_note_from_watch(view->note->index);
    storage_delete_note_from_phone(view->note->index);
    notes_data_remove_note(view->data, view->note->index);
    window_stack_pop(true);
}

#if PBL_RECT
static void prv_scroll_handler(ClickRecognizerRef ref, void *context)
{
    NoteView *view = (NoteView*) context;

    GPoint scroll_point = scroll_layer_get_content_offset(view->scroll_layer);
    GSize scroll_size = scroll_layer_get_content_size(view->scroll_layer);

    ButtonId id = click_recognizer_get_button_id(ref);
    if (id == BUTTON_ID_UP) {
        if (!gpoint_equal(&scroll_point, &GPointZero)) {
            if (scroll_point.y > -16)
                scroll_layer_set_content_offset(view->scroll_layer, GPointZero, true);
            else
                scroll_layer_set_content_offset(view->scroll_layer, GPoint(0, scroll_point.y + 16), true);
        }
    } else if (id == BUTTON_ID_DOWN) {
        scroll_layer_set_content_offset(view->scroll_layer, GPoint(0, scroll_point.y - 16), true);
    }
}

static void prv_click_config_provider(void *context)
{
    window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_handler);
    window_single_click_subscribe(BUTTON_ID_UP, prv_scroll_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, prv_scroll_handler);
}

static void prv_create_action_bar(NoteView *view)
{
    view->action_layer = action_bar_layer_create();
    action_bar_layer_set_context(view->action_layer, view);
    action_bar_layer_set_click_config_provider(view->action_layer, prv_click_config_provider);

    view->delete_icon = gbitmap_create_with_resource(RESOURCE_ID_DELETE_ICON);
    view->up_icon = gbitmap_create_with_resource(RESOURCE_ID_UP_ICON);
    view->down_icon = gbitmap_create_with_resource(RESOURCE_ID_DOWN_ICON);

    action_bar_layer_set_icon(view->action_layer, BUTTON_ID_SELECT, view->delete_icon);
    action_bar_layer_set_icon(view->action_layer, BUTTON_ID_UP, view->up_icon);
    action_bar_layer_set_icon(view->action_layer, BUTTON_ID_DOWN, view->down_icon);
}
#endif

static void prv_window_load(Window *window)
{
    NoteView *view = window_get_user_data(window);

    GFont font = fonts_get_system_font(PBL_IF_ROUND_ELSE(FONT_KEY_GOTHIC_18_BOLD, FONT_KEY_GOTHIC_24_BOLD));
    Layer *window_layer = window_get_root_layer(window);
    GRect window_bounds = layer_get_bounds(window_layer);

    view->status_layer = status_bar_layer_create();
    status_bar_layer_set_colors(view->status_layer, GColorClear, GColorClear);
    GRect status_bounds = layer_get_bounds(status_bar_layer_get_layer(view->status_layer));

    GRect bounds = GRect(0, status_bounds.size.h, window_bounds.size.w, window_bounds.size.h - status_bounds.size.h);
    view->scroll_layer = scroll_layer_create(window_bounds);
    scroll_layer_set_shadow_hidden(view->scroll_layer, true);
    scroll_layer_set_frame(view->scroll_layer, bounds);
    scroll_layer_set_context(view->scroll_layer, view);

#if PBL_RECT
    GRect text_bounds = GRect(2, 0, bounds.size.w - ACTION_BAR_WIDTH - 2, 2000);
#else
    GRect text_bounds = GRect(0, 0, bounds.size.w, 2000);
#endif

    view->text_layer = text_layer_create(text_bounds);

    int text_length = strlen(view->note->note_text);

    char overflow_text[] = "...\n\nConnect to your phone to view the remaining note!";
    int note_length = view->note->note_length + strlen(overflow_text);

    if (view->note->text_overflow) {
        view->alt_text = malloc(note_length + 1);
        snprintf(view->alt_text, note_length, "%s%s", view->note->note_text, overflow_text);
        view->alt_text[note_length] = '\0';
        text_layer_set_text(view->text_layer, view->alt_text);
    } else {
        text_layer_set_text(view->text_layer, view->note->note_text);
    }
    
    text_layer_set_font(view->text_layer, font);
    text_layer_set_text_alignment(view->text_layer, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft));
    text_layer_set_overflow_mode(view->text_layer, GTextOverflowModeWordWrap);
    GSize text_size = text_layer_get_content_size(view->text_layer);
    scroll_layer_set_content_size(view->scroll_layer, GSize(text_size.w, text_size.h + PBL_IF_ROUND_ELSE(120, 30)));

    scroll_layer_add_child(view->scroll_layer, text_layer_get_layer(view->text_layer));

    layer_add_child(window_layer, scroll_layer_get_layer(view->scroll_layer));
    layer_add_child(window_layer, status_bar_layer_get_layer(view->status_layer));

#if PBL_RECT
    prv_create_action_bar(view);
    action_bar_layer_add_to_window(view->action_layer, window);
#endif

#if PBL_ROUND
    scroll_layer_set_click_config_onto_window(view->scroll_layer, window);
    text_layer_enable_screen_text_flow_and_paging(view->text_layer, 5);
    scroll_layer_set_paging(view->scroll_layer, true);
#endif
}

static void prv_window_unload(Window *window)
{
    NoteView *view = window_get_user_data(window);
    text_layer_destroy(view->text_layer);
    scroll_layer_destroy(view->scroll_layer);
    status_bar_layer_destroy(view->status_layer);

#if PBL_RECT
    action_bar_layer_destroy(view->action_layer);
    gbitmap_destroy(view->delete_icon);
    gbitmap_destroy(view->up_icon);
    gbitmap_destroy(view->down_icon);
#endif

    if (view->alt_text) {
        free(view->alt_text);
        view->alt_text = NULL;
    }

    free(view);
    view = NULL;
    window_destroy(window);
}

NoteView *note_view_create(Note *note, NotesData *data)
{
    NoteView *view = malloc(sizeof(NoteView));
    view->alt_text = NULL;
    view->note = note;
    view->data = data;
    view->window = window_create();

    window_set_user_data(view->window, view);
    window_set_window_handlers(view->window, (WindowHandlers) {
        .load = prv_window_load,
        .unload = prv_window_unload
    });

    return view;
}