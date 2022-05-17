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
	if (InputCtrl::triggered(InputCtrl::SWITCH1)) {
		s_demoWk.camCtx.mPosMode ^= 1;
	}
	if (InputCtrl::triggered(InputCtrl::SWITCH2)) {
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

static void init_resources() {
	Pkg* pPkg = Scene::load_pkg("street");
	s_demoWk.pPkg = pPkg;
	s_demoWk.pCol = nullptr;
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
}

static void init_player() {
	ScnObj* pPlr = Player::init();
	pPlr->set_world_quat_pos(nxQuat::from_degrees(0.0f, 0.0f, 0.0f), cxVec(34.5f, 0.0f, -19.0f));
	s_demoWk.pPlayer = pPlr;
	Ostinato::set_cam_tgt("Traveller");
}
void init_params() {
	s_demoWk.disableStgShadows = nxApp::get_bool_opt("nostgshadow", false);
	s_demoWk.showPerf = nxApp::get_bool_opt("showperf", false);
}

static void init() {
	init_params();
	s_demoWk.perfCPU.init();
	s_demoWk.perfGPU.init();

	TimeCtrl::init();
	double start = nxSys::time_micros();
	HumanSys::init();
	init_resources();
	double finish = nxSys::time_micros();
	nxCore::dbg_msg("\nResources loaded in %f millis.\n", (finish - start)/1000.0);
	init_player();
	init_view();
	Scene::glb_rng_reset();
	InputCtrl::init();
	nxCore::dbg_msg("\n~ Welcome to Ostinato ~\n");
}

static void draw_2d() {
	using namespace Performance;

	char str[1024];
	float refSizeX = 800;
	float refSizeY = 600;
	Scene::set_ref_scr_size(refSizeX, refSizeY);
	if (Scene::get_view_mode_width() < Scene::get_view_mode_height()) {
		Scene::set_ref_scr_size(refSizeY, refSizeX);
	}

	if (s_demoWk.showPerf) {
		char fpsStr[16];

		float fps = TimeCtrl::get_fps();
		double exe = s_demoWk.perfCPU.get_median(Measure::EXE);
		double vis = s_demoWk.perfCPU.get_median(Measure::VISIBILITY);
		double drw = s_demoWk.perfCPU.get_median(Measure::DRAW);
		double gpu = s_demoWk.perfGPU.mMillis;

		if (fps < 0.0f) {
			XD_SPRINTF(XD_SPRINTF_BUF(fpsStr, sizeof(fpsStr)), " --");
		} else {
			XD_SPRINTF(XD_SPRINTF_BUF(fpsStr, sizeof(fpsStr)), "%.2f", fps);
		}
		XD_SPRINTF(XD_SPRINTF_BUF(str, sizeof(str)), "FPS: %s, EXE: %.2f, VIS: %.2f, DRW: %.2f, GPU: %.2f, SUM: %.2f", fpsStr, exe, vis, drw, gpu, exe+vis+drw+gpu);
		size_t slen = nxCore::str_len(str);

		float sx = 10.0f;
		float sy = Scene::get_ref_scr_height() - 20.0f;

		xt_float2 bpos[4];
		xt_float2 btex[4];
		btex[0].set(0.0f, 0.0f);
		btex[1].set(1.0f, 0.0f);
		btex[2].set(1.0f, 1.0f);
		btex[3].set(0.0f, 1.0f);
		float bx = 4.0f;
		float by = 4.0f;
		float bw = slen * 8;
		float bh = 12.0f;
		cxColor bclr(0.0f, 0.0f, 0.0f, 0.75f);
		bpos[0].set(sx - bx, sy - by);
		bpos[1].set(sx + bw + bx, sy - by);
		bpos[2].set(sx + bw + bx, sy + bh + by);
		bpos[3].set(sx - bx, sy + bh + by);

		Scene::quad(bpos, btex, bclr);
		Scene::print(sx, sy, cxColor(0.1f, 0.75f, 0.1f, 1.0f), str);
	}
}

static void loop(void* pLoopCtx) {
	using namespace Performance;

	s_demoWk.perfGPU.exec();

	s_demoWk.perfCPU.begin(Measure::EXE);
	TimeCtrl::exec();
	InputCtrl::update();
	Ostinato::update_sensors();
	Ostinato::set_default_lighting();
	Scene::exec();
	view_exec();
	s_demoWk.perfCPU.end(Measure::EXE);

	s_demoWk.perfCPU.begin(Measure::VISIBILITY);
	Scene::visibility();
	s_demoWk.perfCPU.end(Measure::VISIBILITY);

	s_demoWk.perfCPU.begin(Measure::DRAW);
	s_demoWk.perfGPU.begin();
	Scene::frame_begin(cxColor(0.5f));
	Scene::draw();
	draw_2d();
	s_demoWk.perfGPU.end();
	s_demoWk.perfCPU.end(Measure::DRAW);

	Scene::frame_end();
}

static void reset() {
	HumanSys::reset();
	TimeCtrl::reset();
	s_demoWk.perfCPU.free();
	s_demoWk.perfGPU.free();
}

DEMO_REGISTER(default);

DEMO_PROG_END
