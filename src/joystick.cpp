/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

#include <crosscore.hpp>

#if defined(XD_SYS_LINUX)

#include <cstdarg>
//#include <cstdlib>
#	include <cstdio>
#	include <cerrno>

#	include <unistd.h>
#	include <fcntl.h>
#	include <linux/input.h>
#	include <linux/joystick.h>

#define KEY_MAX_LARGE 0x2FF
#define KEY_MAX_SMALL 0x1FF
#define AXMAP_SIZE (ABS_MAX + 1)
#define BTNMAP_SIZE (KEY_MAX_LARGE - BTN_MISC + 1)

#define JSIOCGBTNMAP_LARGE _IOR('j', 0x34, __u16[KEY_MAX_LARGE - BTN_MISC + 1])
#define JSIOCGBTNMAP_SMALL _IOR('j', 0x34, __u16[KEY_MAX_SMALL - BTN_MISC + 1])

#define DEF_JOYSTICK_DEVPATH "/dev/input/js0"
#endif

#include "joystick.hpp"


namespace Joystick {

struct JoystickCtrl {

#if defined(XD_SYS_LINUX)
	int32_t* mpAxisVal;
	uint64_t mNow;
	uint64_t mOld;

	int32_t mFd;
	int32_t mBtnMapIoctl;

	uint16_t mBtnmap[BTNMAP_SIZE];
	uint8_t mAxmap[AXMAP_SIZE];

	unsigned char mNumAxis;
	unsigned char mNumBtns;

	JoystickCtrl() : mNumAxis(0), mNumBtns(0), mFd(-1), mBtnMapIoctl(-1), mNow(0ULL), mOld(0ULL) {}

	void init() {
		const char* pJstDevOpt = nxApp::get_opt("jst_dev");
		const char* pDevPath = pJstDevOpt ? pJstDevOpt : DEF_JOYSTICK_DEVPATH;
		mFd = ::open(pDevPath, O_RDONLY);
		if (mFd < 0) {
			nxCore::dbg_msg("Can't initialize the joystick at %s", pDevPath);
			return;
		}

		int ver = 0x000800;
		char name[128] = "Eni(g)ma";

		ioctl(mFd, JSIOCGVERSION, &ver);
		ioctl(mFd, JSIOCGAXES, &mNumAxis);
		ioctl(mFd, JSIOCGBUTTONS, &mNumBtns);
		ioctl(mFd, JSIOCGNAME(128), name);

		mpAxisVal = reinterpret_cast<int32_t*>(nxCore::mem_alloc(mNumAxis * sizeof(int32_t), "JSTK"));

		nxCore::dbg_msg("\nThe Ostinato city is controlled by\n");
		nxCore::dbg_msg("\t%s at %s\n", name, pDevPath);

		nxCore::dbg_msg("\tDriver version: %d.%d.%d\n", ver >> 16, (ver >> 8) & 0xff, ver & 0xff);
		nxCore::dbg_msg("\tAxis num: %d\n", mNumAxis);
		nxCore::dbg_msg("\tButtons num: %d\n", mNumBtns);

		int res = ioctl(mFd, JSIOCGAXMAP, mAxmap);
		nxCore::dbg_msg("ioctl JSIOCGAXMAP : %d\n", res);

		unsigned long ioctls[3] = { JSIOCGBTNMAP, JSIOCGBTNMAP_LARGE, JSIOCGBTNMAP_SMALL };
		int i = 0;
		for (res = -1; i < 3; ++i) {
			res = ioctl(mFd, ioctls[i], mBtnmap);
			if (res >= 0) {
				mBtnMapIoctl = ioctls[i];
				break;
			}
		}

		nxCore::dbg_msg("button map ioctl %X : %d\n", mBtnMapIoctl, res);

		fcntl(mFd, F_SETFL, O_NONBLOCK);
	}

	void update() {
		js_event jse;
		if (mFd < 0) { return; }
		mOld = mNow;
		while (::read(mFd, &jse, sizeof(js_event)) == sizeof(js_event) ) {
			nxCore::dbg_msg("Joystick event : type %d, time %d, number %d, value %d\n", jse.type, jse.time, jse.number, jse.value);
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
		if (mFd>=0) { close(mFd); }
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

int get_num_axis() {
	return s_JtkCtrl.mNumAxis;
}
int get_axis_val(unsigned char axis) {
	return s_JtkCtrl.mpAxisVal[axis];
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

int get_num_axis() { return 0; }
int get_num_buttons() {return 0; }

int get_axis_val(unsigned char axis) { return 0; }

bool now_active(const int btid) { return false; }
bool was_active(const int btid) { return false; }
bool triggered(const int btid) { return false; }
bool changed(const int btid) { return false; }

#endif
}