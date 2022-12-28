/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

#include <crosscore.hpp>
#include <oglsys.hpp>
#include "perfmon.hpp"

namespace Performance {

XD_NOINLINE void CPUMonitor::echo_stats() const {
	if (mEchoEnabled && (mStatus != 0)) {
		double exe = get_median(Performance::Measure::EXE);
		double vis = get_median(Performance::Measure::VISIBILITY);
		double drw = get_median(Performance::Measure::DRAW);
		nxCore::dbg_msg("EXE: %.2f, VIS: %.2f, DRW: %.2f\n", exe, vis, drw);
	}
}

XD_NOINLINE void CPUMonitor::init() {
	mEchoEnabled = nxApp::get_bool_opt("perfmon_echo", false);
	int nsmps = nxApp::get_int_opt("perfmon_nsmps", 30);
	int n = nxCalc::max(1, nsmps);
	for (int i = 0; i < NUM_TIMER; ++i) {
		mTimers[i].alloc(n);
	}
	mStatus = 0;
}

XD_NOINLINE void CPUMonitor::free() {
	for (int i = 0; i < NUM_TIMER; ++i) {
		mTimers[i].free();
	}
}

XD_NOINLINE void CPUMonitor::begin(const Measure m) {
	mTimers[m].begin();
}

XD_NOINLINE void CPUMonitor::end(const Measure m) {
	uint32_t id = uint32_t(m);
	XD_BIT_ARY_CL(uint32_t, &mStatus, id);
	if (mTimers[m].end()) {
		double us = mTimers[m].median();
		mTimers[m].reset();
		mMedian[m] = us / 1000.0;
		XD_BIT_ARY_ST(uint32_t, &mStatus, id);
	}
}

XD_NOINLINE double CPUMonitor::get_median(const Measure m) const {
	return mMedian[m];
}

XD_NOINLINE void GPUMonitor::init() {
	mFlg = false;
	nxCore::mem_zero(mSmps, sizeof(mSmps));
	mIdx = 0;
	mMillis = 0.0;
	mTS0 = OGLSys::create_timestamp();
	mTS1 = OGLSys::create_timestamp();
}

XD_NOINLINE void GPUMonitor::begin() const {
	OGLSys::put_timestamp(mTS0);
}

XD_NOINLINE  void GPUMonitor::end() const {
	OGLSys::put_timestamp(mTS1);
}

XD_NOINLINE void GPUMonitor::exec() {
	if (mFlg) {
		OGLSys::wait_timestamp(mTS1);
		uint64_t t0 = OGLSys::get_timestamp(mTS0);
		uint64_t t1 = OGLSys::get_timestamp(mTS1);
		uint64_t dt = t1 - t0;
		double millis = double(dt) * 1.0e-6;
		int nsmp = XD_ARY_LEN(mSmps);
		if (mIdx < nsmp) {
			mSmps[mIdx] = millis;
			++mIdx;
		} else {
			mIdx = 0;
			mMillis = nxCalc::median(mSmps, nsmp);
		}
	}
	mFlg = true;
}

XD_NOINLINE void GPUMonitor::free() {
	OGLSys::delete_timestamp(mTS0);
	OGLSys::delete_timestamp(mTS1);
	mTS0 = 0;
	mTS1 = 0;
}


} // Performance