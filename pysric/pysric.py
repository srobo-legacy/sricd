from ctypes import *

class SricDevice(Structure):
	_fields_ = [("address", c_int), ("type", c_int)]

class SricFrame(Structure):
	_fields_ = [("address", c_int), ("note", c_int),
			("payload_length", c_int),
			("payload", c_byte * 64)]

libsric = cdll.LoadLibrary("../libsric/libsric.so")
libsric.sric_init.argtypes = []
libsric.sric_init.restype = c_void_p
libsric.sric_quit.argtypes = [c_void_p]
libsric.sric_quit.restype = None
libsric.sric_enumerate_devices.argtypes = [c_void_p, c_void_p]
libsric.sric_enumerate_devices.restype = POINTER(SricDevice)
libsric.sric_tx.argtypes = [c_void_p, POINTER(SricFrame)]
libsric.sric_tx.restype = c_int
libsric.sric_poll_rx.argtypes = [c_void_p, POINTER(SricFrame), c_int]
libsric.sric_poll_rx.restype = c_int

class PySric(object):
	def __init__(self):
		self.sric_ctx = libsric.sric_init()

		self.devices = []
		tmpdev = None
		while True:
			tmpdev = libsric.sric_enumerate_devices(sric_ctx, tmpdev)
			if cast(tmpdev, c_void_p).value == None:
				break
			# Hello, a device
			self.devices.append(tmpdev[0])
	
	def __del__(self):
		libsric.sric_quit(self.sric_ctx)

	def txrx(self, txframe):
		rxframe = SricFrame()
		txframe.note = -1	# As I understand it, txed frames
					# have no notification id
		ret = libsric.sric_tx(self.sric_ctx, txframe)
		if ret != 0:
			print "Error " + str(ret) + " txing sric frame"
			return None

		ret = libsric.sric_poll_rx(self.sric_ctx, rxframe, -1)
		if ret != 0:
			print "Error " + str(ret) + " rxing sric frame"

		return rxframe
