/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

#include <crosscore.hpp>
#include <oglsys.hpp>
#include <scene.hpp>
#include <draw.hpp>
#include <demo.hpp>

#include "ostinato.hpp"

static const bool c_defBump = true;
static const bool c_defSpec = true;
static const bool c_defReduce = false;

extern "C" {

// web-interface functions

void wi_set_key_state(const char* pName, const int state) {
	if (OGLSys::get_frame_count() > 0) {
		OGLSys::set_key_state(pName, !!state);
	}
}

} // extern "C"

static void* oglsys_mem_alloc(size_t size, const char* pTag) {
	return nxCore::mem_alloc(size, pTag);
}

static void oglsys_mem_free(void* pMem) {
	nxCore::mem_free(pMem);
}

static void init_ogl(const int x, const int y, const int w, const int h, const int msaa) {
	OGLSysCfg cfg;
	::memset(&cfg, 0, sizeof(cfg));
	cfg.x = x;
	cfg.y = y;
	cfg.width = w;
	cfg.height = h;
	cfg.msaa = msaa;
	cfg.reduceRes = nxApp::get_bool_opt("reduce", c_defReduce);
	cfg.ifc.dbg_msg = nxCore::dbg_msg;
	cfg.ifc.mem_alloc = oglsys_mem_alloc;
	cfg.ifc.mem_free = oglsys_mem_free;
	Draw::Ifc* pDrawIfc = Draw::get_ifc_impl();
	if (pDrawIfc) {
		cfg.withoutCtx = !pDrawIfc->info.needOGLContext;
	}
	OGLSys::init(cfg);
	OGLSys::CL::init();
	OGLSys::set_swap_interval(nxApp::get_int_opt("swap", 1));
}

static void reset_ogl() {
	OGLSys::CL::reset();
	OGLSys::reset();
}

static void init_scn(const char* pAppPath) {
	const char* pAltDataDir = nxApp::get_opt("data");
	ScnCfg scnCfg;
	scnCfg.set_defaults();
	if (pAltDataDir) {
		scnCfg.pDataDir = pAltDataDir;
	}
	scnCfg.pAppPath = pAppPath;
	scnCfg.shadowMapSize = nxApp::get_int_opt("smap", 2048);
	scnCfg.numWorkers = nxApp::get_int_opt("nwrk", 0);
	scnCfg.useBump = nxApp::get_bool_opt("bump", c_defBump);
	scnCfg.useSpec = nxApp::get_bool_opt("spec", c_defSpec);
	::printf("#workers: %d\n", scnCfg.numWorkers);
	::printf("shadow size: %d\n", scnCfg.shadowMapSize);
	Scene::init(scnCfg);

	if (nxApp::get_int_opt("spersp", 0) != 0) {
		Scene::set_shadow_uniform(false);
	}
}

static void exec_demo() {
	Demo::Ifc* pIfc = Demo::get_demo();
	if (pIfc) {
		nxCore::dbg_msg("executing demo program: %s\n", pIfc->info.pName);
		pIfc->init();
		Scene::mem_info();
		OGLSys::loop(pIfc->loop);
		pIfc->reset();
	}
}

int main(int argc, char* argv[]) {
	nxApp::init_params(argc, argv);

	Ostinato::init();

	float scrScl = 1.0f;
	int x = 10;
	int y = 10;
	int w = 1200;
	int h = 700;

	w = int(float(w) * scrScl);
	h = int(float(h) * scrScl);

	w = nxCalc::max(32, nxApp::get_int_opt("w", w));
	h = nxCalc::max(32, nxApp::get_int_opt("h", h));

	int msaa = nxApp::get_int_opt("msaa", 0);

	init_ogl(x, y, w, h, msaa);
	init_scn(argv[0]);

	exec_demo();

	Scene::reset();
	reset_ogl();

	nxApp::reset();
	nxCore::mem_dbg();

	Ostinato::reset();

	return 0;

}