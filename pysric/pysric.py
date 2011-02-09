import os
from ctypes import *

# Magic device addresses
SRIC_MASTER_DEVICE = 0
SRIC_BROADCAST = 254
SRIC_NO_DEVICE = 255

# Device classes
SRIC_CLASS_POWER = 1
SRIC_CLASS_MOTOR = 2
SRIC_CLASS_JOINTIO = 3
SRIC_CLASS_SERVO = 4
SRIC_CLASS_PCSRIC = 5

class SricDevice(Structure):
    _fields_ = [("address", c_int), ("type", c_int)]

    def __repr__(self):
        return "SricDevice( address=%i, type=%i )" % (self.address, self.type)

class SricFrame(Structure):
    _fields_ = [("address", c_int), ("note", c_int),
            ("payload_length", c_int),
            ("payload", c_byte * 64)]

class PySric(object):
    def __init__(self):
        self._load_lib()
        self.sric_ctx = self.libsric.sric_init()

        # Indexes are device classes
        self.devices = {}
        tmpdev = None
        while True:
            tmpdev = self.libsric.sric_enumerate_devices(self.sric_ctx, tmpdev)
            if cast(tmpdev, c_void_p).value == None:
                break

            dev = tmpdev[0]
            if dev.type not in self.devices:
                self.devices[dev.type] = []

            self.devices[dev.type].append(dev)

    def _load_lib(self):
        "Load the library and set up how to use it"

        libsric = None
        # Look in our directory first, then the specified search path
        # Ideally, this would ask the dynamic linker where to find it
        # (and allow override with an environment variable)
        for d in [ os.getenv( "PYSRIC_LIBDIR", "" ),
                   os.path.dirname( __file__ ),
                   "./" ]:
            if d == "":
                continue

            p = os.path.join( d, "libsric.so" )
            if os.path.exists(p):
                libsric = cdll.LoadLibrary(p)
                break

        if libsric == None:
            raise Exception( "pysric: libsric.so not found" )

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

        self.libsric = libsric

    def __del__(self):
        self.libsric.sric_quit(self.sric_ctx)

    def txrx(self, txframe):
        rxframe = SricFrame()
	# As I understand it, txed frames
	# have no notification id
        txframe.note = -1

        ret = self.libsric.sric_tx(self.sric_ctx, txframe)
        if ret != 0:
            print "Error " + str(ret) + " txing sric frame"
            return None

        ret = self.libsric.sric_poll_rx(self.sric_ctx, rxframe, -1)
        if ret != 0:
            print "Error " + str(ret) + " rxing sric frame"

        return rxframe
