#include <irblaster.h>

#include <math.h>

static void app_draw_callback(Canvas* canvas, void* ctx) {
    char text[TEXTSIZE];
    TheContext* context = ctx;

    canvas_set_font(canvas, FontPrimary);
    snprintf(text, TEXTSIZE, "IR Blaster %s", infrared_get_protocol_name(context->protocol));
    canvas_draw_str(canvas, 1, 15, text);
    canvas_draw_line(canvas, 0, 16, 128, 16);
    if(context->mode == Shooting) {
        canvas_draw_str(canvas, 1, 48, "RUNNING");
        canvas_draw_str(canvas, 1, 58, "Press BACK to stop");
    } else {
        canvas_draw_str(canvas, 1, 58, "Press OK to start");
    }
    snprintf(text, TEXTSIZE, "Address: 0x%X", (int)context->address);
    canvas_draw_str(canvas, 1, 28, text);
    if(context->entry == AddressSelect && context->mode == Config) {
        canvas_draw_disc(canvas, 120, 24, 4);
    }
    snprintf(text, TEXTSIZE, "Command: 0x%X", (int)context->command);
    canvas_draw_str(canvas, 1, 38, text);
    if(context->entry == CommandSelect && context->mode == Config) {
        canvas_draw_disc(canvas, 120, 34, 4);
    }
    if(context->entry == ProtocolSelect && context->mode == Config) {
        canvas_draw_disc(canvas, 120, 10, 4);
    }
}

static void app_input_callback(InputEvent* event, void* ctx) {
    furi_message_queue_put(ctx, event, FuriWaitForever);
}

uint32_t getMaxCommand(TheContext* context) {
    return ((uint32_t)pow(2, (double)infrared_get_protocol_command_length(context->protocol)) - 1);
}

uint32_t getMaxAddress(TheContext* context) {
    return ((uint32_t)pow(2, (double)infrared_get_protocol_address_length(context->protocol)) - 1);
}

bool configsetting(TheContext* context, InputEvent event) {
    if(event.key == InputKeyDown) {
        context->entry++;
        if(context->entry > MAXENTRY) context->entry = ProtocolSelect;
    }
    if(event.key == InputKeyUp) {
        if(context->entry == 0) {
            context->entry = CommandSelect;
        } else {
            context->entry--;
        }
    }
    if(event.key == InputKeyLeft) {
        if(context->entry == CommandSelect) {
            if(context->command > 1) context->command--;
        }
        if(context->entry == AddressSelect) {
            if(context->address > 0) context->address--;
        }
        if(context->entry == ProtocolSelect) {
            if(context->protocol > 0) {
                context->protocol--;
                context->address = 0;
                context->command = 1;
            }
        }
    }

    if(event.key == InputKeyRight) {
        if(context->entry == CommandSelect) {
            context->command++;
            if(context->command > getMaxCommand(context)) {
                context->command--;
            }
        }
        if(context->entry == AddressSelect) {
            context->address++;
            if(context->address > getMaxAddress(context)) {
                context->address--;
            }
        }
        if(context->entry == ProtocolSelect) {
            context->protocol++;
            context->address = 0;
            context->command = 1;
            if(context->protocol > (InfraredProtocolMAX - 1)) {
                context->protocol--;
            }
        }
    }

    if(event.key == InputKeyBack) {
        return false;
    }
    if(event.key == InputKeyOk) {
        context->mode = Shooting;
    }
    return true;
}

void ir_transmit(TheContext* context) {
    InfraredMessage* message = malloc(sizeof(InfraredMessage));
    message->protocol = context->protocol;
    message->address = context->address;
    message->command = context->command;
    message->repeat = false;
    infrared_send(message, REPEAT);
    free(message);
}

void shooting(void* ctx) {
    TheContext* context = (TheContext*)ctx;
    /*
    FURI_LOG_I(
        TAG,
        "Transmit: %s a:0x%lX c:0x%lX maxC:0x%lX maxA:0x%lX",
        infrared_get_protocol_name(context->protocol),
        (uint32_t)context->address,
        (uint32_t)context->command,
        getMaxCommand(context),
        getMaxAddress(context));
    */
    ir_transmit(context);
    context->command++;
    if(context->command > getMaxCommand(context)) {
        context->command = 1;
        context->address++;
        if(context->address > getMaxAddress(context)) {
            context->address--;
        }
    }
}

int32_t irblaster_main(void* p) {
    ViewPort* wp = NULL;
    FuriMessageQueue* event_queue;
    FuriTimer* timer;
    TheContext* context;
    Gui* gui;

    // Initialization
    UNUSED(p);
    context = malloc(sizeof(TheContext));
    context->address = 0;
    context->command = 1;
    context->mode = Config;
    context->protocol = InfraredProtocolNEC;
    event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    wp = view_port_alloc();
    gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, wp, GuiLayerFullscreen);
    view_port_draw_callback_set(wp, app_draw_callback, context);
    view_port_input_callback_set(wp, app_input_callback, event_queue);

    timer = furi_timer_alloc(shooting, FuriTimerTypePeriodic, context);

    // Main loop
    for(bool is_running = true; is_running;) {
        InputEvent event;
        const FuriStatus status = furi_message_queue_get(event_queue, &event, FuriWaitForever);

        if(status != FuriStatusOk || event.type != InputTypePress) {
            continue;
        }

        if(context->mode == Config) {
            is_running = configsetting(context, event);
            if(context->mode == Shooting) {
                furi_timer_start(timer, furi_ms_to_ticks(INTERVAL));
            }
        } else {
            if(event.key == InputKeyBack) {
                context->mode = Config;
                furi_timer_stop(timer);
            }
        }
    }

    // Release resources
    view_port_enabled_set(wp, false);
    gui_remove_view_port(gui, wp);
    furi_timer_free(timer);
    furi_message_queue_free(event_queue);
    view_port_free(wp);
    furi_record_close(RECORD_GUI);
    free(context);
    return 0;
}
