#include <furi.h>
#include <furi_hal.h>

#include <gui/gui.h>
#include <input/input.h>

#include <infrared_worker.h>
#include <infrared_transmit.h>

#define INTERVAL 400
#define REPEAT   2
#define TEXTSIZE 30
#define MAXENTRY 2

#define TAG "IrBlaster"

typedef enum {
    ProtocolSelect = 0,
    AddressSelect = 1,
    CommandSelect = 2,
} Selected;

typedef enum {
    Config = 0,
    Shooting = 1,
} Prgmode;

typedef struct {
    Prgmode mode;
    Selected entry;
    uint32_t address;
    uint32_t command;
    InfraredProtocol protocol;
} TheContext;

static void app_draw_callback(Canvas* canvas, void* ctx);
static void app_input_callback(InputEvent* event, void* ctx);
uint32_t getMaxCommand(TheContext* context);
uint32_t getMaxAddress(TheContext* context);
bool configsetting(TheContext* context, InputEvent event);
void ir_transmit(TheContext* context);
void shooting(void* ctx);
int32_t irblaster_main(void* p);
