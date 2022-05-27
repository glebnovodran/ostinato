/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

#include <crosscore.hpp>
#include <scene.hpp>
#include <smprig.hpp>

#include "input.hpp"
#include "timectrl.hpp"
#include "human.hpp"
#include "player.hpp"


namespace Player {

bool check_run_mode() {
	return InputCtrl::now_active(InputCtrl::L2) || InputCtrl::now_active(InputCtrl::ButtonB);
}

void traveller_exec_ctrl(Human* pHuman) {
	if (!pHuman) return;
	switch (pHuman->mAction) {
		case Human::ACT_STAND:
			if (InputCtrl::triggered(InputCtrl::UP)) {
				if (check_run_mode()) {
					pHuman->change_act(Human::ACT_RUN, 2.0f, 12);
				} else {
					pHuman->change_act(Human::ACT_WALK, 2.0f, 20);
				}
			} else if (InputCtrl::triggered(InputCtrl::DOWN)) {
				pHuman->change_act(Human::ACT_RETREAT, 0.5f, 20);
			} else if (InputCtrl::now_active(InputCtrl::LEFT)) {
				pHuman->change_act(Human::ACT_TURN_L, 0.5f, 30);
			} else if (InputCtrl::now_active(InputCtrl::RIGHT)) {
				pHuman->change_act(Human::ACT_TURN_R, 0.5f, 30);
			}
			break;
		case Human::ACT_WALK:
			if (InputCtrl::now_active(InputCtrl::UP)) {
				if (check_run_mode()) {
					int startFrame = pHuman->find_nearest_mot_frame(pHuman->mMotLib.pRun, "j_Ankle_L");
					pHuman->change_act(Human::ACT_RUN, 2.0f, 8, startFrame);
				} else {
					if (InputCtrl::now_active(InputCtrl::LEFT)) {
						pHuman->add_deg_y(0.5f);
					} else if (InputCtrl::now_active(InputCtrl::RIGHT)) {
						pHuman->add_deg_y(-0.5f);
					}
				}
			} else if (InputCtrl::triggered(InputCtrl::DOWN)) {
				pHuman->change_act(Human::ACT_RETREAT, 0.5f, 20);
			} else {
				pHuman->change_act(Human::ACT_STAND, 0.5f, 20);
			}
			break;
		case Human::ACT_RETREAT:
			if (InputCtrl::now_active(InputCtrl::DOWN)) {
				if (InputCtrl::now_active(InputCtrl::LEFT)) {
					pHuman->add_deg_y(0.5f);
				} else if (InputCtrl::now_active(InputCtrl::RIGHT)) {
					pHuman->add_deg_y(-0.5f);
				}
			} else {
				pHuman->change_act(Human::ACT_STAND, 0.5f, 20);
			}
			break;
		case Human::ACT_RUN:
			if (InputCtrl::now_active(InputCtrl::UP)) {
				if (check_run_mode()) {
					if (InputCtrl::now_active(InputCtrl::LEFT)) {
						pHuman->add_deg_y(1.0f);
					} else if (InputCtrl::now_active(InputCtrl::RIGHT)) {
						pHuman->add_deg_y(-1.0f);
					}
				} else {
					int startFrame = pHuman->find_nearest_mot_frame(pHuman->mMotLib.pWalk, "j_Ankle_L");
					pHuman->change_act(Human::ACT_WALK, 2.0f, 12, startFrame);
				}
			} else {
				pHuman->change_act(Human::ACT_STAND, 0.5f, 14);
			}
			break;
		case Human::ACT_TURN_L:
			if (InputCtrl::now_active(InputCtrl::LEFT)) {
				if (InputCtrl::triggered(InputCtrl::UP)) {
					pHuman->change_act(Human::ACT_WALK, 0.5f, 20);
				}
			} else {
				pHuman->change_act(Human::ACT_STAND, 0.5f, 20);
			}
			break;
		case Human::ACT_TURN_R:
			if (InputCtrl::now_active(InputCtrl::RIGHT)) {
				if (InputCtrl::triggered(InputCtrl::UP)) {
					pHuman->change_act(Human::ACT_WALK, 0.5f, 20);
				}
			} else {
				pHuman->change_act(Human::ACT_STAND, 0.5f, 20);
			}
			break;
		default:
			break;
	}
}

void wanderer_exec_ctrl(Human* pHuman) {
	if (!pHuman) return;
		switch (pHuman->mAction) {
			case Human::ACT_STAND:
				break;
			default:
				break;
	}
}

void wanderer_bat_pre_draw(ScnObj* pObj, const int ibat) {
	Scene::push_ctx();
	Scene::scl_hemi_lower(2.0f);
}

static void wanderer_bat_post_draw(ScnObj* pObj, const int ibat) {
	Scene::pop_ctx();
}


ScnObj* init() {
	Human::Descr descr;
	descr.reset();
	descr.type = Human::Type::TRAVELLER;
	descr.personId = 0;
	descr.scale = 1.0f;
	ScnObj* pPlr = HumanSys::add_human(descr, traveller_exec_ctrl);
	if (pPlr) {
		const char* pOccp = HumanSys::get_occupation(pPlr->mpName);
		nxCore::dbg_msg("\n  Enter %s The %s\n", pPlr->mpName != nullptr ? pPlr->mpName: "Animous",  pOccp != nullptr ? pOccp : "Trouble maker");
	}

	descr.reset();
	descr.type = Human::Type::WANDERER;
	descr.personId = 0;
	descr.scale = 1.0f;

	ScnObj* pWanderer = HumanSys::add_human(descr, wanderer_exec_ctrl);
	if (pWanderer) {
		pWanderer->mBatchPreDrawFunc = wanderer_bat_pre_draw;
		pWanderer->mBatchPostDrawFunc = wanderer_bat_post_draw;
		const char* pOccp = HumanSys::get_occupation(pWanderer->mpName);
		nxCore::dbg_msg("\n  Enter %s The %s\n", pWanderer->mpName != nullptr ? pWanderer->mpName: "Anonimous",  pOccp != nullptr ? pOccp : "Gorestudent");
	}

	return pPlr;
}

}