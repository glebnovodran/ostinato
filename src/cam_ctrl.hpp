/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

namespace CamCtrl {

struct CamCtx {
	ScnObj* mpTgtObj;
	uint32_t mCamMode;
};
void init();
void exec(const CamCtx* pCtx);

};