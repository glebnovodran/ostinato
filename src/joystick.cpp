/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

#include <crosscore.hpp>

#if defined(XD_SYS_LINUX)

#	include <stdarg.h>
#	include <stdio.h>
#	include <errno.h>
#	include <unistd.h>
#	include <fcntl.h>
#	include <linux/input.h>
#	include <linux/joystick.h>

#	define KEY_MAX_LARGE 0x2FF
#	define KEY_MAX_SMALL 0x1FF
#	define AXMAP_SIZE (ABS_MAX + 1)
#	define BTNMAP_SIZE (KEY_MAX_LARGE - BTN_MISC + 1)

#	define JSIOCGBTNMAP_LARGE _IOR('j', 0x34, __u16[KEY_MAX_LARGE - BTN_MISC + 1])
#	define JSIOCGBTNMAP_SMALL _IOR('j', 0x34, __u16[KEY_MAX_SMALL - BTN_MISC + 1])

#	define DEF_JOYSTICK_DEVPATH "/dev/input/js0"
#endif

#include "joystick.hpp"


namespace Joystick {

struct JoystickCtrl {

#if defined(XD_SYS_LINUX)
	int32_t* mpAxisVal;
	int32_t* mpAxisOldVal;
	uint64_t mNow;
	uint64_t mOld;

	int32_t mFd;
	int32_t mBtnMapIoctl;

	uint16_t mBtnmap[BTNMAP_SIZE];
	uint8_t mAxmap[AXMAP_SIZE];

	uint32_t mNumAxis;
	uint32_t mNumBtns;

	bool mDbgEcho;

	JoystickCtrl() : mpAxisVal(nullptr), mNow(0ULL), mOld(0ULL), mFd(-1), mBtnMapIoctl(-1), mNumAxis(0), mNumBtns(0), mDbgEcho(false) {}

	void init() {
		const char* pJstDevOpt = nxApp::get_opt("jst_dev");
		mDbgEcho = nxApp::get_bool_opt("jst_echo", false);
		const char* pDevPath = pJstDevOpt ? pJstDevOpt : DEF_JOYSTICK_DEVPATH;
		mFd = ::open(pDevPath, O_RDONLY);
		if (mFd < 0) {
			jstk_dbg_msg("Can't initialize the joystick at %s", pDevPath);
			return;
		} else {
			jstk_dbg_msg("File descriptor %d", mFd);
		}

		int ver = 0x000800;
		char name[128] = "Unknown";

		ioctl(mFd, JSIOCGVERSION, &ver);
		ioctl(mFd, JSIOCGAXES, &mNumAxis);
		ioctl(mFd, JSIOCGBUTTONS, &mNumBtns);
		ioctl(mFd, JSIOCGNAME(128), name);

		mpAxisVal = reinterpret_cast<int32_t*>(nxCore::mem_alloc(mNumAxis * sizeof(int32_t), "JSTK"));
		mpAxisOldVal = reinterpret_cast<int32_t*>(nxCore::mem_alloc(mNumAxis * sizeof(int32_t), "JSTK"));

		jstk_dbg_msg("\nThe Ostinato city is controlled by\n");
		jstk_dbg_msg("\t%s at %s\n", name, pDevPath);

		jstk_dbg_msg("\tDriver version: %d.%d.%d\n", ver >> 16, (ver >> 8) & 0xff, ver & 0xff);
		jstk_dbg_msg("\tAxis num: %d\n", mNumAxis);
		jstk_dbg_msg("\tButtons num: %d\n", mNumBtns);

		int res = ioctl(mFd, JSIOCGAXMAP, mAxmap);
		jstk_dbg_msg("ioctl JSIOCGAXMAP : %d\n", res);

		unsigned long ioctls[3] = { JSIOCGBTNMAP, JSIOCGBTNMAP_LARGE, JSIOCGBTNMAP_SMALL };
		int i = 0;
		for (res = -1; i < 3; ++i) {
			res = ioctl(mFd, ioctls[i], mBtnmap);
			if (res >= 0) {
				mBtnMapIoctl = ioctls[i];
				break;
			}
		}

		jstk_dbg_msg("button map ioctl %X : %d\n", mBtnMapIoctl, res);

		fcntl(mFd, F_SETFL, O_NONBLOCK);
	}

	void update() {
		js_event jse;
		if (mFd < 0) { return; }
		mOld = mNow;
		nxCore::mem_copy(mpAxisOldVal, mpAxisVal, mNumAxis * sizeof(int32_t));
		while (::read(mFd, &jse, sizeof(js_event)) == sizeof(js_event) ) {
			jstk_dbg_msg("Joystick event : type %d, time %d, number %d, value %d\n", jse.type, jse.time, jse.number, jse.value);
			switch (jse.type & ~JS_EVENT_INIT) {
				case JS_EVENT_AXIS:
					mpAxisVal[jse.number] = jse.value;
					break;
				case JS_EVENT_BUTTON:
					XD_BIT_ARY_ST(uint64_t, mBtnmap, jse.value);
					break;
			}
		}
	}

	void reset() {
		if (mpAxisVal) { nxCore::mem_free(mpAxisVal); }
		if (mpAxisOldVal) { nxCore::mem_free(mpAxisOldVal); }
		if (mFd>=0) { close(mFd); }
	}

	bool ck_now(const int btid) const { return !!(mNow & (1ULL << btid)); }
	bool ck_old(const int btid) const { return !!(mOld & (1ULL << btid)); }
	bool ck_trg(const int btid) const { return !!((mNow & (mNow ^ mOld)) & (1ULL << btid)); }
	bool ck_chg(const int btid) const { return !!((mNow ^ mOld) & (1ULL << btid)); }

	void jstk_dbg_msg(const char* pFmt, ...) {
		if (mDbgEcho) {
			char msg[1024 * 2];
			va_list mrk;
			va_start(mrk, pFmt);
#if defined(_MSC_VER)
			::vsprintf_s(msg, sizeof(msg), pFmt, mrk);
#else
			::vsprintf(msg, pFmt, mrk);
#endif
			nxCore::dbg_msg(msg);
			va_end(mrk);
		}
	}
#else
	void init() {}
	void update() {}
	void reset() {}
#endif

} s_JtkCtrl;


void init() {
	s_JtkCtrl.init();
}

void update() {
	s_JtkCtrl.update();
}
void reset() {
	s_JtkCtrl.reset();
}

#if defined(XD_SYS_LINUX)

uint32_t get_num_axis() {
	return s_JtkCtrl.mNumAxis;
}

uint32_t get_num_buttons() {
	return s_JtkCtrl.mNumBtns;
}

int get_axis_val(const uint32_t axis) {
	return axis < get_num_axis() ? s_JtkCtrl.mpAxisVal[axis] : 0;
}

int get_axis_old_val(const uint32_t axis) {
	return axis < get_num_axis() ? s_JtkCtrl.mpAxisOldVal[axis] : 0;
}

int get_axis_diff(const uint32_t axis) {
	return axis < get_num_axis() ? s_JtkCtrl.mpAxisVal[axis] - s_JtkCtrl.mpAxisOldVal[axis] : 0;
}

bool now_active(const int btid) {
	return s_JtkCtrl.ck_now(btid);
}
bool was_active(const int btid) {
	return s_JtkCtrl.ck_old(btid);
}
bool triggered(const int btid) {
	return s_JtkCtrl.ck_trg(btid);
}
bool changed(const int btid){
	return s_JtkCtrl.ck_chg(btid);
}
# else

uint32_t get_num_axis() { return 0; }
uint32_t get_num_buttons() {return 0; }

int get_axis_val(const uint32_t axis) { return 0; }
int get_axis_old_val(const uint32_t axis) { return 0; }

bool now_active(const int btid) { return false; }
bool was_active(const int btid) { return false; }
bool triggered(const int btid) { return false; }
bool changed(const int btid) { return false; }

#endif
} // Joystick