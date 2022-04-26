/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

namespace Camera {

struct Context {
	ScnObj* mpTgtObj;
	uint32_t mCamMode;
	sxGeometryData* mpZones;
};
void init();
void exec(const Context& ctx);

};