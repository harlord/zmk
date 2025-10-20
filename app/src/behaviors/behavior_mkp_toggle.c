#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>
#include <zmk/endpoints.h>
#include <zmk/hid.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <drivers/behavior.h>
#include <zephyr/sys/util.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define DT_DRV_COMPAT zmk_behavior_mkp_toggle

/* -------------------------------------------------------------------------- */
/* Build-time selection                                                         */
/* -------------------------------------------------------------------------- */

#define MKP_HAS_POINTING_CENTER (IS_ENABLED(CONFIG_ZMK_POINTING) &&                       \
                                (!IS_ENABLED(CONFIG_ZMK_SPLIT) ||                        \
                                 IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)))

#if MKP_HAS_POINTING_CENTER

#define MAX_MOUSE_BUTTONS 8
static bool button_state[MAX_MOUSE_BUTTONS] = {false};

enum toggle_start_mode { TOGGLE_START_OFF = 0, TOGGLE_START_ON = 1 };

struct mkp_toggle_config { enum toggle_start_mode toggle_mode; };

static int mkp_toggle_press(struct zmk_behavior_binding *binding,                           \
                            struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct mkp_toggle_config *cfg = dev->config; /* may be unused */
    ARG_UNUSED(cfg);

    uint8_t button = binding->param1;
    if (button >= MAX_MOUSE_BUTTONS) {
        return -EINVAL;
    }

    if (!button_state[button]) {
        zmk_hid_mouse_button_press(button);
        button_state[button] = true;
    } else {
        zmk_hid_mouse_button_release(button);
        button_state[button] = false;
    }
    return ZMK_BEHAVIOR_OPAQUE;
}

static int mkp_toggle_release(struct zmk_behavior_binding *binding,                          \
                              struct zmk_behavior_binding_event event) {                    \
    ARG_UNUSED(binding);                                                                     \
    ARG_UNUSED(event);                                                                       \
    return ZMK_BEHAVIOR_OPAQUE;                                                             \
}

static int on_endpoint_changed(const zmk_event_t *eh) {
    if (!as_zmk_endpoint_changed(eh)) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    for (int i = 0; i < MAX_MOUSE_BUTTONS; i++) {
        if (button_state[i]) {
            zmk_hid_mouse_button_release(i);
            button_state[i] = false;
        }
    }
    return 0;
}

ZMK_LISTENER(mkp_toggle, on_endpoint_changed);
ZMK_SUBSCRIPTION(mkp_toggle, zmk_endpoint_changed);

static int mkp_toggle_init(const struct device *dev) {
    const struct mkp_toggle_config *cfg = dev->config;
    if (cfg->toggle_mode == TOGGLE_START_ON) {
        for (int i = 0; i < MAX_MOUSE_BUTTONS; i++) {
            if (!button_state[i]) {
                zmk_hid_mouse_button_press(i);
                button_state[i] = true;
            }
        }
    }
    return 0;
}

static const struct behavior_driver_api behavior_mkp_toggle_driver_api = {
    .binding_pressed = mkp_toggle_press,
    .binding_released = mkp_toggle_release,
};

#define TOGGLE_CFG(inst)                                                                       \
    static const struct mkp_toggle_config mkp_toggle_cfg_##inst = {                            \
        .toggle_mode = DT_ENUM_IDX_OR(inst, toggle_mode, TOGGLE_START_OFF),                    \
    };                                                                                         \
    BEHAVIOR_DT_INST_DEFINE(inst, mkp_toggle_init, NULL, NULL, &mkp_toggle_cfg_##inst,         \
                            APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                  \
                            &behavior_mkp_toggle_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TOGGLE_CFG);

#else /* MKP_HAS_POINTING_CENTER */

/* Stub (peripheral or pointing disabled) */
static int mkp_toggle_press(struct zmk_behavior_binding *binding,                              \
                            struct zmk_behavior_binding_event event) { return ZMK_BEHAVIOR_OPAQUE; }
static int mkp_toggle_release(struct zmk_behavior_binding *binding,                            \
                              struct zmk_behavior_binding_event event) { return ZMK_BEHAVIOR_OPAQUE; }

static const struct behavior_driver_api behavior_mkp_toggle_driver_api = {
    .binding_pressed = mkp_toggle_press,
    .binding_released = mkp_toggle_release,
};

#define STUB_CFG(inst)                                                                            \
    BEHAVIOR_DT_INST_DEFINE(inst, NULL, NULL, NULL, NULL,                                         \
                            APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                    \
                            &behavior_mkp_toggle_driver_api);

DT_INST_FOREACH_STATUS_OKAY(STUB_CFG);

#endif /* MKP_HAS_POINTING_CENTER */
