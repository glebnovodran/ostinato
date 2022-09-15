/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

#include <crosscore.hpp>
#include <oglsys.hpp>
#include <scene.hpp>
#include <draw.hpp>
#include <smprig.hpp>

#include "timectrl.hpp"
#include "human.hpp"
#include "ostinato.hpp"

#ifdef OGLSYS_MACOS
#	include "mac_ifc.h"
#endif

#undef OSTINATO_SENSORS
#undef OSTINATO_PIPES_AVAILABLE

#if defined(XD_SYS_LINUX) || defined(XD_SYS_BSD) || defined(OGLSYS_MACOS)
#	include <termios.h>
#	include <fcntl.h>
#	include <unistd.h>
#	if defined(XD_SYS_BSD)
#		include <sys/select.h>
#	endif
#	define OSTINATO_SENSORS
#	define OSTINATO_PIPES_AVAILABLE
#else 
#	undef OSTINATO_USE_PIPES
#endif

static const bool c_defBump = true;
static const bool c_defSpec = true;
static const bool c_defReduce = false;

static bool print_obj_name(ScnObj* pObj, void* pMem) {
	nxCore::dbg_msg("%s\n",pObj->mpName);
	return true;
}

class CmdInterpreter : public cxXqcLexer::TokenFunc {
private:
	enum State {
		START = 0,
		CAMTGT,
		MARK,
		UNMARK,
		HIDEOBJ,
		HIDEMTL,
		LSMTL,
		NUM_STATE
	};

	cxXqcLexer::Token mParamToks[8];
	cxStrStore* mpStrStore;

	State mState;
	uint32_t mParamCursor;

	const char* get_str_param(const cxXqcLexer::Token& tok) {
		const char* pParam = nullptr;
		if (tok.is_symbol() || tok.is_string()) {
			pParam = reinterpret_cast<const char*>(tok.val.p);
		}

		return pParam;
	}
	bool get_int_param(const cxXqcLexer::Token& tok, int* pVal) {
		bool res = false;
		if (tok.id == cxXqcLexer::TokId::TOK_INT) {
			*pVal = tok.val.i;
			res = true;
		}
		return res;
	}
public:
	CmdInterpreter() : mState(State::START), mParamCursor(0) {}

	void init() {
		mState = State::START;
		mParamCursor = 0;
		mpStrStore = cxStrStore::create("PipeCmdStore");
	}
	void reset() {
		if (mpStrStore) {
			cxStrStore::destroy(mpStrStore);
			mpStrStore = nullptr;
		}
	}

	virtual bool operator()(const cxXqcLexer::Token& tok) {
		bool contFlg = true;
		const char* pName = nullptr;

		if (mpStrStore == nullptr) { return false; }

		switch (mState) {
			case State::START:
				mParamCursor = 0;
				if (tok.is_symbol()) {
					const char* pSymName = reinterpret_cast<const char*>(tok.val.p);
					if (nxCore::str_eq(pSymName, "camtgt")) {
						mState = State::CAMTGT;
					} else if (nxCore::str_eq(pSymName, "mark")) {
						mState = State::MARK;
					} else if (nxCore::str_eq(pSymName, "unmark")) {
						mState = State::UNMARK;
					} else if (nxCore::str_eq(pSymName, "lsobj")) {
						Scene::for_each_obj(print_obj_name, nullptr);
					} else if (nxCore::str_eq(pSymName, "hideobj")) {
						mState = State::HIDEOBJ;
					} else if (nxCore::str_eq(pSymName, "meminfo")) {
						Scene::mem_info();
					} else if (nxCore::str_eq(pSymName, "hidemtl")) {
						mState = State::HIDEMTL;
					} else if (nxCore::str_eq(pSymName, "lsmtl")) {
						mState = State::LSMTL;
					} else {
						nxCore::dbg_msg("Unrecoginized command\n");
						contFlg = false;
					}
				} else {
					contFlg = false;
				}

				break;
			case State::CAMTGT:
				pName = get_str_param(tok);
				if (pName) {
					Ostinato::set_cam_tgt(pName);
					mState = State::START;
				} else {
					contFlg = false;
				}
				break;
			case State::MARK:
				pName = get_str_param(tok);
				if (pName) {
					HumanSys::mark(pName);
					mState = State::START;
				} else {
					contFlg = false;
				}
				break;
			case State::UNMARK: 
				pName = get_str_param(tok);
				if (pName) {
					HumanSys::unmark(pName);
					mState = State::START;
				} else {
					contFlg = false;
				}
				break;
			case State::HIDEOBJ:
				if (mParamCursor <= 1) {
					mParamToks[mParamCursor] = tok;
					pName = get_str_param(tok);
					if (pName) {
						mParamToks[mParamCursor].val.p = mpStrStore->add(pName);
					}
					mParamCursor++;
					if (mParamCursor > 1) {
						const char* pObjName = get_str_param(mParamToks[0]);
						if (pObjName) {
							ScnObj* pObj = Scene::find_obj(pObjName);
							if (pObj) {
								const char* pVal = get_str_param(mParamToks[1]);
								if (pVal) {
									if (nxCore::str_eq(pVal, "on")) {
										pObj->mDisableDraw = true;
									} else if (nxCore::str_eq(pVal, "off")) {
										pObj->mDisableDraw = false;
									}
								} else {
									nxCore::dbg_msg("Error parsing command.\n");
								}
							} else {
								nxCore::dbg_msg("Object not found\n");
							}
						}
						mState = State::START;
					}
				}
				break;
			case State::HIDEMTL:
				if (mParamCursor <= 2) {
					mParamToks[mParamCursor] = tok;
					pName = get_str_param(tok);
					if (pName) {
						mParamToks[mParamCursor].val.p = mpStrStore->add(pName);
					}
					mParamCursor++;
					if (mParamCursor > 2) {
						const char* pObjName = get_str_param(mParamToks[0]);
						if (pObjName) {
							ScnObj* pObj = Scene::find_obj(pObjName);
							const char* pMtlName = get_str_param(mParamToks[1]);
							if (pMtlName == nullptr) {
								int imtl = 0;
								if (get_int_param(mParamToks[1], &imtl)) {
									sxModelData* pMdlData = pObj->get_model_data();
									pMtlName = pMdlData ? pMdlData->get_material_name(imtl) : nullptr;
								}
							}
							if (pMtlName) {
								bool hide = false;
								const char* pVal = get_str_param(mParamToks[2]);
								if (pVal) {
									if (nxCore::str_eq(pVal, "on")) {
										hide = true;
									} else if (nxCore::str_eq(pVal, "off")) {
										hide = false;
									}
									pObj->hide_mtl(pMtlName, hide);
								} else {
									nxCore::dbg_msg("Error parsing command: the value should be on/off\n");
									contFlg = false;
								}
							} else {
								nxCore::dbg_msg("Error parsing command: specify material name.\n");
								contFlg = false;
							}
						} else {
							nxCore::dbg_msg("Object not found.\n");
							contFlg = false;
						}
					}
				}
				break;
			case State::LSMTL: 
				pName = get_str_param(tok);
				if (pName) {
					ScnObj* pObj = Scene::find_obj(pName);
					if (pObj) {
						sxModelData* pMdlData = pObj->get_model_data();
						if (pMdlData) {
							for (uint32_t i = 0; i < pMdlData->mMtlNum; ++i) {
								nxCore::dbg_msg("%d : %s\n", i, pMdlData->get_material_name(i));
							}
						}
					}
					mState = State::START;
				} else {
					contFlg = false;
				}
				break;
			default:
				contFlg = false;
		}
		return contFlg;

	}
};

static struct OstinatoGlobals {
	CmdInterpreter cmdInterp;
	cxXqcLexer lexer;
	ScnObj* pTgtObj;
	float lampsBrightness;
	float ccBrightness;

	struct Sensors {
		int fid;
		struct Values {
			int32_t light;
		} vals;
	} sensors;

	struct CmdPipe {
		int32_t fid;
	} cmdPipe;

	bool mEchoPipe;

	int32_t dummySleep;
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

static const char* oglsys_get_opt(const char* pName) {
	return nxApp::get_opt(pName);
}

#define BUNDLE_SIG XD_FOURCC('B', 'N', 'D', 'L')
#define BUNDLE_FNAME "ostinato.bnd"

enum class BndSource {
	NONE,
	LOCAL,
	MAX = LOCAL
};

enum class BndFileSearch {
	LINEAR,
	STRMAP,
	MAX = STRMAP
};

struct BndFileInfo {
	uint32_t offs;
	uint32_t size;
};

static struct BundleWk {
	FILE* pFile;
	BndFileInfo* pInfos;
	cxStrMap<xt_fhandle>* pFileMap;
	char* pPaths;
	int32_t nfiles;
	BndSource bndSrc;
	bool searchStrMap;
	bool valid() const { return pFile != nullptr; }
} s_bnd = {};

static FILE* bnd_sys_fopen(const char* pPath, const char* pMode) {
#if defined(_MSC_VER)
	FILE* pFile = nullptr;
	::fopen_s(&pFile, pPath, pMode);
	return pFile;
#else
	return ::fopen(pPath, pMode);
#endif
}

static FILE* bnd_sys_bin_open(const char* pPath) {
	return bnd_sys_fopen(pPath, "rb");
}

static bool bnd_check_handle(xt_fhandle fh) {
	BundleWk* pBnd = &s_bnd;
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
	BundleWk* pBnd = &s_bnd;
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
		char prefixBuf[256];
		nxCore::mem_zero(prefixBuf, sizeof(prefixBuf));
		if (!pPathPrefix) {
			size_t pathLen = nxCore::str_len(pPath);
			if (pathLen < sizeof(prefixBuf)) {
				const char* pStdPrefix = "../data/";
				size_t stdPrefixLen = nxCore::str_len(pStdPrefix);
				if (pathLen > stdPrefixLen) {
					for (size_t i = pathLen - stdPrefixLen; --i >= stdPrefixLen;) {
						if (nxCore::mem_eq(&pPath[i], pStdPrefix, stdPrefixLen)) {
							nxCore::mem_copy(prefixBuf, pPath, i + stdPrefixLen);
							pPathPrefix = prefixBuf;
							break;
						}
					}
				}
			}
		}
		if (pPathPrefix && nxCore::str_starts_with(pPath, pPathPrefix)) {
			const char* pReqPath = pPath + nxCore::str_len(pPathPrefix);
			if (s_bnd.searchStrMap) {
				s_bnd.pFileMap->get(pReqPath, &fh);
			} else {
				char* pBndPath = pBnd->pPaths;
				int32_t idx = -1;
				for (int32_t i = 0; i < pBnd->nfiles; ++i) {
					if (nxCore::str_eq(pReqPath, pBndPath)) {
						idx = i;
						break;
					}
					size_t bndPathLen = ::nxCore::str_len(pBndPath);
					pBndPath += bndPathLen + 1;
				}
				if (idx >= 0) {
					fh = (xt_fhandle)(&pBnd->pInfos[idx]);
				}
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
			BundleWk* pBnd = &s_bnd;
			::fseek(pBnd->pFile, pInfo->offs, SEEK_SET);
			nread = ::fread(pDst, 1, nbytes, pBnd->pFile);
		}
	}
	return nread;
}

static bool init_bundle(const char* pPath, BundleWk* pBnd) {
	bool res = false;

	if (pPath != nullptr) {

		FILE* pFile = bnd_sys_bin_open(pPath);

		if (pFile) {
			uint32_t sig = 0;
			size_t nread = ::fread(&sig, 1, sizeof(sig), pFile);
			if (nread == sizeof(sig) && sig == BUNDLE_SIG) {
				int32_t nfiles = 0;
				nread = ::fread(&nfiles, 1, sizeof(nfiles), pFile);
				if (nread == sizeof(nfiles) && nfiles > 0) {
					pBnd->nfiles = nfiles;
					if (s_bnd.searchStrMap) {
						pBnd->pFileMap = cxStrMap<xt_fhandle>::create("bundleMap");
					}
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
									if (s_bnd.searchStrMap) {
										char* pBndPath = pBnd->pPaths;
										for (int32_t i = 0; i < pBnd->nfiles; ++i) {
											xt_fhandle fh = (xt_fhandle)(&pBnd->pInfos[i]);
											pBnd->pFileMap->add(pBndPath, fh);
											size_t bndPathLen = nxCore::str_len(pBndPath);
											pBndPath += bndPathLen + 1;
										}
									}
									res = true;
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
	return res;
}

static const char* init_bundle() {
	static const char* pPathTbl[] = {
		BUNDLE_FNAME,
		"../" BUNDLE_FNAME,
		"../data/" BUNDLE_FNAME,
		"bin/data/" BUNDLE_FNAME
	};
	BundleWk* pBnd = &s_bnd;
	const char* pLoadedPath = nullptr;

	pBnd->pFile = nullptr;
	pBnd->nfiles = 0;
	pBnd->pPaths = nullptr;

	s_bnd.searchStrMap = nxApp::get_bool_opt("bndtbl", false);
	const char* pBndPath = nxApp::get_opt("bndpath");

	if (init_bundle(pBndPath, pBnd)) {
		pLoadedPath = pBndPath;
	} else {
		for (size_t i = 0; i < XD_ARY_LEN(pPathTbl); ++i) {
			const char* pPath = pPathTbl[i];
			if (init_bundle(pPath, pBnd)) {
				pLoadedPath = pPath;
				break;
			}
		}
	}

	return pLoadedPath;
}

static void reset_bundle() {
	BundleWk* pBnd = &s_bnd;
	if (pBnd->valid()) {
		nxCore::mem_free(pBnd->pPaths);
		nxCore::mem_free(pBnd->pInfos);
		::fclose(pBnd->pFile);
		pBnd->pFile = nullptr;
		pBnd->pPaths = nullptr;
		pBnd->pInfos = nullptr;
		pBnd->nfiles = 0;
	}
	if (s_bnd.pFileMap) {
		cxStrMap<xt_fhandle>::destroy(s_bnd.pFileMap);
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
			struct termios opts;
			::tcgetattr(s_globals.sensors.fid, &opts);
			::cfsetispeed(&opts, B115200);
			::cfsetospeed(&opts, B115200);
			opts.c_cflag |= (CLOCAL | CREAD);
			::tcsetattr(s_globals.sensors.fid, TCSANOW, &opts);
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

static void init_cmd_pipe() {
	s_globals.cmdPipe.fid = -1;
#if defined(OSTINATO_PIPES_AVAILABLE) && defined(OSTINATO_USE_PIPES)
	s_globals.cmdPipe.fid = ::open("ostinato_cmd", O_RDONLY | O_NONBLOCK);
	nxCore::dbg_msg("******* pipe %d *******\n", s_globals.cmdPipe.fid);
	s_globals.mEchoPipe = nxApp::get_bool_opt("echopipe", false);
#endif
}

static void reset_cmd_pipe() {
#if defined(OSTINATO_PIPES_AVAILABLE) && defined(OSTINATO_USE_PIPES)
	int32_t fid = s_globals.cmdPipe.fid;
	if (fid >= 0) {
		::close(fid);
	}
#endif
	s_globals.cmdPipe.fid = -1;
}

static void dummygl_swap_func() {
	if (s_globals.dummySleep > 0) {
		nxSys::sleep_millis(s_globals.dummySleep);
	} else if (s_globals.dummySleep < 0) {
		double len = double(-s_globals.dummySleep) * 1.0e3;
		double start = nxSys::time_micros();
		while ((nxSys::time_micros() - start) < len) {}
	}
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
	cfg.ifc.get_opt = oglsys_get_opt;

	Draw::Ifc* pDrawIfc = Draw::get_ifc_impl();
	if (pDrawIfc) {
		cfg.withoutCtx = !pDrawIfc->info.needOGLContext;
	}

	OGLSys::init(cfg);
	OGLSys::CL::init();
	OGLSys::set_swap_interval(nxApp::get_int_opt("swap", 1));

	s_globals.dummySleep = nxApp::get_int_opt("dummygl_swap_sleep", 1);
	OGLSys::set_dummygl_swap_func(dummygl_swap_func);
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

	float heapSizeMb = nxApp::get_float_opt("local_heap_size", 2.0f);

	if (heapSizeMb > 0.0f) {
		heapSizeMb = nxCalc::max(1.0f, heapSizeMb);
		 Scene::alloc_local_heaps(size_t(1024 * 1024 * heapSizeMb));
	}
}


namespace Ostinato {

void init(int argc, char* argv[]) {
#ifndef OGLSYS_MACOS
	nxApp::init_params(argc, argv);
#endif

	sxSysIfc sysIfc;
	::memset(&sysIfc, 0, sizeof(sysIfc));
	sysIfc.fn_dbgmsg = dbgmsg;

	s_globals.ccBrightness = nxApp::get_float_opt("cc_brightness", 1.0f);

	int bndsrc = nxCalc::clamp(nxApp::get_int_opt("bnd", int(BndSource::LOCAL)), 0, int(BndSource::MAX));
	BndSource bndSrc = BndSource(bndsrc);
	const char* pLoadedPath = nullptr;
	if (bndSrc != BndSource::NONE) {
		pLoadedPath = init_bundle();

		if (s_bnd.valid()) {
			sysIfc.fn_fopen = bnd_fopen;
			sysIfc.fn_fclose = bnd_fclose;
			sysIfc.fn_fread = bnd_fread;
			sysIfc.fn_fsize = bnd_fsize;

		}
	}

	nxSys::init(&sysIfc);

	if (pLoadedPath) {
		nxCore::dbg_msg("Loaded bundle [%s]\n", pLoadedPath);
	} else {
		nxCore::dbg_msg("No bundle loaded\n");
	}


	int x = 10;
	int y = 10;
#ifdef OGLSYS_MACOS
	int w = mac_get_width_opt();
	int h = mac_get_height_opt();
#else
	float scrScl = 1.0f;
	int w = get_def_width();
	int h = get_def_height();
	w = int(float(w) * scrScl);
	h = int(float(h) * scrScl);

	w = nxCalc::max(32, nxApp::get_int_opt("w", w));
	h = nxCalc::max(32, nxApp::get_int_opt("h", h));
#endif

	int msaa = nxApp::get_int_opt("msaa", 0);

	init_ogl(x, y, w, h, msaa);

	s_globals.lexer.disable_keywords();

	init_sensors();
	init_cmd_pipe();
	init_scn_sys(argv[0]);
}

void reset() {
	Scene::reset();
	reset_ogl();
	reset_sensors();
	reset_cmd_pipe();
	nxApp::reset();
	reset_bundle();
	nxCore::dbg_msg("Peak MB: %.4f\n", double(nxCore::mem_peak_bytes()) / (1024.0 * 1024.0));
	nxCore::mem_dbg();
}

void mac_start(int argc, const char* argv[]) {
#ifdef OGLSYS_MACOS
	nxApp::init_params(argc, (char**)argv);
#endif
}

int get_def_width() {
	return 1200;
}

int get_def_height() {
	return 700;
}

void set_default_lighting() {
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

	float fogRangeMin = 20.0f;
	float fogRangeMax = 2000.0f;
	Scene::set_fog_rgb(0.748f, 0.74f, 0.65f);
	Scene::set_fog_density(1.0f);
	Scene::set_fog_range(fogRangeMin, 2000.0f);
	Scene::set_fog_curve(0.21f, 0.3f);

	Scene::set_exposure_rgb(0.75f, 0.85f, 0.5f);

	Scene::set_linear_white_rgb(0.65f, 0.65f, 0.55f);
	Scene::set_linear_gain(1.32f * s_globals.ccBrightness);
	Scene::set_linear_bias(-0.025f * s_globals.ccBrightness);

 	s_globals.lampsBrightness = 0.0f;

	int32_t lval = s_globals.sensors.vals.light;
	if (lval >= 0 && lval <= 1023) {
		float val = float(lval) / 1023.0f;
		const float sunLim = 0.75f;
		if (val <= sunLim) {
			float scl = nxCalc::fit(val, 0.0f, sunLim, 0.0f, 1.0f);

			s_globals.lampsBrightness = nxCalc::fit(scl, 0.0f, 1.0f, 1.0f, 0.05f);

			Scene::set_shadow_density(nxCalc::fit(scl, 0.0f, 1.0, 0.25f, 1.0f));
			float fogScl = nxCalc::fit(scl, 0.0f, 1.0, 0.1f, 1.0f);
			Scene::set_fog_range(fogRangeMin * fogScl, fogRangeMax * nxCalc::saturate(fogScl + 0.7f));
			float uprScl = scl;
			float redScl = scl;
			float greenScl = scl;
			float blueScl = scl;
			float biasScl = nxCalc::fit(uprScl, 0.0f, 1.0f, 0.023f, 0.0f);
			uprScl = nxCalc::fit(uprScl, 0.0f, 1.0f, 0.55f, 0.98f);
			redScl = nxCalc::fit(redScl, 0.0f, 1.0f, 0.3f, 0.8f);
			greenScl = nxCalc::fit(greenScl, 0.0f, 1.0f, 0.25f, 0.6f);
			blueScl = nxCalc::fit(blueScl, 0.0f, 1.0f, 2.0f, 1.0f);
			Scene::set_hemi_upper(2.5f * uprScl * redScl, 2.46f * uprScl * greenScl, 2.62f * uprScl * blueScl);
			Scene::set_linear_bias(-0.025f + biasScl);
		}
		float expVal = nxCalc::fit(val, 0.0f, 1.0f, -0.4f, 3.2f);
		Scene::set_linear_gain(0.5f + nxCalc::cb_root(val)*0.25f);
		val = nxCalc::fit(val, 0.0f, 1.0f, 0.75f, 0.1f);
		Scene::set_linear_white_rgb(val, val, val * 0.75f);
		Scene::set_exposure_rgb(0.75f + expVal, 0.85f + expVal, 0.5f + expVal);
		float fogScl = val;
		Scene::set_fog_curve(nxCalc::lerp(0.21f, 0.79f, fogScl), nxCalc::lerp(0.3f, 0.96f, fogScl));
	}
}

float get_lamps_brightness() {
	return s_globals.lampsBrightness;
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
		nxCore::mem_zero(valBuf, sizeof(valBuf));
		ssize_t nread = ::read(fid, inBuf, sizeof(inBuf) - 1);
		if (nread > 0) {
			ssize_t valIdx = 0;
			for (ssize_t i = 0; i < nread; ++i) {
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

void update_cmd_pipe() {
#if defined(OSTINATO_PIPES_AVAILABLE) && defined(OSTINATO_USE_PIPES)
	int32_t fid = s_globals.cmdPipe.fid;
	if (fid <= 0) {
		return;
	}

	char buf[128];
	nxCore::mem_zero(buf, sizeof(buf));
	ssize_t n = ::read(fid, buf, sizeof(buf) - 1);
	if (n > 0) {
		if (s_globals.mEchoPipe) {
			nxCore::dbg_msg("-> %s\n", buf);
		}
		s_globals.lexer.set_text(buf, n);
		s_globals.cmdInterp.init();
		s_globals.lexer.scan(s_globals.cmdInterp);
		s_globals.cmdInterp.reset();
		s_globals.lexer.reset();
	}
#endif
}

extern "C" {

// web-interface functions

void jsi_set_key_state(const char* pName, const int state) {
	if (OGLSys::get_frame_count() > 0) {
		OGLSys::set_key_state(pName, !!state);
	}
}

int jsi_get_test_val() { return 42; }

} // extern "C"

} // Ostinato
