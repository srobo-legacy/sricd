#!/usr/bin/python
# A program for testing the functionality of a motor board
import sys, os, time
sys.path.append( os.path.join( os.path.dirname(__file__), "../" ) )
import pysric

def setspeed(s, b):
    motor.txrx( [0, s, b] )

ctl = pysric.PySric()
motor = ctl.devices[ pysric.SRIC_CLASS_MOTOR ][0]

setspeed(100, 0)
time.sleep(2)
setspeed(-100, 0)
time.sleep(2)
setspeed(0, 1)
time.sleep(2)
setspeed(50, 0)
time.sleep(2)
setspeed(-50, 0)
time.sleep(2)
setspeed(0, 0)
