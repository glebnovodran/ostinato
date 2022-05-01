import math
import time
import threading

def cam_info(dbg = None):
	view =  gdb.parse_and_eval("s_view")
	if view:
		pos = view["pos"]
		tgt = view["tgt"]
		frm = gdb.parse_and_eval("OGLSys::get_frame_count()")
		dbg_out("- Cam Info @ frame <%d>" % frm, dbg)
		dbg_out(" cam @ <%.2f, %.2f, %.2f>" % (pos["x"], pos["y"], pos["z"]), dbg)
		dbg_out(" look @ <%.2f, %.2f, %.2f>" % (tgt["x"], tgt["y"], tgt["z"]), dbg)

class CamEvent:
	def __init__(self): pass
	def __call__(self): cam_info()

def cam_event():
	gdb.post_event(CamEvent())

class CamThr(threading.Thread):
	def run(self):
		while True:
			cam_event()
			time.sleep(5)
