/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

namespace Ostinato {

	void init(int argc, char* argv[]);
	void reset();

	void mac_start(int argc, const char* argv[]);

	int get_def_width();
	int get_def_height();

	void update_io_gateway();
	void update_cmd_pipe();

	void set_default_lighting();
	float get_lamps_brightness();

	ScnObj* get_cam_tgt_obj();
	void set_cam_tgt(const char* pName);
} // Ostinato
