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
	int lastPolId;
	int cnt;

	void reset() {
		pos = cxVec(0.75f, 1.3f, 3.5f);
		tgt = cxVec(0.0f, 0.95f, 0.0f);
		prevPos = pos;
		prevTgt = tgt;
		lastPolId = -1;
		cnt = 0;
	}
} s_view;

namespace Camera {

class CamZonesHitFunc : public sxGeometryData::HitFunc {
public:
	int mPolId;

	CamZonesHitFunc() : mPolId(-1) {}

	virtual bool operator()(const sxGeometryData::Polygon& pol, const cxVec& hitPos, const cxVec& hitNrm, float hitDist) {
		mPolId = pol.get_id();
		return false; /* any pol for now */
	}
};

void init() {
	s_view.reset();
}

void exec(const Context& ctx) {
	ScnObj* pTgtObj = ctx.mpTgtObj;
	bool zoneFlg = false;

	if (pTgtObj) {
		cxVec wpos = pTgtObj->get_world_pos();

		if (ctx.mpZones) {
			CamZonesHitFunc zonesHit;
			cxLineSeg zonesSeg(wpos + cxVec(0.0f, 1.0f, 0.0f), wpos - cxVec(0.0f, 0.5f, 0.0f));
			ctx.mpZones->hit_query(zonesSeg, zonesHit);
			zoneFlg = zonesHit.mPolId >= 0;
			bool zoneChange = (s_view.lastPolId ^ zonesHit.mPolId) < 0;
			s_view.lastPolId = zonesHit.mPolId;
			if (zoneChange) {
				s_view.cnt = 30;
			}

		}

		cxAABB bbox = pTgtObj->get_world_bbox();
		float cy = bbox.get_center().y;
		float yoffs = cy - bbox.get_min_pos().y;
		s_view.tgt = wpos + cxVec(0.0f, yoffs + 0.6f, 0.0f);
		cxVec offs = cxVec(1.0f, 1.0f, 6.0f);
		if (zoneFlg) {
			offs = cxVec(-2.0f, 1.0f, -5.0f);
		}

		if (ctx.mCamMode == 1) {
			offs = cxVec(-4.0f, 4.0f, 12.0f);
		}
		s_view.pos = s_view.tgt + offs;

		float factor = 1.0f;
		if (s_view.cnt > 0) {
			factor = 10.0f;
			--s_view.cnt;
		}
		if (Scene::get_frame_count() > 0) {
			int t = int(nxCalc::div0(40.0f * factor, TimeCtrl::get_motion_speed()));
			s_view.pos = nxCalc::approach(s_view.prevPos, s_view.pos, t);
			t = int(nxCalc::div0(10.0f, TimeCtrl::get_motion_speed()));
			s_view.tgt = nxCalc::approach(s_view.prevTgt, s_view.tgt, t);
		}
		s_view.prevPos = s_view.pos;
		s_view.prevTgt = s_view.tgt;
	}
	Scene::set_view(s_view.pos, s_view.tgt);

	if (s_view.cnt > 0) { --s_view.cnt; }
}

}; // namespace