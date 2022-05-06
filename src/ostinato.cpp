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


#define BUNDLE_SIG XD_FOURCC('B', 'N', 'D', 'L')
#define BUNDLE_FNAME "ostinato.bnd"

struct BndFileInfo {
	uint32_t offs;
	uint32_t size;
};

static struct BUNDLE_WK {
	FILE* pFile;
	BndFileInfo* pInfos;
	char* pPaths;
	int32_t nfiles;

	bool valid() const { return pFile != nullptr; }
} s_bnd = {};

static FILE* bnd_sys_fopen(const char* pPath, const char* pMode) {
#if defined(_MSC_VER)
	FILE* pFile = nullptr;
	::fopen_s(&pFile, pPath, pMode);
	return f;
#else
	return ::fopen(pPath, pMode);
#endif
}

static FILE* bnd_sys_bin_open(const char* pPath) {
	return bnd_sys_fopen(pPath, "rb");
}

static bool bnd_check_handle(xt_fhandle fh) {
	BUNDLE_WK* pBnd = &s_bnd;
	bool res = false;
	if (pBnd->valid() && fh) {
		uint8_t* pInfoTop = (uint8_t*)pBnd->pInfos;
		uint8_t* pInfoLast = (uint8_t*)(pBnd->pInfos + (pBnd->nfiles-1));
		uint8_t* pCheck = (uint8_t*)fh;
		res = pCheck >= pInfoTop && pCheck <= pInfoLast;
	}
	return res;
}

static xt_fhandle bnd_fopen(const char* pPath) {
	xt_fhandle fh = nullptr;
	BUNDLE_WK* pBnd = &s_bnd;
	if (pPath && pBnd->valid()) {
		const char* pPathPrefix = nullptr;
		static const char* pPrefixLst[] = {
			"./../data/", "./bin/data/"
		};
		for (size_t i = 0; i < XD_ARY_LEN(pPrefixLst); ++i) {
			if (nxCore::str_starts_with(pPath, pPrefixLst[i])) {
				pPathPrefix = pPrefixLst[i];
				break;
			}
		}
		if (pPathPrefix && nxCore::str_starts_with(pPath, pPathPrefix)) {
			const char* pReqPath = pPath + ::strlen(pPathPrefix);
			char* pBndPath = pBnd->pPaths;
			int32_t idx = -1;
			for (int32_t i = 0; i < pBnd->nfiles; ++i) {
				if (nxCore::str_eq(pReqPath, pBndPath)) {
					idx = i;
					break;
				}
				size_t bndPathLen = ::strlen(pBndPath);
				pBndPath += bndPathLen + 1;
			}
			if (idx >= 0) {
				fh = (xt_fhandle)(&pBnd->pInfos[idx]);
			}
		}
	}
	return fh;
}

static void bnd_fclose(xt_fhandle fh) {
}

static size_t bnd_fsize(xt_fhandle fh) {
	size_t size = 0;
	if (bnd_check_handle(fh)) {
		BndFileInfo* pInfo = (BndFileInfo*)fh;
		size = pInfo->size;
	}
	return size;
}

static size_t bnd_fread(xt_fhandle fh, void* pDst, size_t nbytes) {
	size_t nread = 0;
	if (pDst && bnd_check_handle(fh)) {
		BndFileInfo* pInfo = (BndFileInfo*)fh;
		if (nbytes == pInfo->size) {
			BUNDLE_WK* pBnd = &s_bnd;
			::fseek(pBnd->pFile, pInfo->offs, SEEK_SET);
			nread = ::fread(pDst, 1, nbytes, pBnd->pFile);
		}
	}
	return nread;
}

static void init_bundle() {
	static const char* pPathTbl[] = {
		BUNDLE_FNAME,
		"../" BUNDLE_FNAME,
		"../data/" BUNDLE_FNAME,
		"bin/data/" BUNDLE_FNAME
	};
	BUNDLE_WK* pBnd = &s_bnd;
	pBnd->pFile = nullptr;
	pBnd->nfiles = 0;
	pBnd->pPaths = nullptr;
	for (size_t i = 0; i < XD_ARY_LEN(pPathTbl); ++i) {
		const char* pPath = pPathTbl[i];
		FILE* pFile = bnd_sys_bin_open(pPath);
		if (pFile) {
			uint32_t sig = 0;
			size_t nread = ::fread(&sig, 1, sizeof(sig), pFile);
			if (nread == sizeof(sig) && sig == BUNDLE_SIG) {
				int32_t nfiles = 0;
				nread = ::fread(&nfiles, 1, sizeof(nfiles), pFile);
				if (nread == sizeof(nfiles) && nfiles > 0) {
					pBnd->nfiles = nfiles;
					size_t finfoSize = nfiles * sizeof(BndFileInfo);
					pBnd->pInfos = (BndFileInfo*)nxCore::mem_alloc(finfoSize, "Bnd.infos");
					if (pBnd->pInfos) {
						nread = ::fread(pBnd->pInfos, 1, finfoSize, pFile);
						if (nread == finfoSize) {
							size_t strsSize = pBnd->pInfos[0].offs - (8 + finfoSize);
							char* pPathsData = (char*)nxCore::mem_alloc(finfoSize, "Bnd.paths0");
							if (pPathsData) {
								nread = ::fread(pPathsData, 1, strsSize, pFile);
								if (nread == strsSize) {
									uint8_t* pUnpackedPaths = nxData::unpack((sxPackedData*)pPathsData, "Bnd.paths");
									if (pUnpackedPaths) {
										nxCore::mem_free(pPathsData);
										pBnd->pPaths = (char*)pUnpackedPaths;
									} else {
										pBnd->pPaths = pPathsData;
									}
									pBnd->pFile = pFile;
								} else {
									nxCore::mem_free(pPathsData);
									nxCore::mem_free(pBnd->pInfos);
									pBnd->pInfos = nullptr;
									pBnd->nfiles = 0;
									::fclose(pFile);
								}
							} else {
								nxCore::mem_free(pBnd->pInfos);
								pBnd->pInfos = nullptr;
								pBnd->nfiles = 0;
								::fclose(pFile);
							}
						} else {
							nxCore::mem_free(pBnd->pInfos);
							pBnd->pInfos = nullptr;
							pBnd->nfiles = 0;
							::fclose(pFile);
						}
					}
				} else {
					::fclose(pFile);
				}
			} else {
				::fclose(pFile);
			}
		}
	}
}

static void reset_bundle() {
	BUNDLE_WK* pBnd = &s_bnd;
	if (pBnd->valid()) {
		nxCore::mem_free(pBnd->pPaths);
		nxCore::mem_free(pBnd->pInfos);
		::fclose(pBnd->pFile);
		pBnd->pFile = nullptr;
		pBnd->pPaths = nullptr;
		pBnd->pInfos = nullptr;
		pBnd->nfiles = 0;
	}
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

	init_bundle();

	sxSysIfc sysIfc;
	::memset(&sysIfc, 0, sizeof(sysIfc));
	sysIfc.fn_dbgmsg = dbgmsg;
	if (s_bnd.valid()) {
		sysIfc.fn_fopen = bnd_fopen;
		sysIfc.fn_fclose = bnd_fclose;
		sysIfc.fn_fread = bnd_fread;
		sysIfc.fn_fsize = bnd_fsize;

	}
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
	reset_bundle();
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