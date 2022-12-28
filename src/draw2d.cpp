/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

#include <crosscore.hpp>
#include <scene.hpp>
#include <oglsys.hpp>

#include "timectrl.hpp"
#include "perfmon.hpp"

static struct DrawWk {
	Performance::CPUMonitor* pPerfCPU;
	Performance::GPUMonitor* pPerfGPU;
	bool showPerf;
	float resScale;
} s_wk = {};

namespace Draw2D {

void init(Performance::CPUMonitor* pPerfCPU, Performance::GPUMonitor* pPerfGPU) {
	s_wk.pPerfCPU = pPerfCPU;
	s_wk.pPerfGPU = pPerfGPU;
	s_wk.showPerf = nxApp::get_bool_opt("showperf", false);
	s_wk.resScale = nxApp::get_float_opt("res2d_scl", 1.0f);
}

void exec() {
	char str[1024];

	if (s_wk.resScale <= 0.0f) {
		return;
	}

	float refSizeX = 800 * s_wk.resScale;
	float refSizeY = 600 * s_wk.resScale;
	Scene::set_ref_scr_size(refSizeX, refSizeY);
	if (Scene::get_view_mode_width() < Scene::get_view_mode_height()) {
		Scene::set_ref_scr_size(refSizeY, refSizeX);
	}

	if (s_wk.showPerf) {
		char fpsStr[32];

		TimeCtrl::get_fps_str(fpsStr, sizeof(fpsStr));

		double exe = s_wk.pPerfCPU->get_median(Performance::Measure::EXE);
		double vis = s_wk.pPerfCPU->get_median(Performance::Measure::VISIBILITY);
		double drw = s_wk.pPerfCPU->get_median(Performance::Measure::DRAW);
		double gpu = s_wk.pPerfGPU->mMillis;

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

void reset() {
	s_wk = {0};
}

} // Draw2D