// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2022 Sergey Chaban <sergey.chaban@gmail.com>

#include "crosscore.hpp"
#include "oglsys.hpp"
#include "draw.hpp"

#ifndef VTXLAYOUT_TEX_HALF
#	define VTXLAYOUT_TEX_HALF 0
#endif

#ifndef VTXLAYOUT_CLR_HALF
#	define VTXLAYOUT_CLR_HALF 0
#endif

#ifndef VTXLAYOUT_WGT_HALF
#	define VTXLAYOUT_WGT_HALF 0
#endif

#define MAX_XFORMS 128
#define MAX_BAT_XFORMS 16

DRW_IMPL_BEGIN

static cxResourceManager* s_pRsrcMgr = nullptr;

struct GPUVtx {
	xt_float3   pos;
	//float _pos_pad;
	xt_float3   nrm;
	//float _nrm_pad;

#if VTXLAYOUT_TEX_HALF
	xt_half2    tex;
#else
	xt_texcoord tex;
	//xt_texcoord _tex_pad;
#endif

#if VTXLAYOUT_CLR_HALF
	xt_half4    clr;
#else
	xt_rgba     clr;
#endif

#if VTXLAYOUT_WGT_HALF
	xt_half4    wgt;
#else
	xt_float4   wgt;
#endif
	xt_int4     idx;
};

static GLuint s_progId = 0;

static GLint s_attrLocPos = -1;
static GLint s_attrLocNrm = -1;
static GLint s_attrLocTex = -1;
static GLint s_attrLocClr = -1;
static GLint s_attrLocWgt = -1;
static GLint s_attrLocIdx = -1;

static GLint s_gpLocViewProj = -1;
static GLint s_gpLocXforms = -1;
static GLint s_gpLocXformMap = -1;

static void prepare_model(sxModelData* pMdl) {
	if (!pMdl) return;
	GLuint* pBufIds = pMdl->get_gpu_wk<GLuint>();
	GLuint* pBufVB = &pBufIds[0];
	GLuint* pBufIB16 = &pBufIds[1];
	GLuint* pBufIB32 = &pBufIds[2];
	if (*pBufVB && (*pBufIB16 || *pBufIB32)) {
		return;
	}
	size_t vsize = pMdl->get_vtx_size();
	if (!(*pBufVB) && vsize > 0) {
		glGenBuffers(1, pBufVB);
		if (*pBufVB) {
			int npnt = pMdl->mPntNum;
			GPUVtx* pVtx = nxCore::tMem<GPUVtx>::alloc(npnt, "VtxTemp");
			if (pVtx) {
				for (int i = 0; i < npnt; ++i) {
					pVtx[i].pos = pMdl->get_pnt_pos(i);
					pVtx[i].nrm = pMdl->get_pnt_nrm(i);

#if VTXLAYOUT_TEX_HALF
					xt_texcoord tex = pMdl->get_pnt_tex(i);
					pVtx[i].tex.set(tex.u, tex.v);
#else
					pVtx[i].tex = pMdl->get_pnt_tex(i);
#endif

#if VTXLAYOUT_CLR_HALF
					xt_rgba clr = pMdl->get_pnt_clr(i);
					pVtx[i].clr.set(clr.r * 0.25f, clr.g * 0.62f, clr.b * 0.1f, clr.a); // greenish tint in half mode
#else
					pVtx[i].clr = pMdl->get_pnt_clr(i);
#endif

					xt_float4 wgt;
					wgt.fill(0.0f);
					pVtx[i].idx.fill(0);
					sxModelData::PntSkin skn = pMdl->get_pnt_skin(i);
					if (skn.num > 0) {
						for (int j = 0; j < skn.num; ++j) {
							wgt.set_at(j, skn.wgt[j]);
							pVtx[i].idx.set_at(j, skn.idx[j]);
						}
					} else {
						wgt[0] = 1.0f;
					}
#if VTXLAYOUT_WGT_HALF
					pVtx[i].wgt.set(wgt);
#else
					pVtx[i].wgt = wgt;
#endif
				}
			
				glBindBuffer(GL_ARRAY_BUFFER, *pBufVB);
				glBufferData(GL_ARRAY_BUFFER, npnt * sizeof(GPUVtx), pVtx, GL_STATIC_DRAW);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				nxCore::tMem<GPUVtx>::free(pVtx);
			}
		}
	}
	if (!(*pBufIB16) && pMdl->mIdx16Num > 0) {
		const uint16_t* pIdxData = pMdl->get_idx16_top();
		if (pIdxData) {
			glGenBuffers(1, pBufIB16);
			if (*pBufIB16) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *pBufIB16);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, pMdl->mIdx16Num * sizeof(uint16_t), pIdxData, GL_STATIC_DRAW);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
		}
	}
	if (!(*pBufIB32) && pMdl->mIdx32Num > 0) {
		const uint32_t* pIdxData = pMdl->get_idx32_top();
		if (pIdxData) {
			glGenBuffers(1, pBufIB32);
			if (*pBufIB32) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *pBufIB32);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, pMdl->mIdx32Num * sizeof(uint32_t), pIdxData, GL_STATIC_DRAW);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
		}
	}
}

static void release_model(sxModelData* pMdl) {
	if (!pMdl) return;
	GLuint* pBufIds = pMdl->get_gpu_wk<GLuint>();
	GLuint* pBufVB = &pBufIds[0];
	GLuint* pBufIB16 = &pBufIds[1];
	GLuint* pBufIB32 = &pBufIds[2];
	if (*pBufVB) {
		glDeleteBuffers(1, pBufVB);
		*pBufVB = 0;
	}
	if (*pBufIB16) {
		glDeleteBuffers(1, pBufIB16);
		*pBufIB16 = 0;
	}
	if (*pBufIB32) {
		glDeleteBuffers(1, pBufIB32);
		*pBufIB32 = 0;
	}
	pMdl->clear_tex_wk();
}

static void prepare_texture(sxTextureData* pTex) {
}

static void release_texture(sxTextureData* pTex) {

}

static GLuint load_shader(const char* pName) {
	GLuint sid = 0;
	const char* pDataPath = s_pRsrcMgr ? s_pRsrcMgr->get_data_path() : nullptr;
	if (pName) {
		char path[1024];
		XD_SPRINTF(XD_SPRINTF_BUF(path, sizeof(path)), "%s/ogl_vtx_test/%s", pDataPath ? pDataPath : ".", pName);
		size_t srcSize = 0;
		char* pSrc = (char*)nxCore::raw_bin_load(path, &srcSize);
		if (pSrc) {
			GLenum kind = nxCore::str_ends_with(pName, ".vert") ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
			sid = OGLSys::compile_shader_str(pSrc, srcSize, kind);
			nxCore::bin_unload(pSrc);
		}
	}
	return sid;
}

static void init_rsrc_mgr() {
	cxResourceManager::GfxIfc rsrcGfxIfc;
	rsrcGfxIfc.reset();
	rsrcGfxIfc.prepareTexture = prepare_texture;
	rsrcGfxIfc.releaseTexture = release_texture;
	rsrcGfxIfc.prepareModel = prepare_model;
	rsrcGfxIfc.releaseModel = release_model;
	s_pRsrcMgr->set_gfx_ifc(rsrcGfxIfc);
}

static void init_gpu_prog() {
	GLuint sidVert = load_shader("vtx.vert");
	GLuint sidFrag = load_shader("pix.frag");
	s_progId = OGLSys::link_draw_prog(sidVert, sidFrag);
	if (s_progId) {
		glDetachShader(s_progId, sidVert);
		glDetachShader(s_progId, sidFrag);
		s_attrLocPos = glGetAttribLocation(s_progId, "vtxPos");
		s_attrLocNrm = glGetAttribLocation(s_progId, "vtxNrm");
		s_attrLocTex = glGetAttribLocation(s_progId, "vtxTex");
		s_attrLocClr = glGetAttribLocation(s_progId, "vtxClr");
		s_attrLocWgt = glGetAttribLocation(s_progId, "vtxWgt");
		s_attrLocIdx = glGetAttribLocation(s_progId, "vtxIdx");
		s_gpLocViewProj = glGetUniformLocation(s_progId, "gpViewProj");
		s_gpLocXforms = glGetUniformLocation(s_progId, "gpXforms");
		s_gpLocXformMap = glGetUniformLocation(s_progId, "gpXformMap");
	}
	glDeleteShader(sidVert);
	glDeleteShader(sidFrag);
}

static void init(int shadowSize, cxResourceManager* pRsrcMgr, Draw::Font* pFont) {
	s_pRsrcMgr = pRsrcMgr;
	if (!pRsrcMgr) return;

	init_rsrc_mgr();
	init_gpu_prog();
}

static void reset() {
	glDeleteProgram(s_progId);
	s_progId = 0;
}

static int get_screen_width() {
	return OGLSys::get_width();
}

static int get_screen_height() {
	return OGLSys::get_height();
}

static cxMtx get_shadow_bias_mtx() {
	static const float bias[4 * 4] = {
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f
	};
	return nxMtx::from_mem(bias);
}

static void batch(cxModelWork* pWk, const int ibat, const Draw::Mode mode, const Draw::Context* pCtx) {
	if (!pCtx) return;
	if (!pWk) return;
	sxModelData* pMdl = pWk->mpData;
	if (!pMdl) return;
	if (!s_progId) return;

	if (mode == Draw::DRWMODE_SHADOW_CAST) return;

	const sxModelData::Batch* pBat = pMdl->get_batch_ptr(ibat);
	if (!pBat) return;

	prepare_model(pMdl);

	GLuint* pBufIds = pMdl->get_gpu_wk<GLuint>();
	GLuint bufVB = pBufIds[0];
	if (!bufVB) return;
	GLuint bufIB16 = pBufIds[1];
	GLuint bufIB32 = pBufIds[2];

	glUseProgram(s_progId);

	if (s_gpLocViewProj >= 0) {
		glUniformMatrix4fv(s_gpLocViewProj, 1, GL_FALSE, pCtx->view.mViewProjMtx);
	}
	if (s_gpLocXforms >= 0) {
		xt_xmtx xforms[MAX_BAT_XFORMS];
		float xmap[MAX_XFORMS];
		for (int i = 0; i < MAX_XFORMS; ++i) {
			xmap[i] = 0;
		}
		if (pWk->mpSkinXforms) {
			const int32_t* pJntLst = pMdl->get_batch_jnt_list(ibat);
			int njnt = pBat->mJntNum;
			for (int i = 0; i < njnt; ++i) {
				xforms[i] = pWk->mpSkinXforms[pJntLst[i]];
			}
			for (int i = 0; i < njnt; ++i) {
				xmap[pJntLst[i]] = float(i * 3);
			}
		} else if (pWk->mpWorldXform) {
			xforms[0] = *pWk->mpWorldXform;
		} else {
			xforms[0].identity();
		}
		glUniform4fv(s_gpLocXforms, MAX_XFORMS * 3, xforms[0]);
		glUniform4fv(s_gpLocXformMap, MAX_XFORMS / 4, xmap);
	}

	glBindBuffer(GL_ARRAY_BUFFER, bufVB);

	GLsizei stride = (GLsizei)sizeof(GPUVtx);
	size_t top = pBat->mMinIdx * stride;
	if (s_attrLocPos >= 0) {
		glEnableVertexAttribArray(s_attrLocPos);
		glVertexAttribPointer(s_attrLocPos, 3, GL_FLOAT, GL_FALSE, stride, (const void*)(top + offsetof(GPUVtx, pos)));
	}
	if (s_attrLocNrm >= 0) {
		glEnableVertexAttribArray(s_attrLocNrm);
		glVertexAttribPointer(s_attrLocNrm, 3, GL_FLOAT, GL_FALSE, stride, (const void*)(top + offsetof(GPUVtx, nrm)));
	}
	if (s_attrLocTex >= 0) {
		glEnableVertexAttribArray(s_attrLocTex);
#if VTXLAYOUT_TEX_HALF
		glVertexAttribPointer(s_attrLocTex, 2, GL_HALF_FLOAT, GL_FALSE, stride, (const void*)(top + offsetof(GPUVtx, tex)));
#else
		glVertexAttribPointer(s_attrLocTex, 2, GL_FLOAT, GL_FALSE, stride, (const void*)(top + offsetof(GPUVtx, tex)));
#endif
	}
	if (s_attrLocClr >= 0) {
		glEnableVertexAttribArray(s_attrLocClr);
#if VTXLAYOUT_CLR_HALF
		glVertexAttribPointer(s_attrLocClr, 4, GL_HALF_FLOAT, GL_FALSE, stride, (const void*)(top + offsetof(GPUVtx, clr)));
#else
		glVertexAttribPointer(s_attrLocClr, 4, GL_FLOAT, GL_FALSE, stride, (const void*)(top + offsetof(GPUVtx, clr)));
#endif
	}
	if (s_attrLocWgt >= 0) {
		glEnableVertexAttribArray(s_attrLocWgt);
#if VTXLAYOUT_WGT_HALF
		glVertexAttribPointer(s_attrLocWgt, 4, GL_HALF_FLOAT, GL_FALSE, stride, (const void*)(top + offsetof(GPUVtx, wgt)));
#else
		glVertexAttribPointer(s_attrLocWgt, 4, GL_FLOAT, GL_FALSE, stride, (const void*)(top + offsetof(GPUVtx, wgt)));
#endif
	}
	if (s_attrLocIdx >= 0) {
		glEnableVertexAttribArray(s_attrLocIdx);
		glVertexAttribPointer(s_attrLocIdx, 4, GL_INT, GL_FALSE, stride, (const void*)(top + offsetof(GPUVtx, idx)));
	}

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glCullFace(GL_BACK);

	intptr_t org = 0;
	GLenum typ = GL_NONE;
	if (pBat->is_idx16()) {
		if (!bufIB16) return;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIB16);
		org = pBat->mIdxOrg * sizeof(uint16_t);
		typ = GL_UNSIGNED_SHORT;
	} else {
		if (!bufIB32) return;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIB32);
		org = pBat->mIdxOrg * sizeof(uint32_t);
		typ = GL_UNSIGNED_INT;
	}
	glDrawElements(GL_TRIANGLES, pBat->mTriNum * 3, typ, (const void*)org);

	if (s_attrLocPos >= 0) {
		glDisableVertexAttribArray(s_attrLocPos);
	}
	if (s_attrLocNrm >= 0) {
		glDisableVertexAttribArray(s_attrLocNrm);
	}
	if (s_attrLocTex >= 0) {
		glDisableVertexAttribArray(s_attrLocTex);
	}
	if (s_attrLocClr >= 0) {
		glDisableVertexAttribArray(s_attrLocClr);
	}
	if (s_attrLocWgt >= 0) {
		glDisableVertexAttribArray(s_attrLocWgt);
	}
	if (s_attrLocIdx >= 0) {
		glDisableVertexAttribArray(s_attrLocIdx);
	}
}

static void begin(const cxColor& clearColor) {
	OGLSys::bind_def_framebuf();
	int w = get_screen_width();
	int h = get_screen_height();
	glViewport(0, 0, w, h);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

static void end() {
	OGLSys::swap();
}


static Draw::Ifc s_ifc;

struct DrwInit {
	DrwInit() {
		nxCore::mem_zero(&s_ifc, sizeof(s_ifc));
		s_ifc.info.pName = "ogl_vtx_test";
		s_ifc.info.needOGLContext = true;
		s_ifc.init = init;
		s_ifc.reset = reset;
		s_ifc.get_screen_width = get_screen_width;
		s_ifc.get_screen_height = get_screen_height;
		s_ifc.get_shadow_bias_mtx = get_shadow_bias_mtx;
		s_ifc.begin = begin;
		s_ifc.end = end;
		s_ifc.batch = batch;
		Draw::register_ifc_impl(&s_ifc);
	}
} s_drwInit;

DRW_IMPL_END
