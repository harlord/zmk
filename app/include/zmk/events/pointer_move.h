/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>

struct zmk_pointer_move {
    int16_t dx;
    int16_t dy;
    int64_t timestamp;
};

ZMK_EVENT_DECLARE(zmk_pointer_move);


