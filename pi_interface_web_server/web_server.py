#!/usr/bin/python

import socket as s
import sys
import traceback

my_port = 8080
max_backlog = 5
bContinue = True
sBody = "<html><head><title>My Raspberry Pi Server</title></head><body>Hello from my Raspberry Pi</body></html>"

try:
    mysocket = s.socket(s.AF_INET, s.SOCK_STREAM)
    mysocket.bind(("",my_port))
    mysocket.listen(max_backlog)

    while bContinue:
        c, addr = mysocket.accept()
        print "Got a request!"
        sResponse = "HTTP/1.1 200 OK\r\nContent-Length: " + str(len(sBody)) + "\r\nConnection: Closed\r\n\r\n" + sBody
        #print "Sending:"
        #print sResponse
        c.send(sResponse)
        c.close()

except Exception as ex:
    print "Exception = ", ex, " Tracekack:", traceback.format_exc()
    sys.exit(1)

