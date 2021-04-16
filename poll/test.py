import select
import time

TIMEOUT = 4000

f1 = open("/dev/myscull1")
f2 = open("/dev/myscull2")
f3 = open("/dev/myscull3")

poller = select.poll()
poller.register(f1)
poller.register(f2)
poller.register(f3)

readable = {}

while True:
    events = poller.poll(TIMEOUT)
    print (events)
    for fd, flag in events:
        if flag == select.EPOLLIN:
            print ("polling %d successfully!" %fd)
            readable[fd] = True 
    if len(readable) == 3:
        break
    else:
        time.sleep(2)

f1.close()
f2.close()
f3.close() 

