/**
 * @file main.c
 * @brief Application Main Entry Point (User-Maintained)
 * Note: This file will NOT be overwritten during GUI regeneration.
 */

#include <stdio.h>
#include <stdbool.h>
#include "custom_logic.h"
#include "target_hal.h"
#include "ui_bindings.h"

int main(void) {
    // 1. Initialize Hardware Board & Peripherals
    target_hal_init();

    // 3. Initialize Hardware Data Bindings
    ui_bindings_init();

    // 4. Initialize User Custom Application Logic
    custom_logic_init();

    printf("Embedded UI application started successfully.\n");

    // Main Event Loop
    while (1) {
        target_hal_poll();
        ui_bindings_update();
        custom_logic_loop();
    }

    return 0;
}
