/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Glib Novodran <novodran@gmail.com> */

struct Human {

	typedef void (*CtrlFunc)(Human*);
	enum State {
		INIT = 0,
		ACTIVE
	};

	enum Kind {
		FEMALE = 0,
		MALE,
		MAX_KIND
	};
	enum Type {
		CITIZEN_FEMALE = 0,
		CITIZEN_MALE,
		TRAVELLER,
		WANDERER,
		MAX
	};

	struct Descr {
		const char* pName;
		Type type;
		int personId;
		int mtlVariation;
		float scale;

		void reset() {
			pName = nullptr;
			type = Type::CITIZEN_FEMALE;
			personId = 0;
			mtlVariation = 0;
			scale = 1.0f;
		}
	};

	struct Rig : public SmpRig {
		Human* mpHuman;

		void init(Human* pHuman);
		void exec();
	};

	struct MotLib {
		sxMotionData* pStand;
		sxMotionData* pTurnL;
		sxMotionData* pTurnR;
		sxMotionData* pWalk;
		sxMotionData* pRetreat;
		sxMotionData* pRun;

		void init(const Pkg* pPkg, const Pkg* pBasePkg = nullptr);
	};

	enum Action {
		ACT_STAND,
		ACT_TURN_L,
		ACT_TURN_R,
		ACT_WALK,
		ACT_RETREAT,
		ACT_RUN
	};
	struct ActionTimer {
		double mStartTime;
		double mDuration;
		void fix_up_time() { mStartTime = TimeCtrl::get_current_time(); }
		double get_time() const { return TimeCtrl::get_current_time() - mStartTime; }
		bool check_time_out() const { return get_time() > mDuration; }
		void set_duration_seconds(const double sec) {
			mDuration = nxCalc::max(sec, 0.0) * 1000.0;
		}
		void start(const double sec) {
			fix_up_time();
			set_duration_seconds(sec);
		}
	};

	Rig mRig;
	MotLib mMotLib;
	ScnObj* mpObj;
	CtrlFunc mCtrlFunc;

	ActionTimer mActionTimer;
	ActionTimer mAuxTimer;
	Action mAction;

	State mState;

	int32_t mObjAdjCount;
	int32_t mObjTouchCount;
	int32_t mWallTouchCount;
	double mObjTouchStartTime;
	double mObjTouchDuration;
	double mWallTouchStartTime;
	double mWallTouchDuration;

	Type mType;

	void change_act(const Action newAct, const double durationSecs, const int blendCnt = 15, const int startFrame = 0);
	void add_deg_y(const float dy);

	State get_state() const { return mState; }
	void change_state(State state) { mState = state; }

	void set_motion(sxMotionData* pMot = nullptr) {
		if (mpObj) {
			mpObj->mpMoveMot = pMot ? pMot : mMotLib.pStand;
		}
	}

	void ctrl();

	void select_motion() {
		sxMotionData* pMot = nullptr;
		switch (mAction) {
			case ACT_STAND: pMot = mMotLib.pStand; break;
			case ACT_TURN_L: pMot = mMotLib.pTurnL; break;
			case ACT_TURN_R: pMot = mMotLib.pTurnR; break;
			case ACT_WALK: pMot = mMotLib.pWalk; break;
			case ACT_RETREAT: pMot = mMotLib.pRetreat; break;
			case ACT_RUN: pMot = mMotLib.pRun; break;
			default: break;
		}
		set_motion(pMot);
	}
	uint32_t find_nearest_mot_frame(const sxMotionData* pMtd, const char* pNodeName) const;

	double get_obj_touch_duration_secs() const { return mObjTouchDuration / 1000.0; }
	double get_wall_touch_duration_secs() const { return mWallTouchDuration / 1000.0; }
	void reset_wall_touch() {
		mWallTouchCount = 0;
		mWallTouchDuration = 0.0;
	}
	void obj_adj();
	void wall_adj();
	void ground_adj();
	void exec_collision();

	const char* get_name() const { return mpObj ? mpObj->mpName : nullptr; }
	void mark() {
		if (mpObj) {
			mpObj->set_base_color_scl(1.5f, 0.25f, 0.25f);
		}
	}
	void unmark() const {
		if (mpObj) {
			mpObj->set_base_color_scl(1.0f);
		}
	}
};

namespace HumanSys {
	void init();
	void reset();

	Human* find(const char* pName);
	bool obj_is_human(ScnObj* pObj);
	Human* as_human(ScnObj* pobj);
	ScnObj* add_human(const Human::Descr& descr, Human::CtrlFunc ctrl = nullptr);
	void set_collision(sxCollisionData* pCol);
	sxCollisionData* get_collision();

	void mark(const char* pName) {
		Human* pHuman = find(pName);
		if (pHuman) {
			pHuman->mark();
		}
	}
	void unmark(const char* pName) {
		Human* pHuman = find(pName);
		if (pHuman) {
			pHuman->unmark();
		}
	}
}