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
	bool mDoEcho;
	bool mEchoFPS;

	void init(TimeCtrl::Frequency freq, int smps) {
		mFreq = freq;
		mTimeFixUpFlg = true;
		mFramerateStopWatch.alloc(smps);
		mMedianFPS = 0.0f;
		mMotSpeed = 1.0f;
		mEchoFPS = false;
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
			mDoEcho = true;
			double us = mFramerateStopWatch.median();
			mFramerateStopWatch.reset();
			double millis = us / 1000.0;
			double secs = millis / 1000.0;
			double fps = nxCalc::rcp0(secs);
			mMedianFPS = float(fps);
		}
		mFramerateStopWatch.begin();

		if (mFreq == Frequency::VARIABLE) {
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
		} else {
			motSpeed = float(mFreq);
			frate = 60.0f / motSpeed;
			mCurrentTime += 1000.0 / frate;
			mMotSpeed = motSpeed;
		}
	}

} s_wk;

void init() {
	Frequency tfreq;

	int freq = nxApp::get_int_opt("tfreq", 0);
	switch (freq) {
		case 1:
			tfreq = TimeCtrl::Frequency::FIXED_60;
			break;
		case 2:
			tfreq = TimeCtrl::Frequency::FIXED_30;
			break;
		case 3:
			tfreq = TimeCtrl::Frequency::FIXED_20;
			break;
		case 4:
			tfreq = TimeCtrl::Frequency::FIXED_15;
			break;
		case 6:
			tfreq = TimeCtrl::Frequency::FIXED_10;
			break;
		case 0:
		default:
			tfreq = TimeCtrl::Frequency::VARIABLE;
	}

	int smps = nxApp::get_int_opt("tsmps", 10);
	smps = nxCalc::max(1, smps);
	s_wk.init(tfreq, smps);
	s_wk.mEchoFPS = nxApp::get_bool_opt("echo_fps", false);
}

void reset() {
	s_wk.reset();
}

void exec() {
	s_wk.exec();
}

float get_fps() { return s_wk.mMedianFPS; }

void get_fps_str(char* pBuf) {
	if (pBuf) {
		if (s_wk.mMedianFPS < 0.0f) {
			XD_SPRINTF(XD_SPRINTF_BUF(pBuf, sizeof(pBuf)), " --");
		} else {
			XD_SPRINTF(XD_SPRINTF_BUF(pBuf, sizeof(pBuf)), "%.2f", s_wk.mMedianFPS);
		}
	}
}

float get_motion_speed() { return s_wk.mMotSpeed * Scene::speed(); }

double get_start_time() { return s_wk.mFreq == Frequency::VARIABLE ? get_sys_time_millis() : 0.0; }

double get_current_time() { return s_wk.mCurrentTime; }

double get_sys_time_millis() { return nxSys::time_micros() / 1000.0; }

void echo_fps(const char* pFmt) {
	char fpsStr[16];
	if (s_wk.mEchoFPS && s_wk.mDoEcho) {
		get_fps_str(fpsStr);
		nxCore::dbg_msg(pFmt, fpsStr);
		s_wk.mDoEcho = false;
	}
}

};
