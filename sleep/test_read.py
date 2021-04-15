import time 
import pdb
from datetime import datetime

def myprint(message):
    print ("test_read %s: %s" %(datetime.now(), message))

myprint("open /dev/myscull0")
f = open("/dev/myscull0")
myprint("trying to read 4 bytes")
buf = f.read(4)
myprint("read result is %s" %buf)
myprint("sleep 4 seconds")
time.sleep(4)
myprint("trying to read 4 bytes")
buf = f.read(4)
myprint("read result is %s" %buf)
myprint("sleep 2 seconds")
time.sleep(2)
myprint("trying to read 4 bytes")
buf = f.read(4)
myprint("read result is %s" %buf)


