/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

#include <crosscore.hpp>
#include <scene.hpp>

#include "timectrl.hpp"
#include "camera.hpp"

struct ViewWk {
	cxVec prevPos;
	cxVec prevTgt;
	cxVec pos;
	cxVec tgt;

	void reset() {
		pos = cxVec(0.75f, 1.3f, 3.5f);
		tgt = cxVec(0.0f, 0.95f, 0.0f);
		prevPos = pos;
		prevTgt = tgt;
	}
} s_view;

namespace Camera {

void init() {
	s_view.reset();
}

void exec(const Context& ctx) {
	ScnObj* pTgtObj = ctx.mpTgtObj;
	if (pTgtObj) {
		cxVec wpos = pTgtObj->get_world_pos();
		cxAABB bbox = pTgtObj->get_world_bbox();
		float cy = bbox.get_center().y;
		float yoffs = cy - bbox.get_min_pos().y;
		s_view.tgt = wpos + cxVec(0.0f, yoffs + 0.6f, 0.0f);
		cxVec offs = cxVec(1.0f, 1.0f, 6.0f);
		if (ctx.mCamMode == 1) {
			offs = cxVec(-4.0f, 4.0f, 12.0f);
		}
		s_view.pos = s_view.tgt + offs;

		if (Scene::get_frame_count() > 0) {
			int t = TimeCtrl::adjust_counter_to_speed(40);
			s_view.pos = nxCalc::approach(s_view.prevPos, s_view.pos, t);
			t = TimeCtrl::adjust_counter_to_speed(30);
			s_view.tgt = nxCalc::approach(s_view.prevTgt, s_view.tgt, t);
		}
		s_view.prevPos = s_view.pos;
		s_view.prevTgt = s_view.tgt;
	}
	Scene::set_view(s_view.pos, s_view.tgt);
}

}; // namespace