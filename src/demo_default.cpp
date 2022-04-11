/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

#include <crosscore.hpp>
#include <scene.hpp>
#include <demo.hpp>
#include <smprig.hpp>
#include "oglsys.hpp"

#include "timectrl.hpp"
#include "humansys.hpp"

DEMO_PROG_BEGIN

#if 0
#define KEY_GET(_name) if (OGLSys::get_key_state(#_name)) mask |= 1ULL << _name
#else
#define KEY_GET(_name) process_key(&mask, _name, #_name)
#endif

static struct KBD_CTRL {
	enum {
		UP = 0,
		DOWN,
		LEFT,
		RIGHT,
		LCTRL,
		LSHIFT,
		TAB,
		BACK,
		ENTER,
		SPACE,
		NUM_KEYS
	};
	uint64_t mNow;
	uint64_t mOld;
	bool mSkip;

	void init() {
		mNow = 0;
		mOld = 0;
		mSkip = false;
	}

	const char* get_alt_key_name(const int code) {
		static struct {
			int code;
			const char* pName;
		} tbl[] = {
			{ UP, "W" }, { DOWN, "S" }, { LEFT, "A" }, { RIGHT, "D" }, {SPACE, " "}
		};
		const char* pAltName = nullptr;
		for (int i = 0; i < XD_ARY_LEN(tbl); ++i) {
			if (code == tbl[i].code) {
				pAltName = tbl[i].pName;
				break;
			}
		}
		return pAltName;
	}

	void process_key(uint64_t* pMask, const int code, const char* pName) {
		if (!pMask || !pName) return;
		if (OGLSys::get_key_state(pName)) *pMask |= 1ULL << code;
		const char* pAltName = get_alt_key_name(code);
		if (pAltName) {
			if (OGLSys::get_key_state(pAltName)) *pMask |= 1ULL << code;
		}
	}

	void update() {
		if (mSkip) return;
		mOld = mNow;
		uint64_t mask = 0;
		KEY_GET(UP);
		KEY_GET(DOWN);
		KEY_GET(LEFT);
		KEY_GET(RIGHT);
		KEY_GET(LCTRL);
		KEY_GET(LSHIFT);
		KEY_GET(TAB);
		KEY_GET(BACK);
		KEY_GET(ENTER);
		KEY_GET(SPACE);
		mNow = mask;
	}

	bool ck_now(int id) const { return !!(mNow & (1ULL << id)); }
	bool ck_old(int id) const { return !!(mOld & (1ULL << id)); }
	bool ck_trg(int id) const { return !!((mNow & (mNow ^ mOld)) & (1ULL << id)); }
	bool ck_chg(int id) const { return !!((mNow ^ mOld) & (1ULL << id)); }
} s_kbdCtrl;

struct VIEW_WK {
	cxVec pos;
	cxVec tgt;
	int viewMode;
} s_view;

struct STAGE {
	Pkg* pPkg;
	sxCollisionData* pCol;
	sxGeometryData* pNPCPosGeo;
} s_stage = {};

//struct DEMO_WK {};
static void init_view() {
	::memset(&s_view, 0, sizeof(VIEW_WK));
}

static void view_exec() {
	ScnObj* pObj = Scene::find_obj("Manana");
	if (pObj) {
		if (s_kbdCtrl.ck_trg(KBD_CTRL::BACK)) {
			s_view.viewMode ^= 1;
		}
		cxVec wpos = pObj->get_world_pos();
		cxAABB bbox = pObj->get_world_bbox();
		float cy = bbox.get_center().y;
		float yoffs = cy - bbox.get_min_pos().y;
		s_view.tgt = wpos + cxVec(0.0f, yoffs + 0.6f, 0.0f);
		cxVec offs = cxVec(1.0f, 1.0f, 6.0f);
		if (s_view.viewMode) {
			offs = cxVec(-4.0f, 4.0f, 12.0f);
		}
		s_view.pos = s_view.tgt + offs;
	}
	Scene::set_view(s_view.pos, s_view.tgt);
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
	//nxCore::dbg_msg("%d", pHuman->mAction);
	switch (pHuman->mAction) {
	case Human::ACT_STAND:
		//if (pHuman->mActionTimer.check_time_out()) {	
		if (s_kbdCtrl.ck_trg(KBD_CTRL::UP)) {
			pHuman->change_act(Human::ACT_WALK, 2.0f, 20);
		} else if (s_kbdCtrl.ck_trg(KBD_CTRL::DOWN)) {
			pHuman->change_act(Human::ACT_RETREAT, 0.5f, 20);
		} else if (s_kbdCtrl.ck_now(KBD_CTRL::LEFT)) {
			pHuman->change_act(Human::ACT_TURN_L, 0.5f, 30);
		} else if (s_kbdCtrl.ck_now(KBD_CTRL::RIGHT)) {
			pHuman->change_act(Human::ACT_TURN_R, 0.5f, 30);
		}
		break;
	case Human::ACT_WALK:
		if (s_kbdCtrl.ck_now(KBD_CTRL::UP)) {
			if (s_kbdCtrl.ck_now(KBD_CTRL::LEFT)) {
				pHuman->add_deg_y(0.5f);
			} else if (s_kbdCtrl.ck_now(KBD_CTRL::RIGHT)) {
				pHuman->add_deg_y(-0.5f);
			}
		} else if (s_kbdCtrl.ck_trg(KBD_CTRL::DOWN)) {
			pHuman->change_act(Human::ACT_RETREAT, 0.5f, 20);
		} else {
			pHuman->change_act(Human::ACT_STAND, 0.5f, 20);
		}
		break;
	case Human::ACT_RETREAT:
		if (s_kbdCtrl.ck_now(KBD_CTRL::DOWN)) {
			if (s_kbdCtrl.ck_now(KBD_CTRL::LEFT)) {
				pHuman->add_deg_y(0.5f);
			} else if (s_kbdCtrl.ck_now(KBD_CTRL::RIGHT)) {
				pHuman->add_deg_y(-0.5f);
			}
		} else {
			pHuman->change_act(Human::ACT_STAND, 0.5f, 20);
		}
		break;
	case Human::ACT_TURN_L:
		if (s_kbdCtrl.ck_now(KBD_CTRL::LEFT)) {
			if (s_kbdCtrl.ck_trg(KBD_CTRL::UP)) {
				pHuman->change_act(Human::ACT_WALK, 0.5f, 20);
			}
		} else {
			pHuman->change_act(Human::ACT_STAND, 0.5f, 20);
		}
		break;
	case Human::ACT_TURN_R:
		if (s_kbdCtrl.ck_now(KBD_CTRL::RIGHT)) {
			if (s_kbdCtrl.ck_trg(KBD_CTRL::UP)) {
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
				descr.scale = 1.25f;
				ScnObj* pObj = HumanSys::add_human(descr, citizen_roam_ctrl);
				pObj->set_world_quat_pos(nxQuat::from_degrees(0.0f, 0.0f, 0.0f), pos);
			}
		}
	}

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
}

static void init() {
	TimeCtrl::init(TimeCtrl::VARIABLE); // TODO:get from params
	HumanSys::init();
	init_resources();
	init_player();
	init_view();
	Scene::glb_rng_reset();
	s_kbdCtrl.init();
}

static void set_scene_ctx() {
	Scene::set_shadow_density(1.1f);
	Scene::set_shadow_fade(35.0f, 40.0f);
	Scene::set_shadow_proj_params(40.0f, 40.0f, 100.0f);
	Scene::set_shadow_uniform(true);

	Scene::set_spec_dir_to_shadow();
	Scene::set_spec_shadowing(0.9f);

	Scene::set_hemi_upper(2.5f, 2.46f, 2.62f);
	Scene::set_hemi_lower(0.32f, 0.28f, 0.26f);
	Scene::set_hemi_up(Scene::get_shadow_dir().neg_val());
	Scene::set_hemi_exp(2.5f);
	Scene::set_hemi_gain(0.7f);

	Scene::set_fog_rgb(0.748f, 0.74f, 0.65f);
	Scene::set_fog_density(1.0f);
	Scene::set_fog_range(20.0f, 2000.0f);
	Scene::set_fog_curve(0.21f, 0.3f);

	Scene::set_exposure_rgb(0.75f, 0.85f, 0.5f);

	Scene::set_linear_white_rgb(0.65f, 0.65f, 0.55f);
	Scene::set_linear_gain(1.32f);
	Scene::set_linear_bias(-0.025f);
}


static void loop(void* pLoopCtx) {
	TimeCtrl::exec();
	s_kbdCtrl.update();
	set_scene_ctx();
	Scene::exec();
	view_exec();
	Scene::visibility();
	Scene::frame_begin(cxColor(0.5f));
	Scene::draw();
	Scene::frame_end();
}

static void reset() {
	HumanSys::reset();
}

DEMO_REGISTER(default);

DEMO_PROG_END
