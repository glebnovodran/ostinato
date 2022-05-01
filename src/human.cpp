/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

#include <crosscore.hpp>
#include <scene.hpp>
#include <smprig.hpp>

#include "timectrl.hpp"
#include "human.hpp"

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
	for (size_t i = 0; i < XD_ARY_LEN(tbl); ++i) {
		*tbl[i].ppMot = pPkg ? pPkg->find_motion(tbl[i].pName) : nullptr;
		if (*tbl[i].ppMot == nullptr) {
			*tbl[i].ppMot = pBasePkg ? pBasePkg->find_motion(tbl[i].pName) : nullptr;
		}
	}
}

uint32_t Human::find_nearest_mot_frame(const sxMotionData* pMtd, const char* pNodeName) const {
	int nearFrame = 0;
	ScnObj* pObj = this->mpObj;
	if (pObj && pObj->mpMdlWk && pObj->mpMotWk && pNodeName && pMtd) {
		cxMotionWork* pMotWk = pObj ? pObj->mpMotWk : nullptr;
		int nodeId = pObj->find_skel_node_id(pNodeName);
		int centerId = pObj->find_skel_node_id("n_Center");
		if (pObj->ck_skel_id(nodeId) && pObj->ck_skel_id(centerId)) {
			xt_xmtx skelXform = pObj->mpMdlWk->mpData->calc_skel_node_chain_xform(nodeId, centerId, pObj->mpMotWk->mpXformsL);
			cxVec skelPos = nxMtx::xmtx_get_pos(skelXform);
			int frame = 0;
			float nearDist = 0.0f;
			for (uint32_t i = 0; i < pMtd->mFrameNum - 2; i++) {
				xt_xmtx motXform = pMotWk->eval_skel_node_chain_xform(pMtd, nodeId, centerId, float(i));
				cxVec motPos = nxMtx::xmtx_get_pos(motXform);
				float dist = nxVec::dist(skelPos, motPos);
				if (i == 0) {
					nearDist = dist;
				} else {
					if (dist < nearDist) {
						nearDist = dist;
						frame = i;
					}
				}
			}
			nearFrame = frame;
		}
	}
	return nearFrame;
}

void Human::change_act(const Action newAct, const double durationSecs, const int blendCnt, const int startFrame) {
	if (!mpObj) return;
	float t = nxCalc::div0(float(blendCnt), TimeCtrl::get_motion_speed());
	mpObj->set_motion_frame(startFrame);
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
		sxCollisionData* pCol = HumanSys::get_collision();
		if (pCol) {
			sxCollisionData::NearestHit stgHit = pCol->nearest_hit(cxLineSeg(opos, adjPos));
			if (stgHit.count > 0) {
				nxCore::dbg_msg("[BREAK] %s made an attempt to escape out of stage!\n", pHuman->get_name());
				adjFlg = false;
			}
		}
		if (adjFlg) {
			pHuman->mpObj->set_skel_root_local_tx(adjPos.x);
			pHuman->mpObj->set_skel_root_local_tz(adjPos.z);
			++pHuman->mObjAdjCount;
		}
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
	ground_adj();
	obj_adj();
	wall_adj();

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

static char get_resource_code(Human::Kind type) {
	static const char htc[Human::Kind::MAX_KIND + 1] = {'f', 'm', ' '};
	return htc[type];
}

static struct HumanWk {

	static const int IDENTITIES_NUM = 10;

	struct ResidentRsrc {
		Pkg* mBasePkg[Human::Kind::MAX_KIND];
		Pkg* mPlr[Human::Kind::MAX_KIND];
	} mResident;
	struct TransientRsrc {
		Pkg* mVariPkg[Human::Kind::MAX_KIND][IDENTITIES_NUM];
	} mTransient;
	sxCollisionData* mpCol;
	int mCount;
	bool mFixedFreq;
	bool mLowQ;


	Pkg* get_base_pkg(const Human::Type type) {
		Pkg* pBasePkg = nullptr;
		switch (type) {
			case Human::CITIZEN_FEMALE:
			case Human::TRAVELLER:
				pBasePkg = mResident.mBasePkg[0]; // base female
				break;
			case Human::CITIZEN_MALE:
			case Human::WANDERER:
				pBasePkg = mResident.mBasePkg[1]; // basemale
				break;
			default:
				break;
		}
		return pBasePkg;
	}

	Pkg* get_pkg(const Human::Type type, uint32_t vari) {
		Pkg* pPkg = nullptr;
		switch (type) {
			case Human::CITIZEN_FEMALE:
				pPkg = (vari < IDENTITIES_NUM) ? mTransient.mVariPkg[Human::FEMALE][vari] : nullptr;
				break;
			case Human::CITIZEN_MALE:
				pPkg = (vari < IDENTITIES_NUM) ? mTransient.mVariPkg[Human::MALE][vari] : nullptr;
				break;
			case Human::TRAVELLER:
				pPkg = mResident.mPlr[Human::FEMALE];
				break;
			case Human::WANDERER:
				pPkg = mResident.mPlr[Human::MALE];
				break;
			default:
				break;
		}
		return pPkg;
	}

	void init_resident() {
		char buff[6];
		for (int i = 0; i < Human::Kind::MAX_KIND; ++i) {
			char htc = get_resource_code(Human::Kind(i));
			XD_SPRINTF(XD_SPRINTF_BUF(buff, 5), "npc_%c", htc);
			mResident.mBasePkg[i] = Scene::load_pkg(buff);
		}
		mResident.mPlr[Human::FEMALE] = Scene::load_pkg("traveller");
#if 0
		mResident.mPlr[Human::MALE] = Scene::load_pkg("wanderer");
#else
		mResident.mPlr[Human::MALE] = nullptr;
#endif
	}

	void init_transient() {
		char buff[10];
		for (int i = 0; i < Human::Kind::MAX_KIND; ++i) {
			char htc = get_resource_code(Human::Kind(i));
			for (int j = 0; j < IDENTITIES_NUM; ++j) {
				XD_SPRINTF(XD_SPRINTF_BUF(buff, 10), "npc_%c%02u", htc, j);
				mTransient.mVariPkg[i][j] = Scene::load_pkg(buff);
			}
		}
	}

	void init() {
		mFixedFreq = nxApp::get_bool_opt("fixfreq", false);
		mLowQ = nxApp::get_bool_opt("lowq", false);
		mCount = 0;
		init_resident();
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

Human* find(const char* pName) {
	ScnObj* pObj = Scene::find_obj(pName);
	return pObj ? HumanSys::as_human(pObj) : nullptr;
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

Human* as_human(ScnObj* pObj) {
	return obj_is_human(pObj) ? pObj->get_ptr_wk<Human>(0) : nullptr;
}

static void human_before_mot_func(ScnObj* pObj) {
	Human* pHuman = as_human(pObj);
	if (!pHuman) return;
	pHuman->select_motion();
}

static void human_exec_func(ScnObj* pObj) {
	Human* pHuman = as_human(pObj);
	if (!pHuman) return;
	pHuman->ctrl();
	float fps = pObj->mpMoveMot ? pObj->mpMoveMot->mFPS : 0.0f;
	float frameAdd = (fps / 60.0f) * TimeCtrl::get_motion_speed();
	if (frameAdd > 0.0f) {
		pObj->move(frameAdd);
	}
}

static void human_del_func(ScnObj* pObj) {
	Human* pHuman = as_human(pObj);
	if (!pHuman) return;
	nxCore::tMem<Human>::free(pHuman);
	pObj->set_ptr_wk<void>(0, nullptr);
	pObj->set_ptr_wk<void>(1, nullptr);
}

static void human_before_blend_func(ScnObj* pObj) {
	Human* pHuman = as_human(pObj);
	if (!pHuman) return;
	pHuman->exec_collision();
	pHuman->mRig.exec();
}

ScnObj* add_human(const Human::Descr& descr, Human::CtrlFunc ctrl) {
	ScnObj* pObj = nullptr;
	if (s_initFlg) {
		int bodyId = descr.personId % HumanWk::IDENTITIES_NUM;
		bodyId  = nxCalc::clamp(bodyId, 0, bodyId);
		Pkg* pPkg = s_wk.get_pkg(descr.type, bodyId);
		if (pPkg) {
			sxModelData* pMdl = pPkg->get_default_model();
			if (pMdl) {
				char nameBuf[64];
				const char* pName = nullptr;
				if (descr.pName) {
					pName = descr.pName;
				} else {
					sxValuesData* pVal = pPkg->find_values("params");
					if (pVal) {
						sxValuesData::Group personalGrp = pVal->find_grp("personal");
						if (personalGrp.is_valid()) {
							int nameIdx = personalGrp.find_val_idx("name");
							if (personalGrp.ck_val_idx(nameIdx)) {
								pName = personalGrp.get_val_s(nameIdx);
							}
						}
					}
				}
				if (!pName) {
					XD_SPRINTF(XD_SPRINTF_BUF(nameBuf, sizeof(nameBuf)), "%s@%03d", pPkg->get_name(), s_wk.mCount);
					pName = nameBuf;
				}
				nxCore::dbg_msg("Adding %s 	\n", pName);
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
				pHuman->mType = descr.type;

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
				pHuman->change_state(Human::INIT);
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