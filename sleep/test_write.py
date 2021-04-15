import pdb
import time
from datetime import datetime

def myprint(message):
    print ("test_write %s: %s" %(datetime.now(), message))

myprint("trying to write abcd")
f = open("/dev/myscull0", "w")
f.write("abcd")
f.close()
myprint("write complete")
myprint("sleep 2 seconds")
time.sleep(2)
myprint("trying to write 1234")
f = open("/dev/myscull0", "w")
f.write("1234")
f.close()
myprint("write complete")
myprint("trying to write xxxx")
f = open("/dev/myscull0", "w")
f.write("xxxx")
f.close()
myprint("write complete")
