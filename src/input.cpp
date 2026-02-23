/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

#include <crosscore.hpp>

#include "keyboard.hpp"
#include "joystick.hpp"
#include "input.hpp"

namespace InputCtrl {

struct BtnState {
	bool active;
	bool wasActive;
	bool triggered;
	bool changed;
};

struct InputWk {
	BtnState state[NUM_BUTTONS] = {};
	int keymap[NUM_BUTTONS] = { 
		Keyboard::UP,
		Keyboard::DOWN,
		Keyboard::LEFT,
		Keyboard::RIGHT,
		Keyboard::LCTRL,
		Keyboard::LSHIFT,
		Keyboard::TAB,
		Keyboard::BACK,
		Keyboard::ENTER,
		Keyboard::SPACE
	};
	bool active;
} s_wk;

void init() {
	s_wk.active = false;
	Keyboard::init();
	Joystick::init();
}

void reset() {
	Joystick::reset();
}

void update() {
	if (!s_wk.active) {
		s_wk.active = true;
		return;
	}

	Joystick::update();
	Keyboard::update();
	for (int i = 0; i < NUM_BUTTONS; ++i) {
		int keyId = s_wk.keymap[i];
		if (keyId >= 0) {
			s_wk.state[i].active = Keyboard::now_active(keyId);
			s_wk.state[i].wasActive = Keyboard::was_active(keyId);
			s_wk.state[i].triggered = Keyboard::triggered(keyId);
			s_wk.state[i].changed = Keyboard::changed(keyId);
		}
	}
}

bool now_active(const int id) { return s_wk.state[id].active; }

bool was_active(const int id){ return s_wk.state[id].wasActive; }

bool triggered(const int id){ return s_wk.state[id].triggered; }

bool changed(const int id){ return s_wk.state[id].changed; }

bool joystick_active() {
	return Joystick::active();
}

float get_axis_val(const uint32_t  axis) {
	int val = Joystick::get_axis_val(axis);
	return nxCalc::fit(float(val), -32767.0f, 32767.0f, -1.0f, 1.0f);
}

float get_axis_old_val(const uint32_t  axis) {
	int val = Joystick::get_axis_old_val(axis);
	return nxCalc::fit(float(val), -32767.0f, 32767.0f, -1.0f, 1.0f);
}

xt_float2 get_stick_vals(const uint32_t stick) {
	xt_float2 res;
	res.x = get_axis_val(stick * 2);
	res.y = get_axis_val(stick * 2 + 1);
	return res;
}

xt_float2 get_stick_old_vals(const uint32_t stick) {
	xt_float2 res;
	res.x = get_axis_old_val(stick * 2);
	res.y = get_axis_old_val(stick * 2 + 1);
	return res;
}

} // InputCtrl