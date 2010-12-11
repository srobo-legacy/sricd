#!/usr/bin/python

# A short example that sets motor power for device 3 to 0x20

import sys
sys.path.append( os.path.join( os.path.dirname(__file__), "../" ) )
import pysric

ctl = pysric.PySric()

txframe = pysric.SricFrame()
txframe.address = 3
txframe.note = -1
txframe.payload_length = 3
txframe.payload[0] = 0
txframe.payload[1] = 32
txframe.payload[2] = 0
rxframe = ctl.txrx(txframe)

print "bees:", rxframe.address, rxframe.payload_length

