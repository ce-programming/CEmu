#ifdef DEBUG_SUPPORT

#include "debug.h"
#include "../mem.h"
#include "../emu.h"
#include "../cpu.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

debug_state_t debug;

void debug_init(void) {
    debug.addr = (uint8_t*)calloc(0x1000000, 1);
    debug.port = (uint8_t*)calloc(0x10000, 1);
    debug.bufPos = debug.bufErrPos = 0;
    debug.open = false;
    gui_console_printf("[CEmu] Initialized Debugger...\n");
}

void debug_free(void) {
    free(debug.addr);
    free(debug.port);
    gui_console_printf("[CEmu] Freed Debugger.\n");
}

bool debug_is_open(void) {
    return debug.open;
}

void debug_open(int reason, uint32_t data) {
    if (cpu.abort == CPU_ABORT_EXIT || debug.open || (debug.ignore && (reason >= DBG_BREAKPOINT && reason <= DBG_PORT_WRITE))) {
        return;
    }

    debug_clear_step();

    debug.cpuCycles = cpu.cycles;
    debug.cpuNext = cpu.next;
    debug.cpuBaseCycles = cpu.baseCycles;
    debug.cpuHaltCycles = cpu.haltCycles;
    debug.totalCycles += sched_total_cycles();
    debug.dmaCycles += cpu.dmaCycles;

    debug.open = true;
    gui_debug_open(reason, data);
    debug.open = false;

    cpu.next = debug.cpuNext;
    cpu.cycles = debug.cpuCycles;
    cpu.baseCycles = debug.cpuBaseCycles;
    cpu.haltCycles = debug.cpuHaltCycles;
    debug.dmaCycles -= cpu.dmaCycles;
    debug.totalCycles -= sched_total_cycles();
}

void debug_watch(uint32_t addr, int mask, bool set) {
    addr &= 0xFFFFFF;
    if (set) {
        debug.addr[addr] |= mask;
    } else {
        debug.addr[addr] &= ~mask;
    }
}

void debug_ports(uint16_t addr, int mask, bool set) {
    addr &= 0xFFFF;
    if (set) {
        debug.port[addr] |= mask;
    } else {
        debug.port[addr] &= ~mask;
    }
}

void debug_flag(int mask, bool set) {
    if (set) {
        debug.flags |= mask;
    } else {
        debug.flags &= ~mask;
    }
    debug.ignore = debug.flags & DBG_IGNORE;
    debug.commands = debug.flags & DBG_SOFT_COMMANDS;
    debug.openOnReset = debug.flags & DBG_OPEN_ON_RESET;
}

void debug_step(int mode, uint32_t addr) {
    switch (mode) {
        case DBG_STEP_NEXT:
        case DBG_RUN_UNTIL:
            debug.tempMode = cpu.ADL;
            debug_watch(debug.tempAddr = addr, DBG_MASK_TEMP, true);
            break;
    }
}

void debug_clear_step(void) {
    uint32_t addr = debug.tempAddr;
    while (debug.addr[addr] & DBG_MASK_TEMP) {
        debug_watch(debug.tempAddr, DBG_MASK_TEMP, false);
        addr = cpu_mask_mode(addr + 1, debug.tempMode);
    }
}

void debug_set_pc(uint32_t addr) {
    cpu_flush(addr, cpu.ADL);
}

#endif
