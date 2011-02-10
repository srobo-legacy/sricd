import os
from ctypes import *

# Magic device addresses
SRIC_MASTER_DEVICE = 0
SRIC_BROADCAST = -2
SRIC_NO_DEVICE = -1

# Device classes
SRIC_CLASS_POWER = 1
SRIC_CLASS_MOTOR = 2
SRIC_CLASS_JOINTIO = 3
SRIC_CLASS_SERVO = 4
SRIC_CLASS_PCSRIC = 5

sric_class_strings = { SRIC_CLASS_POWER: "POWER",
                       SRIC_CLASS_MOTOR: "MOTOR",
                       SRIC_CLASS_JOINTIO: "JOINTIO",
                       SRIC_CLASS_SERVO: "SERVO",
                       SRIC_CLASS_PCSRIC: "PCSRIC",
                       SRIC_MASTER_DEVICE: "MASTER_DEVICE",
                       SRIC_BROADCAST: "BROADCAST",
                       SRIC_NO_DEVICE: "NO_DEVICE" }
class SricDevice(Structure):
    _fields_ = [("address", c_int), ("type", c_int)]

    def __repr__(self):
        if self.type in sric_class_strings:
            t = sric_class_strings[self.type]
        else:
            t = str(self.type)

        return "SricDevice( address=%i, type=%s )" % (self.address, t)

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

        # sric_context sric_init( void )
        libsric.sric_init.argtypes = []
        libsric.sric_init.restype = c_void_p

        # void sric_quit( sric_context ctx )
        libsric.sric_quit.argtypes = [c_void_p]
        libsric.sric_quit.restype = None

        # const sric_device* sric_enumerate_devices( sric_context ctx,
        #                                            const sric_device* device )
        libsric.sric_enumerate_devices.argtypes = [c_void_p, c_void_p]
        libsric.sric_enumerate_devices.restype = POINTER(SricDevice)

        # int sric_tx( sric_context ctx, const sric_frame* frame )
        libsric.sric_tx.argtypes = [c_void_p, POINTER(SricFrame)]
        libsric.sric_tx.restype = c_int

        # int sric_poll_rx( sric_context ctx,
        #                   sric_frame* frame,
        #                   int timeout )
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
