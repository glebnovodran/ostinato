/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

#include <crosscore.hpp>
#include <scene.hpp>
#include <smprig.hpp>

#include "timectrl.hpp"
#include "human.hpp"

namespace PrimFX {

struct Primitives {
	static const int NUM_VTX = 1000;
	static const int NUM_IDX = 1000;
	sxTextureData* mpShdTex;
	uint32_t mNextVtx = 0;

	void init() {
		Scene::init_prims(NUM_VTX, NUM_IDX);
		Pkg* pCmnPkg = Scene::find_pkg(SCN_CMN_PKG_NAME);
		if (pCmnPkg) {
			mpShdTex = pCmnPkg->find_texture("shd_common_BASE");
		}
	}

	void reset() {
		mpShdTex = nullptr;
	}
} s_primWk = {};

void init() {
	s_primWk.init();
}

void reset() {
	s_primWk.reset();
}

void begin() {
	s_primWk.mNextVtx = 0;
}

bool draw_pseudo_shadow(ScnObj* pObj, void* pMem) {
	if (HumanSys::obj_is_human(pObj)) {
		int iroot = pObj->find_skel_node_id("root");

		if (iroot < 0) {
			return false;
		}

		cxMtx wm = pObj->calc_skel_world_mtx(iroot);
		sxPrimVtx vtx[4];
		cxVec nrm(0.0f, 1.0f, 0.0f);
		for (int i = 0; i < 4; ++i) {
			vtx[i].prm.fill(0.0f);
			vtx[i].encode_normal(nrm);
			vtx[i].clr.set(0.05f, 0.05f, 0.05f, 0.75f);
			vtx[i].tex.fill(0.5f);
		}

		vtx[0].pos.set(-0.5f, 0.0f, 0.5f, 1.0f);
		vtx[1].pos.set(0.5f, 0.0f, 0.5f, 1.0f);
		vtx[2].pos.set(0.5f, 0.0f, -0.5f, 1.0f);
		vtx[3].pos.set(-0.5f, 0.0f, -0.5f, 1.0f);

		vtx[0].tex.set(0.0f, 0.0f, 0.0f, 0.0f);
		vtx[1].tex.set(1.0f, 0.0f, 0.0f, 0.0f);
		vtx[2].tex.set(1.0f, 1.0f, 0.0f, 0.0f);
		vtx[3].tex.set(0.0f, 1.0f, 0.0f, 0.0f);

		uint16_t idx[] = { 0, 1, 2, 0, 2, 3 };

		wm.set_translation(wm.get_translation() + cxVec(0.0f, 0.025f, 0.0f));
		Scene::prim_geom(s_primWk.mNextVtx, 4, vtx, 0, 6, idx);
		s_primWk.mNextVtx =  s_primWk.mNextVtx <= (Primitives::NUM_VTX - 4) ? s_primWk.mNextVtx + 4 : 0;
		Scene::idx_tris_semi_dsided(0, 2, &wm, s_primWk.mpShdTex, false);
	}
	return true;
}

}