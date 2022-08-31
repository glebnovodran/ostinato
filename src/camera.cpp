/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

#include <crosscore.hpp>
#include <scene.hpp>
#include <oglsys.hpp>

#include "input.hpp"
#include "timectrl.hpp"
#include "camera.hpp"

class CamZonesHitFunc : public sxGeometryData::HitFunc {
public:
	int mPolId;

	CamZonesHitFunc() : mPolId(-1) {}

	virtual bool operator()(const sxGeometryData::Polygon& pol, const cxVec& hitPos, const cxVec& hitNrm, float hitDist) {
		mPolId = pol.get_id();
		return false; /* any pol for now */
	}
};

struct ViewWk {
	sxQuatTrackball mTrackball;
	sxCollisionData* mpCol;
	cxVec mTgtOffs;
	float mRadius;
	float mDist;
	float mOrgX;
	float mOrgY;
	float mFixedCenterY;
	bool mFixCenterY;

	cxVec mPrevPos;
	cxVec mPrevTgt;
	cxVec mPos;
	cxVec mTgt;
	int mLastPolId;
	int mCnt;
	uint32_t mPosMode;

	void init() {
		mRadius = 0.5f;
		mDist = 7.0f;
		mTrackball.init();
		mTgtOffs.set(0.0f, 0.0f, 0.0f);
		mFixedCenterY = 1.0f;
		mFixCenterY = false;

		mPos = cxVec(0.75f, 1.3f, 3.5f);
		mTgt = cxVec(0.0f, 0.95f, 0.0f);
		mPrevPos = mPos;
		mPrevTgt = mTgt;
		mLastPolId = -1;
		mCnt = 0;
		mPosMode = 0;
	}

	void set_collision(sxCollisionData* pCol) {
		mpCol = pCol;
	}

	sxCollisionData* get_collision() { return mpCol; }

	void update(const OGLSysInput* pInp) {
		OGLSysMouseState state = OGLSys::get_mouse_state();
		if (state.ck_now(OGLSysMouseState::OGLSYS_BTN_LEFT)) {
			if (pInp) {
				if (pInp->act == OGLSysInput::OGLSYS_ACT_DOWN) {
					mOrgX = state.mNowX;
					mOrgY = state.mNowY;
				}
			} else {
				if (!state.ck_old(OGLSysMouseState::OGLSYS_BTN_LEFT)) {
					mOrgX = state.mOldX;
					mOrgY = state.mOldY;
				}
			}
			float dx = state.mNowX - state.mOldX;
			float dy = state.mNowY - state.mOldY;
			if (dx || dy) {
				float x0 = state.mOldX - mOrgX;
				float y0 = mOrgY - state.mOldY;
				float x1 = state.mNowX - mOrgX;
				float y1 = mOrgY - state.mNowY;
				mTrackball.update(x0, y0, x1, y1, mRadius);
			}
		}
	}

	void joystick_update() {
		if (mPosMode == 1) {
			xt_float2 val = InputCtrl::get_stick_values(1);
			val.scl(XD_DEG2RAD(1.0f));
			cxQuat qx, qy, rot;
			qy.set_rot_y(val.x);
			qx.set_rot_x(val.y);

			rot = qy * qx;
			mTrackball.mQuat.mul(rot);
		}
	}

	void set_quat(const cxQuat q) {
		mTrackball.mQuat = q;
	}

	void exec(const Camera::Context& ctx) {
		ScnObj* pTgtObj = ctx.mpTgtObj;
		mPosMode = ctx.mPosMode;
		bool zoneFlg = false;
		cxVec up = nxVec::get_axis(exAxis::PLUS_Y);

		if (pTgtObj) {
			cxVec wpos = pTgtObj->get_world_pos();

			if (mPosMode == 0) {
				if (ctx.mpZones) {
					CamZonesHitFunc zonesHit;
					cxLineSeg zonesSeg(wpos + cxVec(0.0f, 1.0f, 0.0f), wpos - cxVec(0.0f, 0.5f, 0.0f));
					ctx.mpZones->hit_query(zonesSeg, zonesHit);
					zoneFlg = zonesHit.mPolId >= 0;
					bool zoneChange = (mLastPolId ^ zonesHit.mPolId) < 0;
					mLastPolId = zonesHit.mPolId;
					if (zoneChange) {
						mCnt = 30;
					}
				}
			}

			cxAABB bbox = pTgtObj->get_world_bbox();
			float cy = bbox.get_center().y;
			float yoffs = cy - bbox.get_min_pos().y;
			mTgt = wpos + cxVec(0.0f, yoffs + 0.6f, 0.0f);
			cxVec offs;
			if (mPosMode == 0) {
				if (ctx.mTgtMode == 0) {
					offs = zoneFlg? cxVec(2.0f, -1.0f, 5.0f) : cxVec(-1.0f, -1.0f, -6.0f);
				} else {
					offs = zoneFlg? cxVec(5.0f, -3.8f, 9.0f) : cxVec(4.0f, -4.0f, -12.0f);
				}
			} else {
				offs = mTrackball.calc_dir(-mDist);
			}

			mPos = mTgt - offs;

			if (mPosMode == 1) {
				up = mTrackball.calc_up();
			}

			if (mPosMode == 0) {
				float factor = 1.0f;
				if (mCnt > 0) {
					factor = 10.0f;
					--mCnt;
				}
				if (Scene::get_frame_count() > 0) {
					int t = int(nxCalc::div0(40.0f * factor, TimeCtrl::get_motion_speed()));
					mPos = nxCalc::approach(mPrevPos, mPos, t);
					t = int(nxCalc::div0(10.0f, TimeCtrl::get_motion_speed()));
					mTgt = nxCalc::approach(mPrevTgt, mTgt, t);
				}
				mPrevPos = mPos;
				mPrevTgt = mTgt;
			}
		}

		sxCollisionData* pCol = get_collision();
		if (pCol) {
			sxCollisionData::NearestHit stgHit = pCol->nearest_hit(cxLineSeg(mTgt, mPos));
			if (stgHit.count > 0) {
				mPos = stgHit.pos;
				if (nxVec::dist(mPos, mTgt) < 2.0f) {
					mPos.y += 1.7f;
				}
			}
		}
		Scene::set_view(mPos, mTgt, up);
	}

} s_viewWk;

static void input_handler(const OGLSysInput& inp, void* pWk) {
	if (s_viewWk.mPosMode) {
		if (inp.id == 0) {
			s_viewWk.update(&inp);
		}
	}
}

namespace Camera {

void init() {
	s_viewWk.init();
	OGLSys::set_input_handler(input_handler, &s_viewWk);
}

void exec(const Context& ctx) {
	s_viewWk.joystick_update();
	s_viewWk.exec(ctx);
}

void set_collision(sxCollisionData* pCol) {
	s_viewWk.set_collision(pCol);
}

sxCollisionData* get_collision() { return s_viewWk.get_collision(); }

} // namespace
