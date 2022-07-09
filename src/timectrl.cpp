/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

#include <crosscore.hpp>
#include <scene.hpp>
#include "timectrl.hpp"

namespace TimeCtrl {

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
		float motSpeed, frate;

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

		switch (mFreq) {
			case Frequency::FIXED_60:
			case Frequency::FIXED_30:
			case Frequency::FIXED_20:
			case Frequency::FIXED_15:
			case Frequency::FIXED_10:
				motSpeed = int(mFreq);
				frate = 60.0f / motSpeed;
				mCurrentTime += 1000.0 / frate;
				mMotSpeed = motSpeed;
				break;
			case Frequency::VARIABLE:
			default:
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

static bool s_initFlg = false;

void init() {
	if (s_initFlg) return;

	Frequency tfreq = Frequency::VARIABLE;
	int freq = nxApp::get_int_opt("tfreq", 0);
	switch (freq) {
		case 1:
			tfreq = Frequency::FIXED_60;
			break;
		case 2:
			tfreq = Frequency::FIXED_30;
			break;
		case 3:
			tfreq = Frequency::FIXED_20;
			break;
		case 4:
			tfreq = Frequency::FIXED_15;
			break;
		case 6:
			tfreq = Frequency::FIXED_10;
			break;
		case 0:
		default:
			tfreq = Frequency::VARIABLE;
	}

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
