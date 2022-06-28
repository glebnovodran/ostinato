/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

namespace InputCtrl {
// enum input buttons
enum {
	BTN_UP = 0,
	BTN_DOWN,
	BTN_LEFT,
	BTN_RIGHT,
	BTN_L1,
	BTN_L2,
	BTN_SWITCH1,
	BTN_SWITCH2,
	BTN_A,
	BTN_B,
	NUM_BUTTONS,
};

void init();
void update();

bool now_active(const int id);
bool was_active(const int id);
bool triggered(const int id);
bool changed(const int id);

};