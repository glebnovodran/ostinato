/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

namespace Joystick {

void init();
void update();
void reset();

int get_num_axis();
int get_num_buttons();

int get_axis_val(unsigned char axis);

bool now_active(const int btid);
bool was_active(const int btid);
bool triggered(const int btid);
bool changed(const int btid);

}