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
	bool mEcho;

	void init();

	void free();

	void begin(const Measure m);

	void end(const Measure m);

	double get_median(const Measure m) const;

	void echo_cpu_stat() const;
};

struct GPUMonitor {

	double mSmps[30];
	int mIdx;
	uint32_t mTS0;
	uint32_t mTS1;
	double mMillis;
	bool mFlg;

	void init();

	void begin() const;

	void end() const;

	void exec();

	void free();
};

} // namespace