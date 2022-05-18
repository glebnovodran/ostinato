/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

namespace Performance {

enum Measure {
	EXE = 0,
	VISIBILITY,
	DRAW,
	NUM_TIMER
};

struct CPUMonitor {
	cxStopWatch mAll[NUM_TIMER];
	double mMedian[NUM_TIMER];

	void init() {
		for (int i = 0; i < NUM_TIMER; ++i) {
			mAll[i].alloc(30);
		}
	}
	void free() {
		for (int i = 0; i < NUM_TIMER; ++i) {
			mAll[i].free();
		}
	}
	void begin(const Measure m) {
		mAll[m].begin();
	}
	void end(const Measure m) {
		if (mAll[m].end()) {
			double us = mAll[m].median();
			mAll[m].reset();
			mMedian[m] = us / 1000.0;
		}
	}
	double get_median(const Measure m) const {
		return mMedian[m];
	}
};

struct GPUMonitor {
	double mSmps[30];
	int mIdx;
	uint32_t mTS0;
	uint32_t mTS1;
	double mMillis;
	bool mFlg;

	void init() {
		mFlg = false;
		nxCore::mem_zero(mSmps, sizeof(mSmps));
		mIdx = 0;
		mMillis = 0.0;
		mTS0 = OGLSys::create_timestamp();
		mTS1 = OGLSys::create_timestamp();
	}

	void begin() const { OGLSys::put_timestamp(mTS0); }
	void end() const { OGLSys::put_timestamp(mTS1); }

	void exec() {
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

	void free() {
		OGLSys::delete_timestamp(mTS0);
		OGLSys::delete_timestamp(mTS1);
		mTS0 = 0;
		mTS1 = 0;
	}
};

} // namespace