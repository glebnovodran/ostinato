/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

namespace Joystick {

void init();
void update();
void reset();

uint32_t get_num_axis();
uint32_t get_num_buttons();

int get_axis_val(const uint32_t axis);
int get_axis_old_val(const uint32_t axis);
int get_axis_diff(const uint32_t axis);

bool now_active(const int btid);
bool was_active(const int btid);
bool triggered(const int btid);
bool changed(const int btid);

}