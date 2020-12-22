import os
if __name__ == '__main__':
    print "test file io"
    f = open('/dev/myscull0', 'rw+')
    f.seek(0, os.SEEK_END)
    f.write("gggg")
    #f.seek(0, 0)
    #print f.read()  # read() will call llseek internally
    f.close()
