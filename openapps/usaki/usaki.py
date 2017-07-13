import socket
import struct
import datetime
import MySQLdb

CONST = 0.58134
OFFSET_DATASHEET_25C = 827 #// 1422*CONST, from Datasheet
TEMP_COEFF = CONST * 4.2 #// From Datasheet
OFFSET_0C = OFFSET_DATASHEET_25C - (25 * TEMP_COEFF)

# open socket
socket_handler = socket.socket(socket.AF_INET6,socket.SOCK_DGRAM)
socket_handler.bind(('',2424))

def convert_adc_to_batt_by_id(in_ID, in_value):
    matrix2 = {'a732':42, 'a720':-24, 'a6f4':0, 'a714':8, 'a636':6, 'a739':26, 'a6ab':10, 'a668':13, 'a6b2':0, 'a6fa':28, 'a6e4':-12}

    parm_global_carib_batt = 0.9925
    parm_digi2volt=3.3/2047.0
    parm_digi2volt *= parm_global_carib_batt
    #parm_ADCvolt2BATTvolt = 1/0.4498
    parm_ADCvolt2BATTvolt = 1/0.5035

    value = matrix2.get(in_ID)

    print 'offset={0} for {1}'.format(value, in_ID)

    if (value == None ):
        offset = 0
    else:
        offset = value

    temp_volt = (in_value + offset)
    tmp_real = temp_volt * parm_digi2volt
    tmp_real2 = tmp_real * parm_ADCvolt2BATTvolt

    print 'external pure Battery ADC value = {0}, estimated Volt={1}V, estimated BattV={2}'.format(in_value, tmp_real, tmp_real2)

    return tmp_real2

def convert_adc_to_temp_by_id(in_ID, in_value):
    matrix = {'a732':42, 'a720':-24, 'a6f4':0, 'a714':8, 'a636':6, 'a739':26, 'a6ab':10, 'a668':13, 'a692':2, 'a6fa':28}

    value = matrix.get(in_ID)

    print 'offset={0} for {1}'.format(value, in_ID)

    if (value == None ):
        offset = 0
    else:
        offset = value

    temp_volt = (in_value + offset) / 615.80
    tmp_real = (in_value + offset) *0.069 - 20.09

    print 'external pure temp ADC value = {0}, estimated Volt={1}V, estimated temp={2}'.format(in_value, temp_volt, tmp_real)

    return tmp_real

while True:
    
    # wait for a request
    #request,dist_addr = socket_handler.recvfrom(1024)
    data,dist_addr = socket_handler.recvfrom(1024)
 
    hisAddress     = dist_addr[0]
    hisPort        = dist_addr[1]
    systemTime = datetime.datetime.now()

#    counter        = struct.unpack('<h',data)[0]
    print len(data)
    # checksum1, checksum2, asn1, asn2, asn3, asn4, asn5,counter,int_temp,ext_temp,ext_pyra,int_volt,gpio_pulse = struct.unpack('=7B HHHHHH',data)
    counter,int_temp,ext_temp,ext_pyra,int_volt,gpio_pulse = struct.unpack('HHHHHH',data)
    #d1, inner temp
    #d2, exter temp
    #d3, pyra
    #d4, volt
    temp_volt = int_temp * CONST
    #i_temp_real = (temp_volt - OFFSET_0C) / TEMP_COEFF
    i_temp_real = int_temp
    print 'internal temp = %2.2f' % i_temp_real

    hisAddress_split = hisAddress.split(':')
    #pure_value = ext_temp
    temp_real = convert_adc_to_temp_by_id(hisAddress_split[5], ext_temp)
    #temp_volt = pure_value / 615.80
    #temp_real = pure_value *0.069 - 20.09
    print 'external temp = %2.2f' % temp_real

    #pure_value = ext_pyra
    if (ext_pyra > 2047):
        pyra_real = 0
        print 'detect estimated pyra < 0'
        pyra_raw = ext_pyra
    else:
        #pyra_volt = pure_value * 1200 / 2048
        #pyra_real = pyra_volt * 1000 / 1200
        pyra_raw = ext_pyra
        pyra_real = convert_adc_to_batt_by_id(hisAddress_split[5], ext_pyra)
        print 'PA0={0}, estimated pyra = {1}'.format(ext_pyra, pyra_real)
    print 'external batt = %2.2f V' % pyra_real

    #pure_value = int_volt
    #i_volt_real = pure_value / 586.85
    i_volt_real = int_volt
    #print 'raw i_volt = %2.2f' % i_volt_real
    print 'raw i_volt = %d' % i_volt_real

    address = ''
    for i in hisAddress_split[2:]:
        address += '%04x' % int(i, 16)
    
    print 'hisAddress=%s %s %s %s'%(hisAddress_split[2],hisAddress_split[3],hisAddress_split[4],hisAddress_split[5])

    #open db connection 
    db = MySQLdb.connect("localhost","root","sakimaru","ITRI_OpenWSN" )
    cursor = db.cursor()

    #get current time
    #t = datetime.datetime.now()
    currtime = systemTime.strftime("%Y-%m-%d %H:%M:%S")

    #mySql cmd
    sql = "INSERT INTO itri_MOEA_sensor(sn, mac_addr, \
        ext_temperature, pyranometer, datetime, int_temperature, battery_volt) \
        VALUES ('%d', '%s', '%.2f', '%d', '%s', '%.2f', '%.2f')" %\
        (counter, address, temp_real, pyra_raw, currtime, i_temp_real, pyra_real)
    rps_sql = "REPLACE INTO itri_MOEA_current_sensor(sn, mac_addr, \
        ext_temperature, pyranometer, datetime, int_temperature, battery_volt) \
        VALUES ('%d', '%s', '%.2f', '%d', '%s', '%.2f', '%.2f')" %\
        (counter, address, temp_real, pyra_raw, currtime, i_temp_real, pyra_real)
    try:
        # Execute the SQL command
        cursor.execute(sql)
        cursor.execute(rps_sql)
        # Commit your changes in the database
        db.commit()
    except:
        # Rollback in case there is any error
        db.rollback()
    
    # disconnect from server
    db.close()

    print 'insert DB ok !!'
