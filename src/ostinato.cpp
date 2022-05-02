/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */
#include <crosscore.hpp>
#include <oglsys.hpp>
#include <scene.hpp>
#include <draw.hpp>

#include "ostinato.hpp"

#undef OSTINATO_SENSORS
#if defined(XD_SYS_LINUX) || defined(XD_SYS_BSD)
#	include <fcntl.h>
#	include <unistd.h>
#	if defined(XD_SYS_BSD)
#		include <sys/select.h>
#	endif
#	define OSTINATO_SENSORS
#endif

static const bool c_defBump = true;
static const bool c_defSpec = true;
static const bool c_defReduce = false;

static struct OstinatoGlobals {
	ScnObj* pTgtObj;
	struct Sensors {
		int fid;
		struct Values {
			int32_t light;
		} vals;
	} sensors;
} s_globals = {};

static void dbgmsg(const char* pMsg) {
	::printf("%s", pMsg);
	::fflush(stdout);
}

static void* oglsys_mem_alloc(size_t size, const char* pTag) {
	return nxCore::mem_alloc(size, pTag);
}

static void oglsys_mem_free(void* pMem) {
	nxCore::mem_free(pMem);
}

static void init_sensors() {
	s_globals.sensors.fid = -1;
	s_globals.sensors.vals.light = -1;
#ifdef OSTINATO_SENSORS
	const char* pSensPath = nxApp::get_opt("sensors");
	if (pSensPath) {
		s_globals.sensors.fid = ::open(pSensPath, O_SYNC);
		if (s_globals.sensors.fid >= 0) {
			nxCore::dbg_msg("Ostinato: sensors port open @ %s\n", pSensPath);
		}
	}
#endif
}

static void reset_sensors() {
#ifdef OSTINATO_SENSORS
	int fid = s_globals.sensors.fid;
	if (fid >= 0) {
		::close(fid);
	}
#endif
	s_globals.sensors.fid = -1;
}

static void init_ogl(const int x, const int y, const int w, const int h, const int msaa) {
	OGLSysCfg cfg;
	cfg.clear();
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

static void init_scn_sys(const char* pAppPath) {
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
	nxCore::dbg_msg("#workers: %d\n", scnCfg.numWorkers);
	nxCore::dbg_msg("shadow size: %d\n", scnCfg.shadowMapSize);
	Scene::init(scnCfg);

	if (nxApp::get_int_opt("spersp", 0) != 0) {
		Scene::set_shadow_uniform(false);
	}
}


namespace Ostinato {

void init(int argc, char* argv[]) {
	nxApp::init_params(argc, argv);

	sxSysIfc sysIfc;
	::memset(&sysIfc, 0, sizeof(sysIfc));
	sysIfc.fn_dbgmsg = dbgmsg;
	nxSys::init(&sysIfc);

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
	init_sensors();
	init_scn_sys(argv[0]);
}

void reset() {
	Scene::reset();
	reset_ogl();
	reset_sensors();
	nxApp::reset();
	nxCore::mem_dbg();
}

void set_default_lightning() {

#if 0
	// Uniform mode
	Scene::set_shadow_density(1.0f);
	Scene::set_shadow_fade(35.0f, 40.0f);
	Scene::set_shadow_proj_params(35.0f, 30.0f, 100.0f);
	Scene::set_shadow_uniform(true);
#else
	// Perspective mode
	Scene::set_shadow_density(1.0f);
	Scene::set_shadow_fade(55.0f, 70.0f);
	Scene::set_shadow_proj_params(55.0f, 30.0f, 100.0f);
	Scene::set_shadow_uniform(false);
#endif
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

	int32_t lval = s_globals.sensors.vals.light;
	if (lval >= 0 && lval <= 1023) {
		float val = float(lval) / 1023.0f;
		const float sunLim = 0.75f;
		if (val <= sunLim) {
			float uprScl = nxCalc::fit(val, 0.0f, sunLim, 0.0f, 1.0f);
			float redScl = uprScl;
			float biasScl = nxCalc::fit(uprScl, 0.0f, 1.0f, 0.023f, 0.0f);
			uprScl = nxCalc::fit(uprScl, 0.0f, 1.0f, 0.55f, 0.98f);
			redScl = nxCalc::fit(redScl, 0.0f, 1.0f, 0.5f, 1.0f);
			Scene::set_hemi_upper(2.5f * uprScl * redScl, 2.46f * uprScl, 2.62f * uprScl);
			Scene::set_linear_bias(-0.025f + biasScl);
		}
		float expVal = nxCalc::fit(val, 0.0f, 1.0f, -0.4f, 3.2f);
		Scene::set_linear_gain(0.5f + nxCalc::cb_root(val)*0.25f);
		val = nxCalc::fit(val, 0.0f, 1.0f, 0.75f, 0.1f);
		Scene::set_linear_white_rgb(val, val, val * 0.75f);
		Scene::set_exposure_rgb(0.75f + expVal, 0.85f + expVal, 0.5f + expVal);
	}
}

ScnObj* get_cam_tgt_obj() {
	return s_globals.pTgtObj;
}

void set_cam_tgt(const char* pName) {
	s_globals.pTgtObj = Scene::find_obj(pName);
}

void update_sensors() {
#ifdef OSTINATO_SENSORS
	int fid = s_globals.sensors.fid;
	if (fid >= 0) {
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(fid, &fds);
		int sel = ::select(fid + 1, &fds, nullptr, nullptr, nullptr);
		if (sel <= 0) {
			fid = -1;
		}
	}
	if (fid >= 0) {
		char inBuf[128];
		char valBuf[128];
		size_t nread = ::read(fid, inBuf, sizeof(inBuf) - 1);
		if (nread > 0) {
			size_t valIdx = 0;
			for (size_t i = 0; i < nread; ++i) {
				char c = inBuf[i];
				if (c >= '0' && c <= '9') {
					valBuf[valIdx++] = c;
				}
			}
			int32_t val = 0;
			if (valIdx > 0) {
				val = ::atoi(inBuf);
				//nxCore::dbg_msg("%d, ", val);
				s_globals.sensors.vals.light = val;
			}
		}
	}
#endif
}

}; // namespace