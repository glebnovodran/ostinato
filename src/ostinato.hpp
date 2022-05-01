/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

namespace Ostinato {

	void init(int argc, char* argv[]);

	void reset();

	void set_default_lightning();

	ScnObj* get_cam_tgt_obj();
	void set_cam_tgt(const char* pName);

}; // namespace