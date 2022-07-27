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

	cxStopWatch mTimers[NUM_TIMER];
	double mMedian[NUM_TIMER];

	XD_NOINLINE void init();

	XD_NOINLINE void free();

	XD_NOINLINE void begin(const Measure m);

	XD_NOINLINE void end(const Measure m);

	XD_NOINLINE double get_median(const Measure m) const;
};

struct GPUMonitor {

	double mSmps[30];
	int mIdx;
	uint32_t mTS0;
	uint32_t mTS1;
	double mMillis;
	bool mFlg;

	XD_NOINLINE void init();

	XD_NOINLINE void begin() const;

	XD_NOINLINE void end() const;

	XD_NOINLINE void exec();

	XD_NOINLINE void free();
};

} // namespace