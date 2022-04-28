/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

namespace TimeCtrl {

enum Frequency {
	VARIABLE = 0,
	FIXED_60,
	FIXED_30,
	NUM_MODES
};

void init();
void reset();
void exec();

float get_fps();
float get_motion_speed();
double get_sys_time_millis();
double get_start_time();
double get_current_time();

};