/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

#include <crosscore.hpp>

#include "keyboard.hpp"
#include "input.hpp"

namespace InputCtrl {

struct BtnState {
	bool active;
	bool wasActive;
	bool triggered;
	bool changed;
};

struct InputWk {
	BtnState state[NUM_BUTTONS] = { {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0} };
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
} s_wk;

void init() {
	Keyboard::init();
}

void update() {
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

} // namespace