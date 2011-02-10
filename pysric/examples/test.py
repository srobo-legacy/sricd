#!/usr/bin/python
# A short example that sets motor power for device 3 to 0x20
import sys, os
sys.path.append( os.path.join( os.path.dirname(__file__), "../" ) )
import pysric

ctl = pysric.PySric()
motor = ctl.devices[ pysric.SRIC_CLASS_MOTOR ][0]
rxframe = motor.txrx( [0, 32, 0 ] )

print "bees:", rxframe.address, rxframe.payload_length

