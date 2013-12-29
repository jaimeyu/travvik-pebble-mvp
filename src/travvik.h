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

#ifndef WEATHER_H
#define	WEATHER_H

#ifdef	__cplusplus
extern "C" {
#endif

    enum {
        SN_0 = 0,
        SN_1 = 1,
        SN_2 = 2,
        SN_3 = 3,
    };

    enum {
        STATE_SEL_BUS = 0,
        STATE_SEL_STOP
    };

    /**
        "busNumber": 0,
        "stopLabel": 1,
        "destination": 2,
        "arrival":3   
     */

    enum TRIP_KEYS {
        TRIP_ARRIVAL = 0,
        TRIP_ORIGIN,
        TRIP_DESTINATION,
        TRIP_DIRECTION,
        REQ_BUS_NB,
        REQ_STOP_NB,
    };

    static void send_cmd(const int, const int, const int);
    static void up_click_handler(ClickRecognizerRef recognizer, void *context);
    static void down_click_handler(ClickRecognizerRef recognizer, void *context);
    static void select_click_handler(ClickRecognizerRef recognizer, void *context);
    static void select_long_click_handler(ClickRecognizerRef recognizer, void *context);
    static void click_config_provider(void *context);
    static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context);
    static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context);
    static void window_load(Window *window);
    static void window_unload(Window *window);
    static void init(void);
    static void deinit(void);
    static void printTextLabel(TextLayer *txl, char *buf, const int len, const int *val);
    static TextLayer* textLayerFactory(
            TextLayer* layer,
            const GRect frame,
            const GColor font_color,
            const GColor bkg_color,
            const GTextAlignment alignment,
            const GTextOverflowMode overflow,
            const GFont font,
            Layer* window);

    static void printTimeLeftLabel(TextLayer *txl, char *buf, const int len, const int *val);
    static void up_click_handler(ClickRecognizerRef recognizer, void *context);


#ifdef	__cplusplus
}
#endif

#endif	/* WEATHER_H */

