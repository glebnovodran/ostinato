/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

#include <crosscore.hpp>
#include <scene.hpp>

#include "keyboard.hpp"
#include "camera.hpp"

struct ViewWk {
	cxVec pos;
	cxVec tgt;

	void reset() {
		pos = cxVec(0.75f, 1.3f, 3.5f);
		tgt = cxVec(0.0f, 0.95f, 0.0f);
	}
} s_view;

namespace CamCtrl {

void init() {
	s_view.reset();
}

void exec(const CamCtx* pCtx) {
	ScnObj* pTgtObj = pCtx->mpTgtObj;
	if (pTgtObj) {
		cxVec wpos = pTgtObj->get_world_pos();
		cxAABB bbox = pTgtObj->get_world_bbox();
		float cy = bbox.get_center().y;
		float yoffs = cy - bbox.get_min_pos().y;
		s_view.tgt = wpos + cxVec(0.0f, yoffs + 0.6f, 0.0f);
		cxVec offs = cxVec(1.0f, 1.0f, 6.0f);
		if (pCtx->mCamMode == 1) {
			offs = cxVec(-4.0f, 4.0f, 12.0f);
		}
		s_view.pos = s_view.tgt + offs;
	}
	Scene::set_view(s_view.pos, s_view.tgt);
}

}; // namespace