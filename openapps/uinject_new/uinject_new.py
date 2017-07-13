import socket
import struct
from datetime import datetime
import MySQLdb
import thread
import time
import os

secPerPkt = 45

# open socket
socket_handler = socket.socket(socket.AF_INET6,socket.SOCK_DGRAM)
socket_handler.bind(('',2000))

neigRankDic = {}

serialSnDic = {}

calRcvRate = {}

enableAck = 1

progStartTime = datetime.now()

def moteRecv_update(macAddr):
  if macAddr in calRcvRate:
    calRcvRate[macAddr] = calRcvRate[macAddr] + 1
  else:
    calRcvRate.update({macAddr:1})
  return 1

def dispMoteRate():
  #estimate how many pkt should be by time
  t = datetime.now()
  currTimeStr = t.strftime("%Y-%m-%d %H:%M:%S")
  globalTimeStr = progStartTime.strftime("%Y-%m-%d %H:%M:%S")

  elapseT= t - progStartTime
  estPkt = elapseT / secPerPkt
  estPkt_sec = estPkt.total_seconds()
  total = int(estPkt_sec) + 1.0

  for macAddr in calRcvRate:
    rate = calRcvRate[macAddr] / total
    #rate = rate * 100
    print 'Time:{0}, addr={1}, recvCnt={2}, total={3}, rate={4:.1%}'.format(currTimeStr, macAddr, calRcvRate[macAddr], total, rate)

def threadTimer(string, sleeptime, lock, *args):
  while(True):
    lock.acquire()
    print 'enter {0} timerThread, clear table'.format(string)
    db_tmp = MySQLdb.connect("localhost","root","sakimaru","ITRI_OpenWSN" )
    cursor_tmp = db_tmp.cursor()
    cursor_tmp("SELECT * FROM itri_topology_tmp")
    row_tmp = cursor_tmp.fetchone()
    while row_tmp is not NONE:
       
       row_tmp = cursor_tmp.fetchone()

    time.sleep(2)
    lock.release()
    time.sleep(sleeptime)

def neigRank_update(macAddr, rank):
  neigRankDic.update({macAddr:rank})
  return 1

def pktSn_update(macAddr,snNum):
  serialSnDic.update({macAddr:snNum})
  return 1

def pktSn_check(macAddr,snNum):
  try:
    if serialSnDic[macAddr] == snNum:
      return 1
    else:
      return 0
  except:
    return 0

def send_data_to_mote(src_Addr, src_port, dst_Addr, dst_port, sdata):
   s_handler = socket.socket(socket.AF_INET6,socket.SOCK_DGRAM)
   s_handler.settimeout(5)
   s_handler.bind((src_Addr,src_port))
   request = "".join(map(chr, sdata))
   s_handler.sendto(request,(dst_Addr, dst_port))
   s_handler.close()


def prune_table(srcTableName,dstTableName):
   db = MySQLdb.connect("localhost","root","sakimaru","ITRI_OpenWSN" )
   cursor = db.cursor()
   cursor.execute("")
   row = cursor.fetchone()
   while row is not None:
      row = cursor.fetchone()

def age_mechanism():
   #sort by date
   db_seg = MySQLdb.connect("localhost","root","sakimaru","ITRI_OpenWSN" )
   db_tmp = MySQLdb.connect("localhost","root","sakimaru","ITRI_OpenWSN" )
   cursor_tmp = db_age.cursor()
   cursor_seg = db_seg.cursor()
   
   #get entry
   cursor_tmp.execute("SELECT * FROM itri_topology_tmp")
   cursor_seg.execute("SELECT * FROM itri_topology_seg")
   row = cursor_tmp.fetchone()
   while row is not None:
      #check ss_table for duplicate mac addr
      row[1]
      row = cursor_tmp.fetchone()


   #set age_cnt=0, insert into Result table

   #truncate tmp table

   #search for entry GTR MAX in table Resutl

   #truncate sstable, copy result table into it
   
   cursor_age.execute(sql)
   db_age.commit()
   db_age.close()


#lock = thread.allocate_lock()
#thread.start_new_thread(threadTimer, ("T1", 10, lock))

db = MySQLdb.connect("localhost","root","sakimaru","ITRI_OpenWSN" )
cursor = db.cursor()
sql= "CREATE TABLE IF NOT EXISTS itri_topology_current_neighbors ( `devAddr` VARCHAR(45), `SN` INT(11), `mode` VARCHAR(45), `PDR` FLOAT, `rank` INT(11) , `parentAddr` VARCHAR(45), `neighborNum` INT(11), datetime DATETIME, `n1` VARCHAR(45), `n2` VARCHAR(45), `n3` VARCHAR(45),`n4` VARCHAR(45),`n5` VARCHAR(45),`n6` VARCHAR(45),`n7` VARCHAR(45),`n8` VARCHAR(45),`n9` VARCHAR(45),`n10` VARCHAR(45), `rssi1` INT(11),`rssi2` INT(11),`rssi3` INT(11),`rssi4` INT(11),`rssi5` INT(11),`rssi6` INT(11),`rssi7` INT(11),`rssi8` INT(11),`rssi9` INT(11),`rssi10` INT(11), PRIMARY KEY(`devAddr`))"

try:
    cursor.execute(sql)
    db.commit()
except:
    db.rollback()

db.close()

while True:
    print 'normal process here below !!' 
    # wait for a request
    #request,dist_addr = socket_handler.recvfrom(1024)
    data,dist_addr = socket_handler.recvfrom(1024)
 
    hisAddress     = dist_addr[0]
    hisPort        = dist_addr[1]
    counter = 0
    tplg_mode = 0

#    counter        = struct.unpack('<h',data)[0]
    if len(data) > 51:
        print 'illegal data length (%d Bytes), but we continue' % len(data)  
        os.system("touch /home/ubuntu/openwsn_timestamp.byTouch")
        continue
    else :
        print 'data length is {0}'.format(len(data))
        neigList = []
        rssiList = []
        Saddr = 0xffff
        Rssi = -99
        newFileByteArray = bytearray( data )
        tplg_Code = newFileByteArray[0]
        needAck = (tplg_Code & 0x20)
        tplg_mode = tplg_Code
        PDR = 0.0
        PDR += newFileByteArray[1]
        PDR /= 255.0
        tplg_parent = newFileByteArray[2]*256 + newFileByteArray[3]
        tplg_rank = newFileByteArray[4]*256 + newFileByteArray[5]
        tplg_numNeig = newFileByteArray[6]


        for c in range(0,10):
          if c < tplg_numNeig:
            Saddr= newFileByteArray[c*3+7]
            Saddr *=256
            Saddr += newFileByteArray[c*3+8]
            Rssi = newFileByteArray[c*3+9]
            Rssi -= 255
            neigList.append(Saddr)
            rssiList.append(Rssi)
            print 'RSSI = {0}'.format(Rssi)
          else:
            Saddr= 0
            Rssi= 0
            neigList.append(Saddr)
            rssiList.append(Rssi)
           
        print 'd1={0},d2={1:.2f},d3={2}, parent={3}, neigs='.format(tplg_mode,PDR,tplg_numNeig, hex(tplg_parent)) + ' '.join( hex(c) for c in neigList[0:tplg_numNeig])
        counter = newFileByteArray[len(data)-1]*256 + newFileByteArray[len(data)-2] 

        if enableAck and needAck > 0:
           print 'get Ack req from mote={0}, and Host enable Ack'.format(hisAddress)
           send_data_to_mote('',2001,hisAddress,hisPort,[10,newFileByteArray[len(data)-2],newFileByteArray[len(data)-1]])
        elif needAck > 0:
           print 'get Ack req from mote={0}, but Host disable Ack'.format(hisAddress)

        if pktSn_check(hisAddress, counter):
           print 'we receive duplicate udp data sn={0}'.format(counter)
           continue
        else:
           pktSn_update(hisAddress, counter)
        #print 'd1={0},d2={1},d3={2}, n1={3:4x}, n2={4:4x}'.format(d1,d2,d3,neigList[0],neigList[1])

        neigRank_update(hisAddress, tplg_rank)
        #print neigRankDic

        hisAddr_split = hisAddress.split(':')
        tplg_src_addr = ''
        for i in hisAddr_split[2:]:
           tplg_src_addr += '%04x' % int(i,16)
        print 'addr=%s' % tplg_src_addr
        t = datetime.now()
        currtime = t.strftime("%Y-%m-%d %H:%M:%S")

        db = MySQLdb.connect("localhost","root","sakimaru","ITRI_OpenWSN" )
        cursor = db.cursor()

    #mySql cmd
        sql = "INSERT INTO itri_topology_neighbors(mode, neighborNum, devAddr,PDR,\
        parentAddr, datetime, SN, rank,\
        n1,n2,n3,n4,n5,\
        n6,n7,n8,n9,n10,\
        rssi1,rssi2,rssi3,rssi4,rssi5,\
        rssi6,rssi7,rssi8,rssi9,rssi10) \
        VALUES ('%s', '%d', '%s', '%.2f',\
            '%04x', '%s', '%d', '%d',\
            '%04x','%04x','%04x','%04x','%04x',\
            '%04x','%04x','%04x','%04x','%04x',\
            '%d','%d','%d','%d','%d',\
            '%d','%d','%d','%d','%d')" \
            %(hex(tplg_mode), tplg_numNeig, tplg_src_addr, PDR,\
            tplg_parent, currtime, counter, tplg_rank,\
            neigList[0],neigList[1],neigList[2],neigList[3],neigList[4],\
            neigList[5],neigList[6],neigList[7],neigList[8],neigList[9],\
            rssiList[0],rssiList[1],rssiList[2],rssiList[3],rssiList[4],\
            rssiList[5],rssiList[6],rssiList[7],rssiList[8],rssiList[9])

        sql_rp = "REPLACE INTO itri_topology_current_neighbors(mode, neighborNum, devAddr,PDR,\
        parentAddr, datetime, SN, rank,\
        n1,n2,n3,n4,n5,\
        n6,n7,n8,n9,n10,\
        rssi1,rssi2,rssi3,rssi4,rssi5,\
        rssi6,rssi7,rssi8,rssi9,rssi10) \
        VALUES ('%s', '%d', '%s', '%.2f',\
            '%04x', '%s', '%d', '%d',\
            '%04x','%04x','%04x','%04x','%04x',\
            '%04x','%04x','%04x','%04x','%04x',\
            '%d','%d','%d','%d','%d',\
            '%d','%d','%d','%d','%d')" \
            %(hex(tplg_mode), tplg_numNeig, tplg_src_addr, PDR,\
            tplg_parent, currtime, counter, tplg_rank,\
            neigList[0],neigList[1],neigList[2],neigList[3],neigList[4],\
            neigList[5],neigList[6],neigList[7],neigList[8],neigList[9],\
            rssiList[0],rssiList[1],rssiList[2],rssiList[3],rssiList[4],\
            rssiList[5],rssiList[6],rssiList[7],rssiList[8],rssiList[9])

        try:
            # Execute the SQL command
            cursor.execute(sql)
            cursor.execute(sql_rp)
            # Commit your changes in the database
            db.commit()

            #lock.acquire()
            # Execute the SQL command
            #cursor.execute(sql_to_tmp)
            # Commit your changes in the database
            #db.commit()
            #lock.release()
        except:
            # Rollback in case there is any error
            db.rollback()
    
        # disconnect from server
        db.close()

        print 'done insert data into table itri_topology_neighbors'

        moteRecv_update(tplg_src_addr)
        dispMoteRate()

        # os.system("touch /home/ubuntu/openwsn_timestamp.byTouch")

        continue

        # print 'data size = %d !!' % len(data)
        # print 'append to topology.bin'
        # f = open('topology.bin', 'ab')
        # newFileByteArray = bytearray( data )
        # f.write( newFileByteArray )
        # f.close()

    #get current time
    t = datetime.now()
    currtime = t.strftime("%Y-%m-%d %H:%M:%S")

    db = MySQLdb.connect("localhost","root","sakimaru","ITRI_OpenWSN" )
    cursor = db.cursor()

    #mySql cmd
    sql = "INSERT INTO itri_MOEA_topology(mode, numNeighbor, devAddr, rank, power, \
        n1Addr, n1Rank, n1NumTx, n1NumTxA, \
        n2Addr, n2Rank, n2NumTx, n2NumTxA, \
        n3Addr, n3Rank, n3NumTx, n3NumTxA, \
        datetime, sn) \
        VALUES ('%s', '%s', '%s', '%d', '%d', \
                '%s', '%d', '%d', '%d',\
                '%s', '%d', '%d', '%d',\
                '%s', '%d', '%d', '%d',\
            '%s', '%d')" \
        %(hex(tplg_mode), tplg_numNeig, tplg_src_addr, tplg_rank, tplg_power, \
          tplg_n1_addr, tplg_n1_rand, tplg_n1_numTx, tplg_n1_numTxA, \
          tplg_n2_addr, tplg_n2_rand, tplg_n2_numTx, tplg_n2_numTxA, \
          tplg_n3_addr, tplg_n3_rand, tplg_n3_numTx, tplg_n3_numTxA, \
            currtime, counter)
    try:
        # Execute the SQL command
        cursor.execute(sql)
        # Commit your changes in the database
        db.commit()
    except:
        # Rollback in case there is any error
        db.rollback()
    
    # disconnect from server
    db.close()

    print 'received from [{0}]:{1} \
    '.format(hisAddress,hisPort)
