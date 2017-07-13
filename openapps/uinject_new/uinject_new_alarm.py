import time
import socket
import sys
from time import sleep

NUM_TRIES  = 1

#request    = 0x02
myAddress  = '' #means 'all'
myPort     = 2001
hisAddress = 'bbbb::0012:4b00:0615:a53f'
hisPort    = 2000
delays     = []
succ       = 0
fail       = 0
#print "Testing udpEcho..."


if(len(sys.argv) != 3):

    print 'Please use: python uinject_alarm.py [bbbb::0012:4b00:0615:xxxx] [on/off]'
    exit()

inAddr = sys.argv[1]
inputCtrl = sys.argv[2]

# combAddr = 'bbbb::'+inAddr[0:4]+':'+inAddr[4:8]+':'+inAddr[8:12]+':'+inAddr[12:16]
combAddr = 'bbbb::0012:4b00:0615:'+inAddr[0:4]

#print 'combAddr={0}'.format(combAddr)

hisAddress = combAddr

for i in range(NUM_TRIES):
    
    # log
    output         = []
    output        += ['echo {0}'.format(i)]
    output        += ['request [{0}]:{1}->[{2}]:{3}'.format(myAddress,myPort,hisAddress,hisPort)]
#    output        += ['{0} ({1} bytes)'.format(request,len(request))]
    output         = '\n'.join(output)
    #print output
    
    # open socket
    socket_handler = socket.socket(socket.AF_INET6,socket.SOCK_DGRAM)
    socket_handler.settimeout(5)
    socket_handler.bind((myAddress,myPort))
  
    values = bytearray()
    values.append(1)

    if(inputCtrl == 'on'):
        sdata = [40,0,0]
    else:
        sdata = [41,0,0]
 
    request = "".join(map(chr, sdata))
    # send request
    socket_handler.sendto(request,(hisAddress,hisPort))
    sleep(0.5)
    socket_handler.sendto(request,(hisAddress,hisPort))
    sleep(0.5)
    socket_handler.sendto(request,(hisAddress,hisPort))

    print "Send Tripple Alarms to {0}...".format(hisAddress)

    # close socket
    socket_handler.close()
#raw_input("\nPress return to close this window...")
