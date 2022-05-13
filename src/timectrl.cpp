/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

#include <crosscore.hpp>
#include <scene.hpp>
#include "timectrl.hpp"

struct Wk {
	cxStopWatch mFramerateStopWatch;
	double mCurrentTime;
	float mMedianFPS;
	float mMotSpeed;
	TimeCtrl::Frequency mFreq;
	bool mTimeFixUpFlg;

	void init(TimeCtrl::Frequency freq) {
		mFreq = freq;
		mTimeFixUpFlg = true;
		mFramerateStopWatch.alloc(10);
		mMedianFPS = 0.0f;
		mMotSpeed = 1.0f;
	}

	void reset() {
		mFramerateStopWatch.free();
	}

	void exec() {
		using namespace TimeCtrl;
		if (mFreq == Frequency::VARIABLE) {
			mCurrentTime = TimeCtrl::get_sys_time_millis();
		}
		if (mFramerateStopWatch.end()) {
			double us = mFramerateStopWatch.median();
			mFramerateStopWatch.reset();
			double millis = us / 1000.0;
			double secs = millis / 1000.0;
			double fps = nxCalc::rcp0(secs);
			mMedianFPS = float(fps);
		}
		mFramerateStopWatch.begin();
		if (mFreq == Frequency::FIXED_60) {
			mCurrentTime += 1000.0 / 60.0f;
			mMotSpeed = 1.0f;
		} else if (mFreq == Frequency::FIXED_30) {
			mCurrentTime += 1000.0 / 30.0f;
			mMotSpeed = 2.0f;
		} else { // VARIABLE
			if (mMedianFPS > 0.0f) {
				if (mMedianFPS >= 57.0f && mMedianFPS <= 62.0f) {
					mMotSpeed = 1.0f;
				} else if (mMedianFPS >= 29.0f && mMedianFPS <= 31.0f) {
					mMotSpeed = 2.0f;
				} else {
					mMotSpeed = nxCalc::div0(60.0f, mMedianFPS);
				}
			} else {
				mMotSpeed = 1.0f;
			}
		}
	}

} s_wk;

namespace TimeCtrl {

static bool s_initFlg = false;

void init() {
	if (s_initFlg) return;
	int freq = nxApp::get_int_opt("tfreq", 0);
	TimeCtrl::Frequency tfreq = TimeCtrl::Frequency(freq < TimeCtrl::NUM_MODES ? freq : 0);
	s_wk.init(tfreq);
	s_initFlg = true;
}

void reset() {
	if (!s_initFlg) return;
	s_wk.reset();
	s_initFlg = false;
}

void exec() {
	s_wk.exec();
}

float get_fps() { return s_wk.mMedianFPS; }

float get_motion_speed() { return s_wk.mMotSpeed * Scene::speed(); }

double get_start_time() { return s_wk.mFreq == Frequency::VARIABLE ? get_sys_time_millis() : 0.0; }

double get_current_time() { return s_wk.mCurrentTime; }

double get_sys_time_millis() { return nxSys::time_micros() / 1000.0; }

};
