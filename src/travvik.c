/*
* The MIT License (MIT)
* 
* Copyright (c) 2014 Jaime Yu
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
* 
*/

#include "pebble.h"
#include "travvik.h"

static Window *window;

/**
 * route text layers
 */
static TextLayer *tlArrivalTime;
static TextLayer *tlOriginStopStr;
static TextLayer *tlDestinationStr;
static TextLayer *tlToInterjection;
static TextLayer *tlInPreposition;
static TextLayer *tlSelectStopNumber;
static TextLayer *tlSelectBusNumber;
static TextLayer *tlBusSelectorArrow;
static TextLayer *tlStopSelectorArrow;
static TextLayer *tlTimeNow;

static int iArrival = 0;
static int iCurDirection = 0;
static int iCurRouteNumber = 97;
static int iCurStopNumber = 3036;
static int eStateSelector = STATE_SEL_BUS;
static char sRouteStrBuf[32];
static char sStopStrBuf[32];
static char sArrival[32];
static int8_t uiCountdown = 0;

static AppSync sync;
static uint8_t sync_buffer[256];

/* Methods */

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
    // Need to be static because they're used by the system later.
    static char time_text[] = "00:00";

    char *time_format;

    if (clock_is_24h_style()) {
        time_format = "%R";
    } else {
        time_format = "%I:%M";
    }

    strftime(time_text, sizeof (time_text), time_format, tick_time);

    // Kludge to handle lack of non-padded hour format string
    // for twelve hour clock.
    if (!clock_is_24h_style() && (time_text[0] == '0')) {
        memmove(time_text, &time_text[1], sizeof (time_text) - 1);
    }

    iArrival--;
    if (iArrival < 0) {
        text_layer_set_text(tlArrivalTime, "Departed");
        uiCountdown = 0;
    } else if (iArrival == 0) {
        text_layer_set_text(tlArrivalTime, "Arriving");
    } else {
        //printTextLabel(tlArrivalTime, sArrival, 10, &iArrival);
        printTimeLeftLabel(tlArrivalTime, sArrival, 10, &iArrival);
    }

    // After n times, request new data for the sake of new data.
    uiCountdown--;
    if (uiCountdown < 0) {
        send_cmd(iCurRouteNumber, iCurStopNumber, iCurDirection);
    }

    text_layer_set_text(tlTimeNow, time_text);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {

    int clicks = click_number_of_clicks_counted(recognizer);

    if (eStateSelector == STATE_SEL_BUS) {
        iCurRouteNumber += clicks;
        if (iCurRouteNumber > 999) {
            iCurRouteNumber = 0;
        }

    } else if (eStateSelector == STATE_SEL_STOP) {
        iCurStopNumber += clicks;
        if (iCurStopNumber > 9999) {
            iCurStopNumber = 0;
        }
    }

    printTextLabel(tlSelectBusNumber, sRouteStrBuf, 10, &iCurRouteNumber);

    printTextLabel(tlSelectStopNumber, sStopStrBuf, 10, &iCurStopNumber);
}

static void printTextLabel(TextLayer *txl, char *buf, const int len, const int *val) {
    snprintf(buf, len, "%d", *val);
    text_layer_set_text(txl, buf);
}

static void printTimeLeftLabel(TextLayer *txl, char *buf, const int len, const int *val) {
    snprintf(buf, len, "%d mins", *val);
    text_layer_set_text(txl, buf);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    int clicks = click_number_of_clicks_counted(recognizer);

    if (eStateSelector == STATE_SEL_BUS) {
        iCurRouteNumber -= clicks;
        ;
        if (iCurRouteNumber < 0) {
            iCurRouteNumber = 0;
        }

    } else if (eStateSelector == STATE_SEL_STOP) {
        iCurStopNumber -= clicks;
        if (iCurStopNumber < 0) {
            iCurStopNumber = 0;
        }
    }

    printTextLabel(tlSelectBusNumber, sRouteStrBuf, 10, &iCurRouteNumber);

    printTextLabel(tlSelectStopNumber, sStopStrBuf, 10, &iCurStopNumber);

}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "select_click_handler: clicked");

    switch (eStateSelector) {
        default:
        case STATE_SEL_BUS:
            eStateSelector = STATE_SEL_STOP;
            text_layer_set_text(tlBusSelectorArrow, "");
            text_layer_set_text(tlStopSelectorArrow, "  ^");
            APP_LOG(APP_LOG_LEVEL_DEBUG, "Changing state_cur_mod: STATE_SEL_STOP");
            break;

        case STATE_SEL_STOP:
            eStateSelector = STATE_SEL_BUS;
            text_layer_set_text(tlBusSelectorArrow, "^");
            text_layer_set_text(tlStopSelectorArrow, "");
            APP_LOG(APP_LOG_LEVEL_DEBUG, "Changing state_cur_mod: STATE_SEL_BUS");
            break;
    }

}

static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "select_long_click_handler: clicked");
    text_layer_set_text(tlArrivalTime, "Loading");
    text_layer_set_text(tlOriginStopStr, "Loading...");
    text_layer_set_text(tlDestinationStr, "Loading...");

    iCurDirection++;
    iCurDirection = iCurDirection & 0x1;
    send_cmd(iCurRouteNumber, iCurStopNumber, iCurDirection);
}

static void click_config_provider(void *context) {
    const uint16_t repeat_interval_ms = 100;
    window_single_repeating_click_subscribe(BUTTON_ID_UP, repeat_interval_ms, up_click_handler);
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
    window_long_click_subscribe(BUTTON_ID_SELECT, 0, select_long_click_handler, NULL);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, repeat_interval_ms, down_click_handler);
}

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
    text_layer_set_text(tlArrivalTime, "No Bus");

}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
    switch (key) {
        case TRIP_ARRIVAL:
            APP_LOG(APP_LOG_LEVEL_DEBUG, "TRIP_ARRIVAL");
            iArrival = new_tuple->value->int32;
            //printTextLabel(tlArrivalTime, sArrival, 10, &iArrival);
            printTimeLeftLabel(tlArrivalTime, sArrival, 10, &iArrival);
            uiCountdown = 3;
            break;
        case TRIP_ORIGIN:
            APP_LOG(APP_LOG_LEVEL_DEBUG, "TRIP_ORIGIN");
            text_layer_set_text(tlOriginStopStr, new_tuple->value->cstring);
            break;
        case TRIP_DESTINATION:
            APP_LOG(APP_LOG_LEVEL_DEBUG, "TRIP_DESTINATION");
            text_layer_set_text(tlDestinationStr, new_tuple->value->cstring);
            break;
        case TRIP_DIRECTION:
            APP_LOG(APP_LOG_LEVEL_DEBUG, "TRIP_DIRECTION");
            iCurDirection = new_tuple->value->int32;
            break;
        case REQ_BUS_NB:
            APP_LOG(APP_LOG_LEVEL_DEBUG, "REQ_BUS_NB");
            iCurRouteNumber = new_tuple->value->int32;
            printTextLabel(tlSelectBusNumber, sRouteStrBuf, 10, &iCurRouteNumber);
            break;
        case REQ_STOP_NB: APP_LOG(APP_LOG_LEVEL_DEBUG, "REQ_STOP_NB");
            iCurStopNumber = new_tuple->value->int32;
            printTextLabel(tlSelectStopNumber, sStopStrBuf, 10, &iCurStopNumber);
            break;
        default:
            break;
    }
}

static void send_cmd(const int route, const int stopnb, const int direction) {
    DictionaryIterator *iter;

    Tuplet bus = TupletInteger(REQ_BUS_NB, route);
    Tuplet stop = TupletInteger(REQ_STOP_NB, stopnb);
    Tuplet dir = TupletInteger(TRIP_DIRECTION, direction);


    app_message_outbox_begin(&iter);

    if (iter == NULL) {
        return;
    }

    dict_write_tuplet(iter, &bus);
    dict_write_tuplet(iter, &stop);
    dict_write_tuplet(iter, &dir);
    dict_write_end(iter);

    app_message_outbox_send();
}

static TextLayer* textLayerFactory(
        TextLayer* layer,
        const GRect frame,
        const GColor font_color,
        const GColor bkg_color,
        const GTextAlignment alignment,
        const GTextOverflowMode overflow,
        const GFont font,
        Layer* window) {

    layer = text_layer_create(frame);
    text_layer_set_text_color(layer, font_color);
    text_layer_set_background_color(layer, bkg_color);
    text_layer_set_font(layer, fonts_get_system_font(font));
    text_layer_set_text_alignment(layer, alignment);
    text_layer_set_overflow_mode(layer, overflow);
    layer_add_child(window, text_layer_get_layer(layer));

    return layer;
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);

    /* Next arrival time*/
    tlArrivalTime = textLayerFactory(
            tlArrivalTime,
            GRect(0, 25, 144, 35),
            GColorWhite,
            GColorClear,
            GTextAlignmentCenter,
            GTextOverflowModeTrailingEllipsis,
            FONT_KEY_BITHAM_30_BLACK,
            window_layer
            );

    /* Source label */
    tlOriginStopStr = textLayerFactory(
            tlOriginStopStr,
            GRect(0, 60, 144, 30),
            GColorWhite,
            GColorClear,
            GTextAlignmentCenter,
            GTextOverflowModeTrailingEllipsis,
            FONT_KEY_GOTHIC_24,
            window_layer
            );

    /*  ("to") label */
    tlToInterjection = textLayerFactory(
            tlToInterjection,
            GRect(0, 90, 144, 20),
            GColorWhite,
            GColorClear,
            GTextAlignmentCenter,
            GTextOverflowModeTrailingEllipsis,
            FONT_KEY_GOTHIC_18_BOLD,
            window_layer
            );


    /* Destination label*/
    tlDestinationStr = textLayerFactory(
            tlDestinationStr,
            GRect(0, 110, 144, 30),
            GColorWhite,
            GColorClear,
            GTextAlignmentCenter,
            GTextOverflowModeTrailingEllipsis,
            FONT_KEY_GOTHIC_24,
            window_layer
            );


    /* in label*/
    tlInPreposition = textLayerFactory(
            tlInPreposition,
            GRect(0, 130, 144, 20),
            GColorWhite,
            GColorClear,
            GTextAlignmentCenter,
            GTextOverflowModeTrailingEllipsis,
            FONT_KEY_GOTHIC_18_BOLD,
            window_layer
            );


    /* busid label*/
    tlSelectBusNumber = textLayerFactory(
            tlSelectBusNumber,
            GRect(0, 135, 144, 20),
            GColorWhite,
            GColorClear,
            GTextAlignmentLeft,
            GTextOverflowModeTrailingEllipsis,
            FONT_KEY_GOTHIC_18_BOLD,
            window_layer
            );



    /* stopid label*/
    tlSelectStopNumber = textLayerFactory(
            tlSelectStopNumber,
            GRect(0, 135, 144, 20),
            GColorWhite,
            GColorClear,
            GTextAlignmentRight,
            GTextOverflowModeTrailingEllipsis,
            FONT_KEY_GOTHIC_18_BOLD,
            window_layer
            );


    /* stop cursor label*/
    tlStopSelectorArrow = textLayerFactory(
            tlStopSelectorArrow,
            GRect(0, 148, 144, 30),
            GColorWhite,
            GColorClear,
            GTextAlignmentRight,
            GTextOverflowModeTrailingEllipsis,
            FONT_KEY_DROID_SERIF_28_BOLD,
            window_layer
            );

    /* bus cursor label*/
    tlBusSelectorArrow = textLayerFactory(
            tlBusSelectorArrow,
            GRect(0, 148, 18, 30),
            GColorWhite,
            GColorClear,
            GTextAlignmentLeft,
            GTextOverflowModeTrailingEllipsis,
            FONT_KEY_DROID_SERIF_28_BOLD,
            window_layer
            );

    /* Time now */
    tlTimeNow = textLayerFactory(
            tlTimeNow,
            GRect(0, -5, 144, 30),
            GColorBlack,
            GColorWhite,
            GTextAlignmentCenter,
            GTextOverflowModeTrailingEllipsis,
            FONT_KEY_BITHAM_30_BLACK,
            window_layer
            );


    tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);


    text_layer_set_text(tlOriginStopStr, "Fail");
    text_layer_set_text(tlDestinationStr, "Connect");
    text_layer_set_text(tlToInterjection, "to");

    printTextLabel(tlSelectBusNumber, sRouteStrBuf, 10, &iCurRouteNumber);
    printTextLabel(tlSelectStopNumber, sStopStrBuf, 10, &iCurStopNumber);


    text_layer_set_text(tlBusSelectorArrow, "^");

    Tuplet initial_values[] = {
        TupletInteger(TRIP_ARRIVAL, 0),
        TupletCString(TRIP_ORIGIN, "No stop"),
        TupletCString(TRIP_DESTINATION, "No destination"),
        TupletInteger(TRIP_DIRECTION, 0),
        TupletInteger(REQ_BUS_NB, -1),
        TupletInteger(REQ_STOP_NB, -1),
    };

    app_sync_init(&sync, sync_buffer, sizeof (sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
            sync_tuple_changed_callback, sync_error_callback, NULL);

    send_cmd(-1, -1, -1);
}

static void window_unload(Window *window) {
    app_sync_deinit(&sync);

    text_layer_destroy(tlArrivalTime);
    text_layer_destroy(tlOriginStopStr);
    text_layer_destroy(tlDestinationStr);
    text_layer_destroy(tlToInterjection);
    text_layer_destroy(tlInPreposition);
    text_layer_destroy(tlSelectBusNumber);
    text_layer_destroy(tlSelectStopNumber);
    text_layer_destroy(tlBusSelectorArrow);
    text_layer_destroy(tlStopSelectorArrow);

}

static void init(void) {
    window = window_create();
    window_set_click_config_provider(window, click_config_provider);

    window_set_background_color(window, GColorBlack);
    window_set_fullscreen(window, true);

    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload
    });

    const int inbound_size = 256;
    const int outbound_size = 256;
    app_message_open(inbound_size, outbound_size);

    const bool animated = true;
    window_stack_push(window, animated);
}

static void deinit(void) {
    tick_timer_service_unsubscribe();

    window_destroy(window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}


