# coding=utf-8

# serial logger for heat meter

# Copyright (c) 2015 arsh0r
# See the file LICENSE for copying permission.

#$screen -S logger
#$python logger.py
#CTRL+AD
#$screen -r

from __future__ import print_function
from backports import lzma
import datetime, serial, io, os, sys

addr  = '/dev/ttyACM0'
baud  = 9600
fname = 'heatmeter'

pt = serial.Serial(addr,baud)
spb = io.TextIOWrapper(io.BufferedRWPair(pt,pt,1), encoding='ascii', errors='ignore', newline='\n',line_buffering=True)

lastfilename = ''
headwritten = False
outf = None
spb.readline() #drop first incomplete line
while (1):
    filename = 'log/' + datetime.datetime.now().strftime('%y%m%d') + '_' + fname + '.csv.xz'
    if outf == None or filename != lastfilename:
        headwritten = os.path.isfile(filename)
        if headwritten:
            with lzma.open(filename) as f:
                f.seek(0, os.SEEK_END)
                size = f.tell()
                if size == 0:
                    headwritten = False
        outf = lzma.open(filename, 'a')
        lastfilename = filename
    
    if not headwritten:
        print('ts,heat_power,flow_rate,tdiff,heat_quantity,tforward,tbackward', file=outf)
        headwritten = True    
    hmline = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S') + ',' + spb.readline()
    print(hmline,end='')    
    print(hmline,end='', file=outf)    

