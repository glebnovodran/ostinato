/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

namespace Keyboard {

enum {
	UP = 0,
	DOWN,
	LEFT,
	RIGHT,
	LCTRL,// L1
	LSHIFT,//L2
	TAB,//SWITCH1
	BACK,//SWITCH2
	ENTER,//ButtonA
	SPACE,//ButtonB
	NUM_KEYS
};

void init();
void update();

bool now_active(int id);
bool was_active(int id);
bool triggered(int id);
bool changed(int id);

}