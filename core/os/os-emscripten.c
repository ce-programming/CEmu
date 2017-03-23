#ifdef __EMSCRIPTEN__

#include <emscripten.h>

#define _BSD_SOURCE
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "os.h"

#include "../../core/emu.h"
#include "../../core/lcd.h"
#include "../../core/debug/debug.h"

extern lcd_state_t lcd;

FILE *fopen_utf8(const char *filename, const char *mode)
{
    return fopen(filename, mode);
}

void throttle_timer_off() {}
void throttle_timer_on() {}
void throttle_timer_wait() {}

void gui_emu_sleep() { usleep(50); }
void gui_do_stuff()
{
    if (debugger.currentBuffPos) {
        debugger.buffer[debugger.currentBuffPos] = '\0';
        fprintf(stdout, "[CEmu DbgOutPrint] %s\n", debugger.buffer);
        fflush(stdout);
        debugger.currentBuffPos = 0;
    }

    if (debugger.currentErrBuffPos) {
        debugger.errBuffer[debugger.currentErrBuffPos] = '\0';
        fprintf(stderr, "[CEmu DbgErrPrint] %s\n", debugger.errBuffer);
        fflush(stderr);
        debugger.currentErrBuffPos = 0;
    }
}

void gui_set_busy(bool busy) {}

void gui_debugger_raise_or_disable(bool entered)
{
    inDebugger = false;
}

void gui_debugger_send_command(int reason, uint32_t addr)
{
    printf("[CEmu Debugger] Got software command (r=%d, addr=0x%X)\n", reason, addr);
    fflush(stdout);
    inDebugger = false;
}

void gui_console_vprintf(const char *fmt, va_list ap)
{
    vfprintf(stdout, fmt, ap);
    fflush(stdout);
}

void gui_console_err_vprintf(const char *fmt, va_list ap)
{
    vfprintf(stderr, fmt, ap);
    fflush(stderr);
}

void gui_console_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    gui_console_vprintf(fmt, ap);
    va_end(ap);
}

void gui_console_err_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    gui_console_err_vprintf(fmt, ap);

    va_end(ap);
}

void gui_perror(const char *msg)
{
    printf("[Error] %s: %s\n", msg, strerror(errno));
    fflush(stdout);
}

uint32_t * buf_lcd_js = NULL;

void EMSCRIPTEN_KEEPALIVE set_lcd_js_ptr(uint32_t * ptr) {
    buf_lcd_js = ptr;
}

void paint_LCD_to_JS()
{
    if (lcd.control & 0x800) { // LCD on
        lcd_drawframe(buf_lcd_js, &lcd);
        EM_ASM(repaint());
    } else { // LCD off
        EM_ASM(drawLCDOff());
    }
}

void EMSCRIPTEN_KEEPALIVE emsc_pause_main_loop() {
    emscripten_pause_main_loop();
}

void EMSCRIPTEN_KEEPALIVE emsc_resume_main_loop() {
    emscripten_resume_main_loop();
}

void EMSCRIPTEN_KEEPALIVE emsc_cancel_main_loop() {
    emu_cleanup();
    emscripten_cancel_main_loop();
}

int main(int argc, char* argv[])
{
    bool success;
    emulationPaused = false;

    success = emu_start("CE.rom", NULL);

    if (success) {
        debugger_init();
        EM_ASM(
            emul_is_inited = true;
            emul_is_paused = false;
            initFuncs();
            initLCD();
            enableGUI();
        );
        lcd_event_gui_callback = paint_LCD_to_JS;
        emu_loop(true);
    } else {
        EM_ASM(
            lcd_event_gui_callback = NULL;
            emul_is_inited = false;
            disableGUI();
            alert("Error: Couldn't start emulation ; bad ROM?");
        );
        return 1;
    }

    puts("Finished");

    EM_ASM(
        lcd_event_gui_callback = NULL;
        emul_is_inited = false;
        disableGUI();
    );

    return 0;
}

#endif
