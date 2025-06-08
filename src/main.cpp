/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

#include <crosscore.hpp>
#include <oglsys.hpp>
#include <scene.hpp>
#include <draw.hpp>
#include <demo.hpp>

#ifdef OGLSYS_MACOS
#	include "mac_ifc.h"
#endif

#include "ostinato.hpp"


#ifdef OGLSYS_MACOS

static Demo::Ifc* s_pDemoIfc = nullptr;

void mac_start(int argc, const char* argv[]) {
	Ostinato::mac_start(argc, argv);
}

void mac_init(const char* pAppPath) {
	const char* argv[] = { pAppPath };
	Ostinato::init(1, (char**)argv);
	Demo::Ifc* pIfc = Demo::get_demo();
	if (pIfc) {
		nxCore::dbg_msg("executing demo program: %s\n", pIfc->info.pName);
		pIfc->init();
		Scene::mem_info();
	}
	s_pDemoIfc = pIfc;
}

void mac_exec() {
	if (s_pDemoIfc && s_pDemoIfc->loop) {
		s_pDemoIfc->loop(&s_pDemoIfc->info);
	}
}

void mac_stop() {
	if (s_pDemoIfc) {
		s_pDemoIfc->reset();
		s_pDemoIfc = nullptr;
	}
	Ostinato::reset();
}

int mac_get_int_opt(const char* pName) {
	return nxApp::get_int_opt(pName, 0);
}

bool mac_get_bool_opt(const char* pName) {
	return nxApp::get_bool_opt(pName, false);
}

float mac_get_float_opt(const char* pName) {
	return nxApp::get_float_opt(pName, 0.0f);
}

int mac_get_width_opt() {
	return nxCalc::max(32, nxApp::get_int_opt("w", Ostinato::get_def_width()));
}

int mac_get_height_opt() {
	return nxCalc::max(32, nxApp::get_int_opt("h", Ostinato::get_def_height()));
}

static float mac_mouse_y(float y) {
	return float(OGLSys::get_height()) - y;
}

void mac_mouse_down(int btn, float x, float y) {
	OGLSys::send_mouse_up((OGLSysMouseState::BTN)btn, x, mac_mouse_y(y), false);
}

void mac_mouse_up(int btn, float x, float y) {
	OGLSys::send_mouse_up((OGLSysMouseState::BTN)btn, x, mac_mouse_y(y), false);
}

void mac_mouse_move(float x, float y) {
	OGLSys::send_mouse_move(x, mac_mouse_y(y));
}

void mac_kbd(const char* pName, const bool state) {
	OGLSys::set_key_state(pName, state);
}

#else // OGLSYS_MACOS

static void exec_demo() {
	Demo::Ifc* pIfc = Demo::get_demo();
	if (pIfc) {
		nxCore::dbg_msg("executing demo program: %s\n", pIfc->info.pName);
		pIfc->init();
		Scene::mem_info();
		OGLSys::loop(pIfc->loop);
		pIfc->reset();
	}
}

int main(int argc, char* argv[]) {
	if (3 == argc && nxCore::str_eq(argv[1], "exeinfo")) {
		return Ostinato::exe_info(argv[2]);
	}
	Ostinato::init(argc, argv);
	exec_demo();
	Ostinato::reset();

	return 0;
}

#endif // OGLSYS_MACOS
