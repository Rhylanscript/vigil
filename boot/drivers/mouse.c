// boot/drivers/mouse.c

#include "mouse.h"
#include "irq.h"
#include "pic.h"
#include "io.h"
#include "graphics.h"

#define PS2_DATA_PORT    0x60
#define PS2_STATUS_PORT  0x64
#define PS2_COMMAND_PORT 0x64

#define PS2_STATUS_OUTPUT_FULL 0x01
#define PS2_STATUS_INPUT_FULL  0x02

#define PS2_CMD_ENABLE_AUX     0xA8
#define PS2_CMD_READ_CONFIG    0x20
#define PS2_CMD_WRITE_CONFIG   0x60
#define PS2_CMD_WRITE_TO_MOUSE 0xD4

#define MOUSE_CMD_SET_DEFAULTS     0xF6
#define MOUSE_CMD_ENABLE_REPORTING 0xF4

#define MOUSE_IRQ 12

#define CURSOR_SIZE 8
#define CURSOR_COLOR GFX_WHITE

// --- byte level comms with the ps/2 controller ---

static void ps2_wait_write_ready(void) {
    while (inb(PS2_STATUS_PORT) & PS2_STATUS_INPUT_FULL) {
    }
}

static void ps2_wait_read_ready(void) {
    while (!(inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL)) {
    }
}

static void ps2_write_command(uint8_t command) {
    ps2_wait_write_ready();
    outb(PS2_COMMAND_PORT, command);
}

static void ps2_write_data(uint8_t data) {
    ps2_wait_write_ready();
    outb(PS2_DATA_PORT, data);
}

static uint8_t ps2_read_data(void) {
    ps2_wait_read_ready();
    return inb(PS2_DATA_PORT);
}

static void mouse_send_command(uint8_t command) {
    ps2_write_command(PS2_CMD_WRITE_TO_MOUSE);
    ps2_write_data(command);
    ps2_read_data();
}

// --- packet parsing ---

static uint8_t packet[3];
static int packet_index = 0;

static int32_t cursor_x;
static int32_t cursor_y;

static uint8_t button_state;

static void process_packet(void) {
    uint8_t flags = packet[0];

    if (!(flags & 0x08)) {
        packet_index = 0;
        return;
    }

    button_state = flags & 0x07;

    int32_t dx = packet[1];
    if (flags & 0x10) {
        dx -= 256;
    }

    int32_t dy = packet[2];
    if (flags & 0x20) {
        dy -= 256;
    }

    cursor_x += dx;
    cursor_y -= dy;

    if (cursor_x < 0) cursor_x = 0;
    if (cursor_y < 0) cursor_y = 0;
    if (cursor_x >= (int32_t) gfx_screen_width())  cursor_x = (int32_t) gfx_screen_width() - 1;
    if (cursor_y >= (int32_t) gfx_screen_height()) cursor_y = (int32_t) gfx_screen_height() - 1;
}

static void mouse_callback(struct registers regs) {
    (void) regs;

    packet[packet_index] = inb(PS2_DATA_PORT);
    packet_index++;

    if (packet_index >= 3) {
        process_packet();
        packet_index = 0;
    }
}

void mouse_install(void) {
    ps2_write_command(PS2_CMD_ENABLE_AUX);

    ps2_write_command(PS2_CMD_READ_CONFIG);
    uint8_t config = ps2_read_data();
    config |= 0x02;
    config &= ~0x20;
    ps2_write_command(PS2_CMD_WRITE_CONFIG);
    ps2_write_data(config);

    mouse_send_command(MOUSE_CMD_SET_DEFAULTS);
    mouse_send_command(MOUSE_CMD_ENABLE_REPORTING);

    cursor_x = (int32_t) gfx_screen_width() / 2;
    cursor_y = (int32_t) gfx_screen_height() / 2;

    pic_unmask_irq(2);
    pic_unmask_irq(MOUSE_IRQ);

    irq_install_handler(MOUSE_IRQ, mouse_callback);
}

// --- cursor sprite drawing ---

static gfx_color_t saved_pixels[CURSOR_SIZE][CURSOR_SIZE];
static int32_t drawn_x;
static int32_t drawn_y;
static int cursor_is_drawn = 0;

static void restore_under_cursor(void) {
    for (int row = 0; row < CURSOR_SIZE; row++) {
        for (int col = 0; col < CURSOR_SIZE; col++) {
            gfx_put_pixel((uint32_t) (drawn_x + col), (uint32_t) (drawn_y + row), saved_pixels[row][col]);
        }
    }
}

static void draw_cursor_at(int32_t x, int32_t y) {
    for (int row = 0; row < CURSOR_SIZE; row++) {
        for (int col = 0; col < CURSOR_SIZE; col++) {
            saved_pixels[row][col] = gfx_get_pixel((uint32_t) (x + col), (uint32_t) (y + row));
            gfx_put_pixel((uint32_t) (x + col), (uint32_t) (y + row), CURSOR_COLOR);
        }
    }
    drawn_x = x;
    drawn_y = y;
}

void mouse_update_cursor(void) {
    if (cursor_is_drawn && drawn_x == cursor_x && drawn_y == cursor_y) {
        return;
    }

    if (cursor_is_drawn) {
        restore_under_cursor();
    }

    draw_cursor_at(cursor_x, cursor_y);
    cursor_is_drawn = 1;
}

// --- public accessors ---

int32_t mouse_get_x(void) { return cursor_x; }
int32_t mouse_get_y(void) { return cursor_y; }

int mouse_left_button(void)   { return button_state & 0x01; }
int mouse_right_button(void)  { return button_state & 0x02; }
int mouse_middle_button(void) { return button_state & 0x04; }
