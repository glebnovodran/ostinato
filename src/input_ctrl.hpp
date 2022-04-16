/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

namespace InputCtrl {
// enum input buttons
enum {
	UP = 0,
	DOWN,
	LEFT,
	RIGHT,
	L1,
	L2,
	SWITCH1,
	SWITCH2,
	ButtonA,
	ButtonB,
	NUM_KEYS,
};
// work structure : array of structure (4 booleans + enum(^) key code)
	// input_now_active
	// ...
	// input_triggered
	// input_update

void init();

void update();

bool now_active(const int id);

bool was_active(const int id);

bool triggered(const int id);

bool changed(const int id);

};