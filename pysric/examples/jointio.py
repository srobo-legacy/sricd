#!/usr/bin/python
# A program for testing the functionality of a jointio board
import sys, os, time, atexit, select
sys.path.append( os.path.join( os.path.dirname(__file__), "../" ) )
import pysric

def setpower(s):
    jio.txrx( [4, s] )

def setoutput(d):
    jio.txrx( [0, d] )

def getinputs():
    data = jio.txrx( [2] )
    inputs = [0]*8
    for i in range(8):
        inputs[i] = data[i*2]
        inputs[i] |= data[(i*2)+1] << 8
    return inputs

def dispinputs():
    while 1:
        inputs = getinputs()
	sys.stdout.write(" "*100 + "\r")
        for inp in inputs:
            sys.stdout.write("%i\t" % inp)
        sys.stdout.write("\r")
	sys.stdout.flush()
        if select.select([sys.stdin], [], [], 0) == ([sys.stdin], [], []):
            break
        time.sleep(0.2)

def setoutputs():
    i = 0
    while 1:
        op = 1<<i
        setoutput(op)
        sys.stdout.write(str(i))
        sys.stdin.read(1)
        i = i+1
        if i == 8:
            i = 0

ctl = pysric.PySric()
jio = ctl.devices[ pysric.SRIC_CLASS_JOINTIO ][0]

atexit.register(setpower, 0)

# Enable 3.3V reg
setpower(1)
print "3.3V output enabled"

raw_input("Press enter to begin testing inputs")

print "Inputs:"
dispinputs()

print "\nSet outputs:"
setoutputs()
