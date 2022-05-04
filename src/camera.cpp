/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

#include <crosscore.hpp>
#include <scene.hpp>
#include <oglsys.hpp>

#include "timectrl.hpp"
#include "camera.hpp"

struct TrackBall {
	cxQuat mSpin;
	cxQuat mQuat;

	void init() {
		mSpin.identity();
		mQuat.identity();
	}

	void update(const float x0, const float y0, const float x1, const float y1, float const radius = 0.5f) {
		cxVec tp0 = project(x0, y0, radius);
		cxVec tp1 = project(x1, y1, radius);
		cxVec dir = tp0 - tp1;
		cxVec axis = nxVec::cross(tp1, tp0);
		float t = nxCalc::clamp(dir.mag() / (2.0f*radius), -1.0f, 1.0f);
		float ang = 2.0f * mth_asinf(t);
		mSpin.set_rot(axis, ang);
		mQuat.mul(mSpin);
		mQuat.normalize();
	}

	static cxVec project(float x, float y, float r) {
		float d = nxCalc::hypot(x, y);
		float t = r / mth_sqrtf(2.0f);
		float z;
		if (d < t) {
			z = mth_sqrtf(r*r - d*d);
		} else {
			z = (t*t) / d;
		}
		return cxVec(x, y, z);
	}
};

struct ViewWk {
	TrackBall mTrackBall;
	float mOrgX;
	float mOrgY;

	cxVec mPrevPos;
	cxVec mPrevTgt;
	cxVec mPos;
	cxVec mTgt;
	int mLastPolId;
	int mCnt;
	uint32_t mPosMode;

	void init() {
		mTrackBall.init();

		mPos = cxVec(0.75f, 1.3f, 3.5f);
		mTgt = cxVec(0.0f, 0.95f, 0.0f);
		mPrevPos = mPos;
		mPrevTgt = mTgt;
		mLastPolId = -1;
		mCnt = 0;
		mPosMode = 0;
	}

	void update(const OGLSysInput& inp) {
		OGLSysMouseState state = OGLSys::get_mouse_state();

		if (state.ck_now(OGLSysMouseState::OGLSYS_BTN_LEFT)) {
			if (inp.act == OGLSysInput::OGLSYS_ACT_DOWN) {
				mOrgX = state.mNowX;
				mOrgY = state.mNowY;
			}
			float dx = state.mNowX - state.mOldX;
			float dy = state.mNowY - state.mOldY;
			if (dx || dy) {
				float x0 = state.mOldX - mOrgX;
				float y0 = mOrgY - state.mOldY;
				float x1 = state.mNowX - mOrgX;
				float y1 = mOrgY - state.mNowY;
				mTrackBall.update(x0, y0, x1, y1); // radius == 0.5
				//nxCore::dbg_msg("[DEBUG] track ball update : %f, %f, %f, %f\n", x0, y0, x1, y1);
			}
		}
	}
} s_view;

static void input_handler(const OGLSysInput& inp, void* pWk) {
	if (s_view.mPosMode) {
		if (inp.id == 0) {
			s_view.update(inp);
		}
	}
}

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
	s_view.init();
	OGLSys::set_input_handler(input_handler, &s_view);
}

void exec(const Context& ctx) {
	ScnObj* pTgtObj = ctx.mpTgtObj;
	s_view.mPosMode = ctx.mPosMode;
	bool zoneFlg = false;
	cxVec up = nxVec::get_axis(exAxis::PLUS_Y);

	if (pTgtObj) {
		cxVec wpos = pTgtObj->get_world_pos();

		if (s_view.mPosMode == 0) {
			if (ctx.mpZones) {
				CamZonesHitFunc zonesHit;
				cxLineSeg zonesSeg(wpos + cxVec(0.0f, 1.0f, 0.0f), wpos - cxVec(0.0f, 0.5f, 0.0f));
				ctx.mpZones->hit_query(zonesSeg, zonesHit);
				zoneFlg = zonesHit.mPolId >= 0;
				bool zoneChange = (s_view.mLastPolId ^ zonesHit.mPolId) < 0;
				s_view.mLastPolId = zonesHit.mPolId;
				if (zoneChange) {
					s_view.mCnt = 30;
				}
			}
		}

		cxAABB bbox = pTgtObj->get_world_bbox();
		float cy = bbox.get_center().y;
		float yoffs = cy - bbox.get_min_pos().y;
		s_view.mTgt = wpos + cxVec(0.0f, yoffs + 0.6f, 0.0f);
		cxVec offs;
		if (s_view.mPosMode == 0) {
			if (ctx.mTgtMode == 0) {
				offs = zoneFlg? cxVec(2.0f, -1.0f, 5.0f) : cxVec(-1.0f, -1.0f, -6.0f);
			} else {
				offs = zoneFlg? cxVec(5.0f, -3.8f, 9.0f) : cxVec(4.0f, -4.0f, -12.0f);
			}
		} else {
			float dist = ctx.mTgtMode == 0 ? 7.0f : 14.0f;
			offs = s_view.mTrackBall.mQuat.apply(nxVec::get_axis(exAxis::MINUS_Z) * dist);
		}

		s_view.mPos = s_view.mTgt - offs;

		if (s_view.mPosMode == 1) {
			up = s_view.mTrackBall.mQuat.apply(up);
		}

		if (s_view.mPosMode == 0) {
			float factor = 1.0f;
			if (s_view.mCnt > 0) {
				factor = 10.0f;
				--s_view.mCnt;
			}
			if (Scene::get_frame_count() > 0) {
				int t = int(nxCalc::div0(40.0f * factor, TimeCtrl::get_motion_speed()));
				s_view.mPos = nxCalc::approach(s_view.mPrevPos, s_view.mPos, t);
				t = int(nxCalc::div0(10.0f, TimeCtrl::get_motion_speed()));
				s_view.mTgt = nxCalc::approach(s_view.mPrevTgt, s_view.mTgt, t);
			}
			s_view.mPrevPos = s_view.mPos;
			s_view.mPrevTgt = s_view.mTgt;
		}
	}

	Scene::set_view(s_view.mPos, s_view.mTgt, up);
}

}; // namespace