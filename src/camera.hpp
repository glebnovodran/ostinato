/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

namespace Camera {

enum PosMode {
	NORMAL = 0,
	MOUSE,
	STICK,
	NUM_MODES
};

struct Context {
	ScnObj* mpTgtObj;
	sxGeometryData* mpZones;
	uint32_t mTgtMode;
	PosMode mPosMode;

	void next_pos_mode() {
		mPosMode = PosMode((uint32_t(mPosMode)+1) % uint32_t(NUM_MODES));
	}
};

void init();
void exec(const Context& ctx);

void set_collision(sxCollisionData* pCol);
sxCollisionData* get_collision();

} // Camera