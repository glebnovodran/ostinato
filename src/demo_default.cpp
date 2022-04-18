/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

#include <crosscore.hpp>
#include <scene.hpp>
#include <demo.hpp>
#include <smprig.hpp>
#include <oglsys.hpp>

#include "input.hpp"
#include "timectrl.hpp"
#include "camera.hpp"
#include "human.hpp"
#include "ostinato.hpp"

DEMO_PROG_BEGIN

struct Stage {
	Pkg* pPkg;
	sxCollisionData* pCol;
	sxGeometryData* pNPCPosGeo;
	ScnObj* pPlayer;
	CamCtrl::CamCtx camCtx;
} s_stage = {};

static void init_view() {
	CamCtrl::init();
	s_stage.camCtx.mCamMode = 0;
	s_stage.camCtx.mpTgtObj = s_stage.pPlayer;
}

static void view_exec() {
	if (InputCtrl::triggered(InputCtrl::SWITCH2)) {
		s_stage.camCtx.mCamMode ^= 1;
	}
	CamCtrl::exec(&s_stage.camCtx);
}

static void add_stg_obj(sxModelData* pMdl, void* pWkData) {
	ScnObj* pObj = Scene::add_obj(pMdl);
	if (pObj) {
		pObj->set_base_color_scl(1.0f);
	}
}

static void citizen_roam_ctrl(Human* pHuman) {
	if (!pHuman) return;
	double objTouchDT = pHuman->get_obj_touch_duration_secs();
	double wallTouchDT = pHuman->get_wall_touch_duration_secs();

	switch (pHuman->mAction) {
		case Human::ACT_STAND:
			if (pHuman->mActionTimer.check_time_out()) {
				uint64_t rng = Scene::glb_rng_next() & 0x3f;
				if (rng < 0xF) {
					pHuman->change_act(Human::ACT_RUN, 2.0f);
				} else if (rng < 0x30) {
					pHuman->change_act(Human::ACT_WALK, 5.0f);
					pHuman->reset_wall_touch();
				} else {
					float t = nxCalc::fit(Scene::glb_rng_f01(), 0.0f, 1.0f, 0.5f, 2.0f);
					if (rng < 0x36) {
						pHuman->change_act(Human::ACT_TURN_L, t);
					} else {
						pHuman->change_act(Human::ACT_TURN_R, t);
					}
				}
			}
			break;
		case Human::ACT_WALK: 
			if ( objTouchDT > 0.2 || wallTouchDT > 0.25) {
				if (objTouchDT > 0 && ((Scene::glb_rng_next() & 0x3F) < 0x1F)) {
					pHuman->change_act(Human::ACT_RETREAT, 0.5f);
				} else {
					float t = nxCalc::fit(Scene::glb_rng_f01(), 0.0f, 1.0f, 0.5f, 2.0f);
					if (Scene::glb_rng_next() & 0x1) {
						pHuman->change_act(Human::ACT_TURN_L, t);
					} else {
						pHuman->change_act(Human::ACT_TURN_R, t);
					}
				}
			} else {
				ScnObj* pObj = pHuman->mpObj;

				if (pObj->mRoutine[0] == 0) {
					if (!pHuman->mActionTimer.check_time_out()) {
						bool rotFlg = int(Scene::glb_rng_next() & 0xFF) < 0x10;
						if (rotFlg) {
							pObj->mRoutine[0] = 1;
							float ryAdd = 0.5f;
							if (Scene::glb_rng_next() & 1) {
								ryAdd = -ryAdd;
							}
							pObj->mFltWk[0] = ryAdd;
							float t = nxCalc::fit(Scene::glb_rng_f01(), 0.0f, 1.0f, 0.5f, 1.0f);
							pHuman->mAuxTimer.start(t);
						}
					}
				} else {
					if (pHuman->mAuxTimer.check_time_out()) {
						pObj->mRoutine[0] = 0;
						pObj->mFltWk[0] = 0.0f;
					} else {
						pHuman->add_deg_y(pObj->mFltWk[0]);
					}
				}
			}
			break;
		case Human::ACT_RUN:
			if (pHuman->mActionTimer.check_time_out() || wallTouchDT > 0.1 || objTouchDT > 0.1) {
				bool standFlg = int(Scene::glb_rng_next() & 0xF) < 0x4;
				if (standFlg) {
					float t = nxCalc::fit(Scene::glb_rng_f01(), 0.0f, 1.0f, 1.0f, 2.0f);
					pHuman->change_act(Human::ACT_STAND, t);
				} else {
					float t = nxCalc::fit(Scene::glb_rng_f01(), 0.0f, 1.0f, 1.5f, 2.5f);
					pHuman->change_act(Human::ACT_WALK, t);
				}
			}
			break;
		case Human::ACT_RETREAT:
			if (pHuman->mActionTimer.check_time_out() || wallTouchDT > 0.1) {
				float t = nxCalc::fit(Scene::glb_rng_f01(), 0.0f, 1.0f, 1.5f, 2.5f);
				pHuman->change_act(Human::ACT_STAND, t);
			}
			break;
		case Human::ACT_TURN_L:
		case Human::ACT_TURN_R:
			if (pHuman->mActionTimer.check_time_out()) {
				float t = nxCalc::fit(Scene::glb_rng_f01(), 0.0f, 1.0f, 0.5f, 1.5f);
				pHuman->change_act(Human::ACT_STAND, t);
			}
			break;
	}
}

static void Manana_exec_ctrl(Human* pHuman) {
	if (!pHuman) return;
	switch (pHuman->mAction) {
	case Human::ACT_STAND:
		if (InputCtrl::triggered(InputCtrl::UP)) {
			pHuman->change_act(Human::ACT_WALK, 2.0f, 20);
		} else if (InputCtrl::triggered(InputCtrl::DOWN)) {
			pHuman->change_act(Human::ACT_RETREAT, 0.5f, 20);
		} else if (InputCtrl::now_active(InputCtrl::LEFT)) {
			pHuman->change_act(Human::ACT_TURN_L, 0.5f, 30);
		} else if (InputCtrl::now_active(InputCtrl::RIGHT)) {
			pHuman->change_act(Human::ACT_TURN_R, 0.5f, 30);
		}
		break;
	case Human::ACT_WALK:
		if (InputCtrl::now_active(InputCtrl::UP)) {
			if (InputCtrl::now_active(InputCtrl::LEFT)) {
				pHuman->add_deg_y(0.5f);
			} else if (InputCtrl::now_active(InputCtrl::RIGHT)) {
				pHuman->add_deg_y(-0.5f);
			}
		} else if (InputCtrl::triggered(InputCtrl::DOWN)) {
			pHuman->change_act(Human::ACT_RETREAT, 0.5f, 20);
		} else {
			pHuman->change_act(Human::ACT_STAND, 0.5f, 20);
		}
		break;
	case Human::ACT_RETREAT:
		if (InputCtrl::now_active(InputCtrl::DOWN)) {
			if (InputCtrl::now_active(InputCtrl::LEFT)) {
				pHuman->add_deg_y(0.5f);
			} else if (InputCtrl::now_active(InputCtrl::RIGHT)) {
				pHuman->add_deg_y(-0.5f);
			}
		} else {
			pHuman->change_act(Human::ACT_STAND, 0.5f, 20);
		}
		break;
	case Human::ACT_TURN_L:
		if (InputCtrl::now_active(InputCtrl::LEFT)) {
			if (InputCtrl::triggered(InputCtrl::UP)) {
				pHuman->change_act(Human::ACT_WALK, 0.5f, 20);
			}
		} else {
			pHuman->change_act(Human::ACT_STAND, 0.5f, 20);
		}
		break;
	case Human::ACT_TURN_R:
		if (InputCtrl::now_active(InputCtrl::RIGHT)) {
			if (InputCtrl::triggered(InputCtrl::UP)) {
				pHuman->change_act(Human::ACT_WALK, 0.5f, 20);
			}
		} else {
			pHuman->change_act(Human::ACT_STAND, 0.5f, 20);
		}
		break;
	default:
		break;
	}
}

static void init_resources() {
	Pkg* pPkg = Scene::load_pkg("street");

	s_stage.pPkg = pPkg;
	s_stage.pCol = nullptr;
	if (pPkg) {
		Scene::for_all_pkg_models(pPkg, add_stg_obj, &s_stage);
		s_stage.pCol = pPkg->find_collision("col");
		HumanSys::set_collision(s_stage.pCol);
		sxGeometryData* pGeo = pPkg->find_geometry("npc_pos");
		s_stage.pNPCPosGeo = pGeo;
		if (pGeo) {
			int npts = s_stage.pNPCPosGeo->get_pnt_num();
			for (int i = 0; i < npts; ++i) {
				Human::Descr descr;
				descr.reset();
				cxVec pos = pGeo->get_pnt(i);
				descr.type = Human::Type(nxCore::rng_next()%2);
				descr.bodyVariation = i;
				float heightMod = 0.0f;
				if (descr.type == Human::FEMALE) {
					heightMod = nxCalc::fit(nxCore::rng_f01(), 0.0f, 1.0f, -0.05f, 0.06f);
				} else {
					heightMod = nxCalc::fit(nxCore::rng_f01(), 0.0f, 1.0f, 0.0f, 0.06f);
				}
				descr.scale = 1.0f + heightMod;
				ScnObj* pObj = HumanSys::add_human(descr, citizen_roam_ctrl);
				pObj->set_world_quat_pos(nxQuat::from_degrees(0.0f, 0.0f, 0.0f), pos);
			}
		}
	}
	nxCore::dbg_msg("\n~ Welcome to Ostinato ~\n");
}

void init_player() {
	Human::Descr descr;
	descr.reset();
	descr.type = Human::Type::FEMALE;
	descr.bodyVariation = 5;
	descr.scale = 1.0f;
	descr.pName = "Manana";
	ScnObj* pPlr = HumanSys::add_human(descr, Manana_exec_ctrl);
	pPlr->set_world_quat_pos(nxQuat::from_degrees(0.0f, 0.0f, 0.0f), cxVec(34.5f, 0.0f, -19.0f));
	s_stage.pPlayer = pPlr;
}

static void init() {
	TimeCtrl::init(TimeCtrl::VARIABLE);
	HumanSys::init();
	init_resources();
	init_player();
	init_view();
	Scene::glb_rng_reset();
	InputCtrl::init();
}

static void loop(void* pLoopCtx) {
	TimeCtrl::exec();
	InputCtrl::update();
	Ostinato::set_default_lightning();
	Scene::exec();
	view_exec();
	Scene::visibility();
	Scene::frame_begin(cxColor(0.5f));
	Scene::draw();
	Scene::frame_end();
}

static void reset() {
	HumanSys::reset();
	TimeCtrl::reset();
}

DEMO_REGISTER(default);

DEMO_PROG_END
