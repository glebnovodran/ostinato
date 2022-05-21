/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

#include <crosscore.hpp>
#include <scene.hpp>
#include <demo.hpp>
#include <smprig.hpp>

#include "timectrl.hpp"
#include "human.hpp"

namespace Citizen {

void roam_ctrl(Human* pHuman) {
	if (!pHuman) return;
	double objTouchDT = pHuman->get_obj_touch_duration_secs();
	double wallTouchDT = pHuman->get_wall_touch_duration_secs();

	switch (pHuman->mAction) {
		case Human::ACT_STAND:
			if (pHuman->mActionTimer.check_time_out()) {
				uint64_t rng = Scene::glb_rng_next() & 0x3f;
				if (rng < 0x10) {
					float t = nxCalc::fit(Scene::glb_rng_f01(), 0.0f, 1.0f, 1.0f, 4.0f);
					pHuman->change_act(Human::ACT_RUN, t);
				} else if (rng < 0x38) {
					float t = nxCalc::fit(Scene::glb_rng_f01(), 0.0f, 1.0f, 2.0f, 6.0f);
					pHuman->change_act(Human::ACT_WALK, t);
					pHuman->reset_wall_touch();
				} else {
					float t = nxCalc::fit(Scene::glb_rng_f01(), 0.0f, 1.0f, 0.5f, 2.0f);
					if (rng < 0x3C) {
						pHuman->change_act(Human::ACT_TURN_L, t);
					} else {
						pHuman->change_act(Human::ACT_TURN_R, t);
					}
				}
			}
			break;
		case Human::ACT_WALK: 
			if (objTouchDT > 0.2 || wallTouchDT > 0.25) {
				if (objTouchDT > 0 && ((Scene::glb_rng_next() & 0x3F) < 0x1F)) {
					pHuman->change_act(Human::ACT_RETREAT, 0.5f);
				} else {
					float t = nxCalc::fit(Scene::glb_rng_f01(), 0.0f, 1.0f, 0.5f, 2.0f);
					if (Scene::glb_rng_next() & 0x1) {
						pHuman->change_act(Human::ACT_TURN_L, t);
					} else {
						pHuman->change_act(Human::ACT_TURN_R, t);
					}
				}
			} else {
				ScnObj* pObj = pHuman->mpObj;

				if (pObj->mRoutine[0] == 0) {
					if (!pHuman->mActionTimer.check_time_out()) {
						bool rotFlg = int(Scene::glb_rng_next() & 0xFF) < 0x10;
						if (rotFlg) {
							pObj->mRoutine[0] = 1;
							float ryAdd = 0.5f;
							if (Scene::glb_rng_next() & 1) {
								ryAdd = -ryAdd;
							}
							pObj->mFltWk[0] = ryAdd;
							float t = nxCalc::fit(Scene::glb_rng_f01(), 0.0f, 1.0f, 0.5f, 1.0f);
							pHuman->mAuxTimer.start(t);
						}
					}
				} else {
					if (pHuman->mAuxTimer.check_time_out()) {
						pObj->mRoutine[0] = 0;
						pObj->mFltWk[0] = 0.0f;
					} else {
						pHuman->add_deg_y(pObj->mFltWk[0]);
					}
				}
			}
			break;
		case Human::ACT_RUN:
			if (pHuman->mActionTimer.check_time_out() || wallTouchDT > 0.1 || objTouchDT > 0.1) {
				bool standFlg = int(Scene::glb_rng_next() & 0xF) < 0x4;
				if (standFlg) {
					float t = nxCalc::fit(Scene::glb_rng_f01(), 0.0f, 1.0f, 1.0f, 2.0f);
					pHuman->change_act(Human::ACT_STAND, t);
				} else {
					float t = nxCalc::fit(Scene::glb_rng_f01(), 0.0f, 1.0f, 1.5f, 2.5f);
					pHuman->change_act(Human::ACT_WALK, t);
				}
			}
			break;
		case Human::ACT_RETREAT:
			if (pHuman->mActionTimer.check_time_out() || wallTouchDT > 0.1) {
				float t = nxCalc::fit(Scene::glb_rng_f01(), 0.0f, 1.0f, 1.5f, 2.5f);
				pHuman->change_act(Human::ACT_STAND, t);
			}
			break;
		case Human::ACT_TURN_L:
		case Human::ACT_TURN_R:
			if (pHuman->mActionTimer.check_time_out()) {
				float t = nxCalc::fit(Scene::glb_rng_f01(), 0.0f, 1.0f, 0.5f, 1.0f);
				pHuman->change_act(Human::ACT_STAND, t);
			}
			break;
	}
}

ScnObj* add(const Human::Descr descr, const cxQuat& quat, const cxVec& pos) {
	ScnObj* pObj = HumanSys::add_human(descr, roam_ctrl);
	pObj->set_world_quat_pos(nxQuat::from_degrees(0.0f, 0.0f, 0.0f), pos);
	return pObj;
}

}