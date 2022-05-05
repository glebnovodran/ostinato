/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

namespace Camera {

struct Context {
	ScnObj* mpTgtObj;
	sxGeometryData* mpZones;
	uint32_t mTgtMode;
	uint32_t mPosMode;
};

void init();
void exec(const Context& ctx);

void set_collision(sxCollisionData* pCol);
sxCollisionData* get_collision();

};