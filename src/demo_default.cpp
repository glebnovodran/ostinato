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

DEMO_PROG_BEGIN

struct Stage {
	Pkg* pPkg;
	sxCollisionData* pCol;
	sxGeometryData* pNPCPosGeo;
	ScnObj* pPlayer;
	Camera::Context camCtx;
	bool disableStgShadows;
} s_stage = {};

struct DemoWk {
	bool showFPS;
} s_demoWk;

static void init_view() {
	Camera::init();
	s_stage.camCtx.mTgtMode = 0;
	s_stage.camCtx.mpTgtObj = Ostinato::get_cam_tgt_obj();
}

static void view_exec() {
	if (InputCtrl::triggered(InputCtrl::SWITCH1)) {
		s_stage.camCtx.mPosMode ^= 1;
	}
	if (InputCtrl::triggered(InputCtrl::SWITCH2)) {
		s_stage.camCtx.mTgtMode ^= 1;
	}
	s_stage.camCtx.mpTgtObj = Ostinato::get_cam_tgt_obj();
	Camera::exec(s_stage.camCtx);
}

static void add_stg_obj(sxModelData* pMdl, void* pWkData) {
	ScnObj* pObj = Scene::add_obj(pMdl);
	if (pObj) {
		pObj->set_base_color_scl(1.0f);
		pObj->mDisableShadowCast = s_stage.disableStgShadows;
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
		Camera::set_collision(s_stage.pCol);
		s_stage.camCtx.mpZones = pPkg->find_geometry("cam_zones");
		sxGeometryData* pGeo = pPkg->find_geometry("npc_pos");
		s_stage.pNPCPosGeo = pGeo;
		if (pGeo) {
			int npts = s_stage.pNPCPosGeo->get_pnt_num();
			nxCore::dbg_msg("Populating city quarter with %d citizens...\n\n", npts);
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

				nxCore::dbg_msg("Citizen added: %s \n", pObj->mpName != nullptr ? pObj->mpName: "Anonimous citizen");
				const char* pOccp = HumanSys::get_occupation(pObj->mpName);
				nxCore::dbg_msg("[Occupation]:%s\n\n", pOccp != nullptr ? pOccp : "Lounging");
			}
		}
	}

	nxCore::dbg_msg("\n~ Welcome to Ostinato ~\n");
}

static void init_player() {
	ScnObj* pPlr = Player::init();
	pPlr->set_world_quat_pos(nxQuat::from_degrees(0.0f, 0.0f, 0.0f), cxVec(34.5f, 0.0f, -19.0f));
	s_stage.pPlayer = pPlr;
	Ostinato::set_cam_tgt("Traveller");
}
void init_params() {
	s_stage.disableStgShadows = nxApp::get_bool_opt("nostgshadow", false);
	s_demoWk.showFPS = nxApp::get_bool_opt("showfps", false);
}

static void init() {
	init_params();
	TimeCtrl::init();
	HumanSys::init();
	init_resources();
	init_player();
	init_view();
	Scene::glb_rng_reset();
	InputCtrl::init();
}

static void draw_2d() {
	char str[32];
	float refSizeX = 800;
	float refSizeY = 600;
	Scene::set_ref_scr_size(refSizeX, refSizeY);
	if (Scene::get_view_mode_width() < Scene::get_view_mode_height()) {
		Scene::set_ref_scr_size(refSizeY, refSizeX);
	}

	if (s_demoWk.showFPS) {
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
		float bw = 80.0f + 10.0f;
		float bh = 12.0f;
		cxColor bclr(0.0f, 0.0f, 0.0f, 0.75f);
		bpos[0].set(sx - bx, sy - by);
		bpos[1].set(sx + bw + bx, sy - by);
		bpos[2].set(sx + bw + bx, sy + bh + by);
		bpos[3].set(sx - bx, sy + bh + by);
		Scene::quad(bpos, btex, bclr);

		float fps = TimeCtrl::get_fps();
		if (fps < 0.0f) {
			XD_SPRINTF(XD_SPRINTF_BUF(str, sizeof(str)), "FPS: --");
		} else {
			XD_SPRINTF(XD_SPRINTF_BUF(str, sizeof(str)), "FPS: %.2f", fps);
		}
		Scene::print(sx, sy, cxColor(0.1f, 0.75f, 0.1f, 1.0f), str);
	}
}

static void loop(void* pLoopCtx) {
	TimeCtrl::exec();
	InputCtrl::update();
	Ostinato::update_sensors();
	Ostinato::set_default_lightning();
	Scene::exec();
	view_exec();
	Scene::visibility();
	Scene::frame_begin(cxColor(0.5f));
	Scene::draw();
	draw_2d();
	Scene::frame_end();
}

static void reset() {
	HumanSys::reset();
	TimeCtrl::reset();
}

DEMO_REGISTER(default);

DEMO_PROG_END
