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
#include "citizen.hpp"
#include "player.hpp"
#include "ostinato.hpp"
#include "perfmon.hpp"
#include "primfx.hpp"
#include "draw2d.hpp"

DEMO_PROG_BEGIN


struct DemoWk {
	Performance::CPUMonitor perfCPU;
	Performance::GPUMonitor perfGPU;

	Pkg* pPkg;
	sxCollisionData* pCol;
	sxGeometryData* pNPCPosGeo;
	ScnObj* pPlayer;
	Camera::Context camCtx;
	bool disableStgShadows;
	bool showPerf;
	bool drawPseudoShd;
	int exeRep;
	TimeCtrl::Frequency tfreq;
} s_demoWk = {};

static const char* pLampsMtlName = "_lamps_";

static void stg_bat_pre_draw(ScnObj* pObj, const int ibat) {
	if (!pObj) return;
	const char* pMtlName = pObj->get_batch_mtl_name(ibat);
	if (nxCore::str_eq(pMtlName, pLampsMtlName)) {
		Scene::push_ctx();
		float scl = Ostinato::get_lamps_brightness();
		if (scl > 0.0f) {
			scl = nxCalc::ease_crv(0.03f, 0.1f, scl);
			scl = nxCalc::ease_crv(0.01f, 0.99f, scl) * 10.0f;
			Scene::set_hemi_const(1.0f, 0.805248f, 0.192973);
			Scene::scl_hemi_upper(scl);
			Scene::scl_hemi_lower(scl);
		}
	}
}

static void stg_bat_post_draw(ScnObj* pObj, const int ibat) {
	if (!pObj) return;
	const char* pMtlName = pObj->get_batch_mtl_name(ibat);
	if (nxCore::str_eq(pMtlName, pLampsMtlName)) {
		Scene::pop_ctx();
	}
}

static void init_view() {
	Camera::init();
	s_demoWk.camCtx.mTgtMode = 0;
	s_demoWk.camCtx.mpTgtObj = Ostinato::get_cam_tgt_obj();
}

static void view_exec() {
	if (InputCtrl::triggered(InputCtrl::BTN_SWITCH1)) {
		s_demoWk.camCtx.mPosMode ^= 1;
	}
	if (InputCtrl::triggered(InputCtrl::BTN_SWITCH2)) {
		s_demoWk.camCtx.mTgtMode ^= 1;
	}
	s_demoWk.camCtx.mpTgtObj = Ostinato::get_cam_tgt_obj();
	Camera::exec(s_demoWk.camCtx);
}

static void add_stg_obj(sxModelData* pMdl, void* pWkData) {
	ScnObj* pObj = Scene::add_obj(pMdl);
	if (pObj) {
		pObj->set_base_color_scl(1.0f);
		pObj->mDisableShadowCast = s_demoWk.disableStgShadows;
		pObj->mBatchPreDrawFunc = stg_bat_pre_draw;
		pObj->mBatchPostDrawFunc = stg_bat_post_draw;
	}
}

static bool init_resources() {
	Pkg* pPkg = Scene::load_pkg("street");
	s_demoWk.pPkg = pPkg;
	s_demoWk.pCol = nullptr;
	bool res = false;
	if (pPkg) {
		Scene::for_all_pkg_models(pPkg, add_stg_obj, &s_demoWk);
		s_demoWk.pCol = pPkg->find_collision("col");
		HumanSys::set_collision(s_demoWk.pCol);
		Camera::set_collision(s_demoWk.pCol);
		s_demoWk.camCtx.mpZones = pPkg->find_geometry("cam_zones");
		sxGeometryData* pGeo = pPkg->find_geometry("npc_pos");
		s_demoWk.pNPCPosGeo = pGeo;
		if (pGeo) {
			int npts = s_demoWk.pNPCPosGeo->get_pnt_num();
			nxCore::dbg_msg("\nPopulating city quarter with %d citizens...\n\n", npts);
			for (int i = 0; i < npts; ++i) {
				Human::Descr descr;
				descr.reset();
				cxVec pos = pGeo->get_pnt(i);
				descr.type = Human::Type(i % 2);
				descr.personId = i / 2;
				descr.pName = nullptr;
				float heightMod = 0.0f;
				if (descr.type == Human::CITIZEN_FEMALE) {
					heightMod = nxCalc::fit(nxCore::rng_f01(), 0.0f, 1.0f, -0.05f, 0.06f);
				} else {
					heightMod = nxCalc::fit(nxCore::rng_f01(), 0.0f, 1.0f, 0.0f, 0.06f);
				}
				descr.scale = 1.0f + heightMod;
				float startY = nxCore::rng_f01() * 360.0f;
				ScnObj* pObj = Citizen::add(descr, nxQuat::from_degrees(0.0f, startY, 0.0f), pos);

				const char* pName = pObj->mpName != nullptr ? pObj->mpName: "Anonimous";
				const char* pOccp = HumanSys::get_occupation(pObj->mpName);
				pOccp = pOccp != nullptr ? pOccp : "Lounging";
				nxCore::dbg_msg("  Enter %s The %s \n", pName, pOccp);
			}
		}
	}
	return res;
}

static void init_player() {
	ScnObj* pPlr = Player::init();
	if (pPlr) {
		pPlr->set_world_quat_pos(nxQuat::from_degrees(0.0f, 0.0f, 0.0f), cxVec(34.5f, 0.0f, -19.0f));
		s_demoWk.pPlayer = pPlr;
		Ostinato::set_cam_tgt("Traveller");
		ScnObj* pWanderer = Scene::find_obj("Wanderer");
		if (pWanderer) {
			pWanderer->set_world_quat_pos(nxQuat::from_degrees(0.0f, 48.0f, 0.0f), cxVec(47.35f, 0.0f, -61.676f));
		}
	}
}

void init_params() {
	s_demoWk.disableStgShadows = nxApp::get_bool_opt("nostgshadow", false);
	s_demoWk.showPerf = nxApp::get_bool_opt("showperf", false);
	int smapVal = nxApp::get_int_opt("smap", 2048);
	s_demoWk.drawPseudoShd = (smapVal == -1);
	s_demoWk.exeRep = nxCalc::max(nxApp::get_int_opt("exerep", 1), 1);
}

static void init() {
	init_params();

	s_demoWk.perfCPU.init();
	s_demoWk.perfGPU.init();

	PrimFX::init();
	Draw2D::init(&s_demoWk.perfCPU, &s_demoWk.perfGPU, s_demoWk.showPerf);

	TimeCtrl::init();
	double start = nxSys::time_micros();

	HumanSys::init();
	init_resources();

	double finish = nxSys::time_micros();
	nxCore::dbg_msg("\nResources loaded in %.2f millis.\n", (finish - start)/1000.0);

	init_player();
	init_view();
	Scene::glb_rng_reset();
	InputCtrl::init();

	nxCore::dbg_msg("\n~ Welcome to Ostinato ~\n");
}


static void draw_prims() {
	if (s_demoWk.drawPseudoShd) {
		Scene::for_each_obj(PrimFX::draw_pseudo_shadow, nullptr);
	}
}

void scene_exec() {
	for (int i = 0; i < s_demoWk.exeRep; ++i) {
		Scene::exec();
		view_exec();
	}
}

static void loop(void* pLoopCtx) {
	using namespace Performance;

	s_demoWk.perfGPU.exec();

	// ------------------------------------------

	s_demoWk.perfCPU.begin(Measure::EXE);

	TimeCtrl::exec();
	InputCtrl::update();
	Ostinato::update_sensors();
	Ostinato::set_default_lighting();

	scene_exec();

	s_demoWk.perfCPU.end(Measure::EXE);

	// ------------------------------------------

	s_demoWk.perfCPU.begin(Measure::VISIBILITY);
	Scene::visibility();
	s_demoWk.perfCPU.end(Measure::VISIBILITY);

	// ------------------------------------------

	s_demoWk.perfCPU.begin(Measure::DRAW);
	s_demoWk.perfGPU.begin();

	Scene::frame_begin(cxColor(0.5f));
	Scene::draw();
	draw_prims();
	Draw2D::exec();

	s_demoWk.perfGPU.end();
	s_demoWk.perfCPU.end(Measure::DRAW);

	// ------------------------------------------

	Scene::frame_end();
}

static void reset() {
	HumanSys::reset();
	TimeCtrl::reset();;
	PrimFX::reset();
	Draw2D::reset();

	s_demoWk.perfCPU.free();
	s_demoWk.perfGPU.free();
}

DEMO_REGISTER(default);

DEMO_PROG_END
