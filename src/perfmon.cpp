/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

#include <crosscore.hpp>
#include <oglsys.hpp>
#include "perfmon.hpp"

namespace Performance {

void CPUMonitor::init() {
	for (int i = 0; i < NUM_TIMER; ++i) {
		mTimers[i].alloc(30);
	}
}
void CPUMonitor::free() {
	for (int i = 0; i < NUM_TIMER; ++i) {
		mTimers[i].free();
	}
}
void CPUMonitor::begin(const Measure m) {
	mTimers[m].begin();
}
void CPUMonitor::end(const Measure m) {
	if (mTimers[m].end()) {
		double us = mTimers[m].median();
		mTimers[m].reset();
		mMedian[m] = us / 1000.0;
	}
}
double CPUMonitor::get_median(const Measure m) const {
	return mMedian[m];
}


void GPUMonitor::init() {
	mFlg = false;
	nxCore::mem_zero(mSmps, sizeof(mSmps));
	mIdx = 0;
	mMillis = 0.0;
	mTS0 = OGLSys::create_timestamp();
	mTS1 = OGLSys::create_timestamp();
}

void GPUMonitor::begin() const { OGLSys::put_timestamp(mTS0); }
void GPUMonitor::end() const { OGLSys::put_timestamp(mTS1); }

void GPUMonitor::exec() {
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

void GPUMonitor::free() {
	OGLSys::delete_timestamp(mTS0);
	OGLSys::delete_timestamp(mTS1);
	mTS0 = 0;
	mTS1 = 0;
}


} // namespace