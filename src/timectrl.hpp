/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

namespace TimeCtrl {

enum Frequency {
	VARIABLE = 0,
	FIXED_60 = 1,
	FIXED_30 = 2,
	FIXED_20 = 3,
	FIXED_15 = 4,
	FIXED_10 = 6
};

void init(Frequency freq);
void reset();
void exec();

float get_fps();
float get_motion_speed();
double get_sys_time_millis();
double get_start_time();
double get_current_time();

};