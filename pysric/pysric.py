from ctypes import *

libsric = cdll.LoadLibrary("../libsric/libsric.so")
libsric.sric_init.argtypes = []
libsric.sric_init.restype = c_void_p
libsric.sric_quit.argtypes = [c_void_p]
libsric.sric_quit.restype = None

class pysric(object):
	sric_ctx = libsric.sric_init()
	
	def __del__(self):
		libsric.sric_quit(self.sric_ctx)
