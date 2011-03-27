#!/usr/bin/python
# A program for testing the functionality of a servo board
import sys, os, time, atexit, select, random
sys.path.append( os.path.join( os.path.dirname(__file__), "../" ) )
import pysric

def setpower(s):
    servo.txrx( [7, s] )

def setangle(s, a):
    servo.txrx( [0, s, 0xff & a, 0xff & (a>>8)] )

def setallangle(a):
    for i in range(8):
        setangle(i, a)

def sweepoutputs():
    while 1:
        for a in range(0, 400, 4):
            sys.stdout.write("%03i\r" % a)
            sys.stdout.flush()
            setallangle(a)
        for a in range(0, 400, 1):
            sys.stdout.write("%03i\r" % a)
            sys.stdout.flush()
            setallangle(a)

ctl = pysric.PySric()
servo = ctl.devices[ pysric.SRIC_CLASS_SERVO ][0]

atexit.register(setpower, 0)

# Enable 5V SMPS
setpower(1)
print "5V rail enabled"

sweepoutputs()
