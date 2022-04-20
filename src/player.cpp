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

static void Manana_exec_ctrl(Human* pHuman) {
	if (!pHuman) return;
	switch (pHuman->mAction) {
	case Human::ACT_STAND:
		if (InputCtrl::triggered(InputCtrl::UP)) {
			pHuman->change_act(Human::ACT_WALK, 2.0f, 20);
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
			if (InputCtrl::now_active(InputCtrl::LEFT)) {
				pHuman->add_deg_y(0.5f);
			} else if (InputCtrl::now_active(InputCtrl::RIGHT)) {
				pHuman->add_deg_y(-0.5f);
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

ScnObj* init() {
	Human::Descr descr;
	descr.reset();
	descr.type = Human::Type::FEMALE;
	descr.bodyVariation = 5;
	descr.scale = 1.0f;
	descr.pName = "Manana";
	ScnObj* pPlr = HumanSys::add_human(descr, Manana_exec_ctrl);
	return pPlr;
}

}