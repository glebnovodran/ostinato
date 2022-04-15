namespace KeyCtrl {

enum {
	UP = 0,
	DOWN,
	LEFT,
	RIGHT,
	LCTRL,
	LSHIFT,
	TAB,
	BACK,
	ENTER,
	SPACE,
	NUM_KEYS
};

void init();
void update();

bool ck_now(int id);
bool ck_old(int id);
bool ck_trg(int id);
bool ck_chg(int id);

}