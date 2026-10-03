#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <lvgl.h>
#include "screen_ring.h"

namespace AirDot::screen_ring {

enum class Effect : uint8_t { NONE, AIR_QUALITY, PULSE, SPINNER };

inline Effect effect_from_name(const char *name) {
  if (name == nullptr)
    return Effect::NONE;
  if (std::strcmp(name, "Air Quality") == 0)
    return Effect::AIR_QUALITY;
  if (std::strcmp(name, "Pulse") == 0)
    return Effect::PULSE;
  if (std::strcmp(name, "Spinner") == 0)
    return Effect::SPINNER;
  return Effect::NONE;
}

// Effects only ever change the arc angles. LVGL then invalidates just the slices at the arc ends,
// which matters on this panel: a full 480x480 redraw takes about 200 ms, and even redrawing only the
// band under the ring (as a whole-ring opacity animation would) takes about 100 ms per frame. An angle
// step costs ~19 ms, so the animations run at ~23 fps with LVGL busy ~42% of the time.
constexpr uint32_t PULSE_PERIOD_MS = 3000;
constexpr uint32_t SPINNER_PERIOD_MS = 3000;
constexpr float SPINNER_SWEEP = 90.0f;

// value is in tenths of a degree over two turns: the first fills the ring clockwise, the second wipes it out.
inline void pulse_exec_(void *obj, int32_t value) {
  const float angle = value / 10.0f;
  if (angle < 360.0f)
    set_sweep(static_cast<lv_obj_t *>(obj), 0.0f, angle);
  else
    set_sweep(static_cast<lv_obj_t *>(obj), angle - 360.0f, 720.0f - angle);
}

inline void spinner_exec_(void *obj, int32_t value) {
  set_sweep(static_cast<lv_obj_t *>(obj), value / 10.0f, SPINNER_SWEEP);
}

inline void stop_effects(lv_obj_t *ring) {
  if (ring == nullptr)
    return;
  lv_anim_delete(ring, pulse_exec_);
  lv_anim_delete(ring, spinner_exec_);
}

inline void start_angle_animation_(lv_obj_t *ring, lv_anim_exec_xcb_t exec, int32_t end_value, uint32_t period_ms,
                                   lv_anim_path_cb_t path) {
  lv_anim_t anim;
  lv_anim_init(&anim);
  lv_anim_set_var(&anim, ring);
  lv_anim_set_exec_cb(&anim, exec);
  lv_anim_set_values(&anim, 0, end_value);
  lv_anim_set_duration(&anim, period_ms);
  lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_path_cb(&anim, path);
  lv_anim_start(&anim);
}

inline void start_pulse(lv_obj_t *ring) {
  set_sweep(ring, 0.0f, 0.0f);
  // Stops just short of 720 so the wrap back to an empty ring does not invalidate the whole arc.
  start_angle_animation_(ring, pulse_exec_, 7199, PULSE_PERIOD_MS, lv_anim_path_ease_in_out);
}

inline void start_spinner(lv_obj_t *ring) {
  set_sweep(ring, 0.0f, SPINNER_SWEEP);
  start_angle_animation_(ring, spinner_exec_, 3599, SPINNER_PERIOD_MS, lv_anim_path_linear);
}

}  // namespace AirDot::screen_ring
