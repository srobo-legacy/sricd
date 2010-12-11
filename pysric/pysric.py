from ctypes import *

class sric_device(Structure):
	_fields_ = [("address", c_int), ("type", c_int)]

class sric_frame(Structure):
	_fields_ = [("address", c_int), ("note", c_int),
			("payload_length", c_int),
			("payload", c_byte * 64)]

libsric = cdll.LoadLibrary("../libsric/libsric.so")
libsric.sric_init.argtypes = []
libsric.sric_init.restype = c_void_p
libsric.sric_quit.argtypes = [c_void_p]
libsric.sric_quit.restype = None
libsric.sric_enumerate_devices.argtypes = [c_void_p, c_void_p]
libsric.sric_enumerate_devices.restype = POINTER(sric_device)
libsric.sric_tx.argtypes = [c_void_p, POINTER(sric_frame)]
libsric.sric_tx.restype = c_int
libsric.sric_poll_rx.argtypes = [c_void_p, POINTER(sric_frame), c_int]
libsric.sric_poll_rx.restype = c_int

class pysric(object):
	sric_ctx = libsric.sric_init()

	devices = []
	tmpdev = None
	while True:
		tmpdev = libsric.sric_enumerate_devices(sric_ctx, tmpdev)
		if cast(tmpdev, c_void_p).value == None:
			break
		# Hello, a device
		devices.append(tmpdev[0])
	
	def __del__(self):
		libsric.sric_quit(self.sric_ctx)
