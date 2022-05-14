/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

#include <crosscore.hpp>
#include <oglsys.hpp>
#include <scene.hpp>
#include <draw.hpp>
#include <demo.hpp>

#include "ostinato.hpp"

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
	Ostinato::init(argc, argv);
	exec_demo();
	Ostinato::reset();

	return 0;
}