import socket
import struct
from datetime import datetime
import MySQLdb

CONST = 0.58134
OFFSET_DATASHEET_25C = 827 #// 1422*CONST, from Datasheet
TEMP_COEFF = CONST * 4.2 #// From Datasheet
OFFSET_0C = OFFSET_DATASHEET_25C - (25 * TEMP_COEFF)

# open socket
socket_handler = socket.socket(socket.AF_INET6,socket.SOCK_DGRAM)
socket_handler.bind(('',2424))

while True:
    
    # wait for a request
    #request,dist_addr = socket_handler.recvfrom(1024)
    data,dist_addr = socket_handler.recvfrom(1024)
 
    hisAddress     = dist_addr[0]
    hisPort        = dist_addr[1]

#    counter        = struct.unpack('<h',data)[0]
    counter,data1,data2,data3,data4,data5 = struct.unpack('HHHHHH',data)
    
    print 'len=%d, counter=%x, d1=%x, d2=%x, d3=%d, d4=%x' % (len(data),counter,data1,data2,data3,data4)
    #d1, inner temp
    #d2, exter temp
    #d3, pyra
    #d4, volt
    pure_value = data1
    temp_volt = pure_value * CONST
    i_temp_real = (temp_volt - OFFSET_0C) / TEMP_COEFF
    print 'internal temp = %2.2f' % i_temp_real


    pure_value = data2
    temp_volt = pure_value * 1200 / 2048
    temp_real = (temp_volt - 0.1678) / 12.223
    print 'external temp = %2.2f' % temp_real
    
    pure_value = data3
    if (pure_value > 2047):
        pyra_real = 0
        print 'detect estimated pyra < 0'
    else:
        pyra_volt = pure_value * 1200 / 2048
        pyra_real = pyra_volt * 1000 / 1200
        print 'estimated pyra = %d' % pyra_real    

    pure_value = data4
    i_volt_real = pure_value / 364.5
    print 'raw i_volt = %2.2f' % i_volt_real
   
    hisAddress_split = hisAddress.split(':')
    address = ":".join(hisAddress_split[2:])
    #print 'hisAddress=%s %s %s %s'%(hisAddress_split[2],hisAddress_split[3],hisAddress_split[4],hisAddress_split[5])

    #open db connection 
    db = MySQLdb.connect("localhost","root","sakimaru","ITRI_OpenWSN" )
    cursor = db.cursor()

    #get current time
    t = datetime.now()
    currtime = t.strftime("%Y-%m-%d %H:%M:%S")

    #mySql cmd
    sql = "INSERT INTO itri_MOEA_sensor(serialNumber, mac_addr, \
		ext_temperature, pyranometer, datetime, int_temperature, battery_volt) \
		VALUES ('%d', '%s', '%.2f', '%d', '%s', '%.2f', '%.2f')" %\
		(counter, address, temp_real, pyra_real, currtime, i_temp_real, i_volt_real)
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

    print 'insert DB ok !!'

    print 'received from [{0}]:{1} \
    '.format(hisAddress,hisPort)
