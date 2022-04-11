/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

#include <crosscore.hpp>
#include <scene.hpp>
#include <smprig.hpp>

#include "timectrl.hpp"
#include "humansys.hpp"

void Human::Rig::init(Human* pHuman) {
	mpHuman = nullptr;
	mpObj = nullptr;
	if (!pHuman) return;
	ScnObj* pObj = pHuman->mpObj;
	if (!pObj) return;
	mpHuman = pHuman;
	sxValuesData* pVals = pObj->find_values("params");
	SmpRig::init(pObj, pVals);
}

void Human::Rig::exec() {
	Human* pHuman = mpHuman;
	if (!pHuman) return;
	sxCollisionData* pCol = HumanSys::get_collision();
	SmpRig::exec(pCol);
}

void Human::MotLib::init(const Pkg* pPkg, const Pkg* pBasePkg) {
	struct {
		sxMotionData** ppMot;
		const char* pName;
	} tbl[] = {
		{ &pStand, "stand" },
		{ &pTurnL, "turn_l" },
		{ &pTurnR, "turn_r" },
		{ &pWalk, "walk" },
		{ &pRetreat, "retreat" },
		{ &pRun, "run" }
	};
	for (int i = 0; i < XD_ARY_LEN(tbl); ++i) {
		*tbl[i].ppMot = pPkg ? pPkg->find_motion(tbl[i].pName) : nullptr;
		if (*tbl[i].ppMot == nullptr) {
			*tbl[i].ppMot = pBasePkg ? pBasePkg->find_motion(tbl[i].pName) : nullptr;
		}
	}
}

void Human::change_act(const Action newAct, const double durationSecs, const int blendCnt) {
	if (!mpObj) return;
	//int blendCnt = 15;
	float mspeed = TimeCtrl::get_motion_speed();
	float t = nxCalc::div0(float(blendCnt), TimeCtrl::get_motion_speed());
	mpObj->init_motion_blend(int(t));
	mAction = newAct;
	mActionTimer.start(durationSecs);
}

void Human::add_deg_y(const float dy) {
	if (!mpObj) return;
	mpObj->add_world_deg_y(dy * TimeCtrl::get_motion_speed());
}

void Human::ctrl() {
	if (get_state() == Human::INIT) {
		mActionTimer.fix_up_time();
		change_state(Human::ACTIVE);
	}
	if (mCtrlFunc) { mCtrlFunc(this); }
}
static bool human_obj_adj_for_each(ScnObj* pObj, void* pHumanMem) {
	if (!pObj) return true;
	Human* pHuman = (Human*)pHumanMem;
	if (pHuman->mpObj == pObj) return true;
	float rchar = pHuman->mpObj->mObjAdjRadius;
	float robj = pObj->mObjAdjRadius;
	if (!(rchar > 0.0f && robj > 0.0f)) return true;
	cxVec objWPos = pObj->get_prev_world_pos();
	cxVec objPos = objWPos;
	cxVec wpos = pHuman->mpObj->get_world_pos();
	cxVec npos = wpos;
	cxVec opos = pHuman->mpObj->get_prev_world_pos();
	npos.y += pHuman->mpObj->mObjAdjYOffs;
	opos.y += pHuman->mpObj->mObjAdjYOffs;
	objPos.y += pObj->mObjAdjYOffs;
	cxVec adjPos = npos;
	bool adjFlg = Scene::sph_sph_adj(npos, opos, rchar, objPos, robj, &adjPos);
	if (adjFlg) {
		pHuman->mpObj->set_skel_root_local_tx(adjPos.x);
		pHuman->mpObj->set_skel_root_local_tz(adjPos.z);
		++pHuman->mObjAdjCount;
	}
	return true;
}

void Human::obj_adj() {
	if (!mpObj) return;
	mObjAdjCount = 0;
	Scene::for_each_obj(human_obj_adj_for_each, this);
	if (mObjAdjCount > 0) {
		++mObjTouchCount;
	} else {
		mObjTouchCount = 0;
	}
}

void Human::wall_adj() {
	if (!mpObj) return;
	sxCollisionData* pCol = HumanSys::get_collision();
	if (!pCol) {
		mWallTouchCount = 0;
		return;
	}
	cxVec wpos = mpObj->get_world_pos();
	cxVec npos = wpos;
	cxVec opos = mpObj->get_prev_world_pos();
	cxVec apos = wpos;
	float yoffs = mpObj->mObjAdjYOffs;
	npos.y += yoffs;
	opos.y += yoffs;
	float radius = mpObj->mObjAdjRadius * 1.1f;
	bool touchFlg = Scene::wall_touch(mpObj->mpJobCtx, pCol, npos, opos, radius * 1.025f);
	if (touchFlg) {
		++mWallTouchCount;
	} else {
		mWallTouchCount = 0;
	}
	bool adjFlg = Scene::wall_adj(mpObj->mpJobCtx, pCol, npos, opos, radius, &apos);
	if (adjFlg) {
		mpObj->set_skel_root_local_pos(apos.x, wpos.y, apos.z);
	}
}

void Human::ground_adj() {
	ScnObj* pObj = mpObj;
	if (!pObj) return;
	sxCollisionData* pCol = HumanSys::get_collision();
	if (!pCol) return;
	cxVec wpos = pObj->get_world_pos();
	float y = Scene::get_ground_height(pCol, wpos);
	pObj->set_skel_root_local_ty(y);
}

void Human::exec_collision() {
	if (!mpObj) return;
	bool objTouchOngoing = mObjTouchCount > 0;
	bool wallTouchOngoing = mWallTouchCount > 0;
	obj_adj();
	wall_adj();
	ground_adj();
	if (mObjTouchCount > 0) {
		if (objTouchOngoing) {
			mObjTouchDuration = TimeCtrl::get_current_time() - mObjTouchStartTime;
		} else {
			mObjTouchStartTime = TimeCtrl::get_current_time();
			mObjTouchDuration = 0.0f;
		}
	} else {
		mObjTouchDuration = 0.0f;
	}
	if (mWallTouchCount > 0) {
		if (wallTouchOngoing) {
			mWallTouchDuration = TimeCtrl::get_current_time() - mWallTouchStartTime;
		} else {
			mWallTouchStartTime = TimeCtrl::get_current_time();
			mWallTouchDuration = 0.0f;
		}
	} else {
		mWallTouchDuration = 0.0f;
	}

}

namespace HumanSys {

static char get_resource_code(Human::Type type) {
	static const char htc[Human::Type::MAX + 1] = {'f', 'm', ' '};
	return htc[type];
}

static struct HUMAN_WK {

	static const int BODY_VAR_NUM = 10;

	struct ResidentRsrc {
		Pkg* mBasePkg[Human::Type::MAX];
	} mResident;
	struct TransientRsrc {
		Pkg* mVariPkg[Human::Type::MAX][BODY_VAR_NUM];
	} mTransient;
	sxCollisionData* mpCol;
	int mCount;
	bool mFixedFreq;
	bool mLowQ;


	Pkg* get_base_pkg(const Human::Type type) {
		return mResident.mBasePkg[type];
	}

	Pkg* get_pkg(const Human::Type type, uint32_t vari) {
		return vari < BODY_VAR_NUM ? mTransient.mVariPkg[type][vari] : nullptr;
	}

	void init_base() {
		char buff[6];
		for (int i = 0; i < Human::Type::MAX; ++i) {
			char htc = get_resource_code(Human::Type(i));
			XD_SPRINTF(XD_SPRINTF_BUF(buff, 5), "npc_%c", htc);
			mResident.mBasePkg[i] = Scene::load_pkg(buff);
		}
	}

	void init_transient() {
		char buff[10];
		for (int i = 0; i < Human::Type::MAX; ++i) {
			char htc = get_resource_code(Human::Type(i));
			for (int j = 0; j < BODY_VAR_NUM; ++j) {
				XD_SPRINTF(XD_SPRINTF_BUF(buff, 10), "npc_%c%02u", htc, j);
				mTransient.mVariPkg[i][j] = Scene::load_pkg(buff);
			}
		}
	}

	void init() {
		mFixedFreq = nxApp::get_bool_opt("fixfreq", false);
		mLowQ = nxApp::get_bool_opt("lowq", false);
		mCount = 0;
		init_base();
		init_transient();
	}
	void reset() {}
} s_wk = {};

static bool s_initFlg = false;
static const char* s_pHumanTag = "Human";
void init() {
	if (s_initFlg) return;
	s_wk.init();
	s_initFlg = true;
}

void reset() {
	if (!s_initFlg) return;
	s_wk.reset();
	s_initFlg = false;
}

bool obj_is_human(ScnObj* pObj) {
	bool res = false;
	if (pObj) {
		if (pObj->get_ptr_wk<void>(0)) {
			const char* pTag = pObj->get_ptr_wk<const char>(1);
			if (pTag) {
				res = (pTag == s_pHumanTag);
			}
		}
	}
	return res;
}

Human* get_human(ScnObj* pObj) {
	return obj_is_human(pObj) ? pObj->get_ptr_wk<Human>(0) : nullptr;
}

static void human_before_mot_func(ScnObj* pObj) {
	Human* pHuman = get_human(pObj);
	if (!pHuman) return;
	pHuman->select_motion();
}

static void human_exec_func(ScnObj* pObj) {
	Human* pHuman = get_human(pObj);
	if (!pHuman) return;
	pHuman->ctrl();
	float fps = pObj->mpMoveMot ? pObj->mpMoveMot->mFPS : 0.0f;
	float frameAdd = (fps / 60.0f) * TimeCtrl::get_motion_speed();
	if (frameAdd > 0.0f) {
		pObj->move(frameAdd);
	}
}

static void human_del_func(ScnObj* pObj) {
	Human* pHuman = get_human(pObj);
	if (!pHuman) return;
	nxCore::tMem<Human>::free(pHuman);
	pObj->set_ptr_wk<void>(0, nullptr);
	pObj->set_ptr_wk<void>(1, nullptr);
}

static void human_before_blend_func(ScnObj* pObj) {
	Human* pHuman = get_human(pObj);
	if (!pHuman) return;
	pHuman->exec_collision();
	pHuman->mRig.exec();
}

ScnObj* add_human(const Human::Descr& descr, Human::CtrlFunc ctrl) {
	ScnObj* pObj = nullptr;
	if (s_initFlg) {
		int bodyId = descr.bodyVariation % HUMAN_WK::BODY_VAR_NUM;
		bodyId  = nxCalc::clamp(bodyId, 0, bodyId);
		Pkg* pPkg = s_wk.get_pkg(descr.type, bodyId);
		if (pPkg) {
			sxModelData* pMdl = pPkg->get_default_model();
			if (pMdl) {
				char nameBuf[64];
				const char* pName = descr.pName;
				if (!pName) {
					XD_SPRINTF(XD_SPRINTF_BUF(nameBuf, sizeof(nameBuf)), "%s@%03d", pPkg->get_name(), s_wk.mCount);
					pName = nameBuf;
				}
				pObj = Scene::add_obj(pMdl, pName);
			}
		}
		if (pObj) {
			float scale = descr.scale > 0.0f ? descr.scale : 1.0f;
			pObj->set_motion_uniform_scl(scale);
			pObj->set_model_variation(descr.mtlVariation);
			#if 0
			if (s_wk.mLowQ) {
				pObj->mDisableShadowRecv = true;
			}
			#else
			pObj->mDisableShadowRecv = true;
			pObj->mDisableShadowCast = false;
			#endif
			Human* pHuman = nxCore::tMem<Human>::alloc(1, s_pHumanTag);
			if (pHuman) {
				Pkg* pBasePkg = s_wk.get_base_pkg(descr.type);
				pObj->set_texture_pkg(pBasePkg);
				pObj->set_ptr_wk(0, pHuman);
				pObj->set_ptr_wk(1, s_pHumanTag);
				pHuman->mpObj = pObj;

				pHuman->mMotLib.init(pPkg, pBasePkg);
				pHuman->mRig.init(pHuman);
				//pHuman->mRig.mScale = scale;

				pObj->mExecFunc = human_exec_func;
				pObj->mDelFunc = human_del_func;
				pObj->mBeforeMotionFunc = human_before_mot_func;
				pObj->mBeforeBlendFunc = human_before_blend_func;

				pObj->mObjAdjRadius = 0.26f * scale;
				pObj->mObjAdjYOffs = 1.0f * scale;
				pObj->mRoutine[0] = 0;
				pObj->mFltWk[0] = 0.0f;
				pHuman->mObjTouchCount = 0;
				pHuman->mObjTouchStartTime = 0.0;
				pHuman->mObjTouchDuration = 0.0;
				pHuman->mWallTouchCount = 0;
				pHuman->mWallTouchStartTime = 0.0;
				pHuman->mWallTouchDuration = 0.0;

				pHuman->mCtrlFunc = ctrl;

				pHuman->mAction = Human::ACT_STAND;
				pHuman->set_motion();
				pHuman->mActionTimer.mStartTime = s_wk.mFixedFreq ? 0.0 : TimeCtrl::get_sys_time_millis();
				pHuman->mActionTimer.set_duration_seconds(0.5f);

				++s_wk.mCount;
			}
		}
	}
	return pObj;
}

void set_collision(sxCollisionData* pCol) {
	s_wk.mpCol = pCol;
}

sxCollisionData* get_collision() {
	return s_wk.mpCol;
}

};