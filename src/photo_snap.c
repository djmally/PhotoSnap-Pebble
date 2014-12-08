#include <pebble.h>

enum {
    SELECT_KEY = 0,
    UP_KEY     = 1,
    DOWN_KEY   = 2,
    DATA_KEY   = 0
};

static Window *window;
static TextLayer *text_layer;
static bool wasFirstMsg;

static bool send_to_phone(int key_press) {
    if(key_press == -1) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "no data to send");
        return true;
    }

    DictionaryIterator *iter;
    int res = app_message_outbox_begin(&iter);
    if(iter == NULL) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "null iterator");
        return false;
    }

    dict_write_int(iter, DATA_KEY, &key_press, sizeof(key_press), false);
    dict_write_end(iter);

    app_message_outbox_send();
    return true;
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    send_to_phone(SELECT_KEY);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
    send_to_phone(UP_KEY);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    send_to_phone(DOWN_KEY);
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
    Tuple *data_tuple = dict_find(iter, DATA_KEY);

    // Set text layer text to the name we received
    if(data_tuple) {
            text_layer_set_text(text_layer, data_tuple->value->cstring);
    }
}

static void inbox_dropped_handler(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Dropped!");
}

static void outbox_failed_handler(DictionaryIterator *failed,
                                  AppMessageResult reason, void *context) {
    if(wasFirstMsg) {
        // Do nothing
    } else {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Failed to Send!");
    }
    wasFirstMsg = false;
}

static void outbox_sent_handler(DictionaryIterator *iter, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Outbox send success!");
}

static void click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    text_layer = text_layer_create((GRect) { .origin = { 0, 72 }, .size = { bounds.size.w, 20 } });
    text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(text_layer));

    wasFirstMsg = true;
}

static void window_unload(Window *window) {
    text_layer_destroy(text_layer);
}

static void init(void) {
    wasFirstMsg = false;
    window = window_create();
    window_set_click_config_provider(window, click_config_provider);
    // Register message handlers
    app_message_register_inbox_received(inbox_received_handler);
    app_message_register_inbox_dropped(inbox_dropped_handler);
    app_message_register_outbox_failed(outbox_failed_handler);
    app_message_register_outbox_sent(outbox_sent_handler);

    app_message_open(app_message_inbox_size_maximum(),
                     app_message_outbox_size_maximum());

    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
    const bool animated = true;
    window_stack_push(window, animated);
}

static void deinit(void) {
    window_destroy(window);
}

int main(void) {
    init();

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

    app_event_loop();
    deinit();
}
