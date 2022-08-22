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

enum {
	STICK_L_X = 0,
	STICK_L_Y,
	STICK_R_X,
	STICK_R_Y,
	NUM_AXIS
};

void init();
void update();
void reset();

bool now_active(const int id);
bool was_active(const int id);
bool triggered(const int id);
bool changed(const int id);

int get_axis_val(const int axis);

};