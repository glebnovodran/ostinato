/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */
#include <crosscore.hpp>

#include "ostinato.hpp"

static void dbgmsg(const char* pMsg) {
	::printf("%s", pMsg);
	::fflush(stdout);
}

namespace Ostinato {
	void init() {
		sxSysIfc sysIfc;
		::memset(&sysIfc, 0, sizeof(sysIfc));
		sysIfc.fn_dbgmsg = dbgmsg;
		nxSys::init(&sysIfc);
	}

	void reset() {

	}
}; // Ostinato