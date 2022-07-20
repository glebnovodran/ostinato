/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

namespace Draw2D {

	void init(Performance::CPUMonitor* pPerfCPU, Performance::GPUMonitor* pPerfGPU);

	void exec();
	
	void reset();
}