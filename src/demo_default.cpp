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
} s_stage = {};

static void init_view() {
	Camera::init();
	s_stage.camCtx.mCamMode = 0;
	s_stage.camCtx.mpTgtObj = s_stage.pPlayer;
}

static void view_exec() {
	if (InputCtrl::triggered(InputCtrl::SWITCH2)) {
		s_stage.camCtx.mCamMode ^= 1;
	}
	Camera::exec(s_stage.camCtx);
}

static void add_stg_obj(sxModelData* pMdl, void* pWkData) {
	ScnObj* pObj = Scene::add_obj(pMdl);
	if (pObj) {
		pObj->set_base_color_scl(1.0f);
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
		s_stage.camCtx.mpZones = pPkg->find_geometry("cam_zones");
		sxGeometryData* pGeo = pPkg->find_geometry("npc_pos");
		s_stage.pNPCPosGeo = pGeo;
		if (pGeo) {
			int npts = s_stage.pNPCPosGeo->get_pnt_num();
			nxCore::dbg_msg("Populating city quarter with %d citizens...\n", npts);
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
			}
		}
	}
	nxCore::dbg_msg("\n~ Welcome to Ostinato ~\n");
}

void init_player() {
	ScnObj* pPlr = Player::init();
	pPlr->set_world_quat_pos(nxQuat::from_degrees(0.0f, 0.0f, 0.0f), cxVec(34.5f, 0.0f, -19.0f));
	s_stage.pPlayer = pPlr;
}

static void init() {
	TimeCtrl::init();
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
