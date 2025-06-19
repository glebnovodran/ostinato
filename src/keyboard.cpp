/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

#include <crosscore.hpp>
#include <oglsys.hpp>
#include "keyboard.hpp"

#define KEY_GET(_name) process_key(&mask, _name, #_name)

namespace Keyboard {

static struct KeyboardCtrl {

	uint64_t mNow;
	uint64_t mOld;
	bool mSkip;

	void init() {
		mNow = 0;
		mOld = 0;
		mSkip = false;
	}

	const char* get_alt_key_name(const int code) {
		static struct {
			int code;
			const char* pName;
		} tbl[] = {
			{ UP, "W" }, { DOWN, "S" }, { LEFT, "A" }, { RIGHT, "D" },
			{ SPACE, " " }, { LSHIFT, "RSHIFT" }
		};
		const char* pAltName = nullptr;
		for (size_t i = 0; i < XD_ARY_LEN(tbl); ++i) {
			if (code == tbl[i].code) {
				pAltName = tbl[i].pName;
				break;
			}
		}
		return pAltName;
	}

	void process_key(uint64_t* pMask, const int code, const char* pName) {
		if (!pMask || !pName) return;
		if (OGLSys::get_key_state(pName)) *pMask |= 1ULL << code;
		const char* pAltName = get_alt_key_name(code);
		if (pAltName) {
			if (OGLSys::get_key_state(pAltName)) *pMask |= 1ULL << code;
		}
	}

	void update() {
		if (mSkip) return;
		mOld = mNow;
		uint64_t mask = 0;
		KEY_GET(UP);
		KEY_GET(DOWN);
		KEY_GET(LEFT);
		KEY_GET(RIGHT);
		KEY_GET(LCTRL);
		KEY_GET(LSHIFT);
		KEY_GET(TAB);
		KEY_GET(BACK);
		KEY_GET(ENTER);
		KEY_GET(SPACE);
		mNow = mask;
	}

	bool ck_now(const int id) const { return !!(mNow & (1ULL << id)); }
	bool ck_old(const int id) const { return !!(mOld & (1ULL << id)); }
	bool ck_trg(const int id) const { return !!((mNow & (mNow ^ mOld)) & (1ULL << id)); }
	bool ck_chg(const int id) const { return !!((mNow ^ mOld) & (1ULL << id)); }

} s_kbdCtrl;

void init() { s_kbdCtrl.init(); }
void update() { s_kbdCtrl.update(); }

bool now_active(const int id) { return s_kbdCtrl.ck_now(id); }
bool was_active(const int id) { return s_kbdCtrl.ck_old(id); }
bool triggered(const int id) { return s_kbdCtrl.ck_trg(id); }
bool changed(const int id) { return s_kbdCtrl.ck_chg(id); }

} // Keyboard