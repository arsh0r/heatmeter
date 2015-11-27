# coding=utf-8

# plot heat meter logfiles

# Copyright (c) 2015 arsh0r
# See the file LICENSE for copying permission.

import os, time
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from backports import lzma

def movingaverage(interval, window_size):
    window= np.ones(int(window_size))/float(window_size)
    return np.convolve(interval, window, 'same')
    
def read_datafile(file_name):
   with lzma.open(file_name, "r") as f:
      for line in f:
         if line.startswith('ts,'):
            names=line.rstrip().split(',')
            break
      data = np.genfromtxt(f, delimiter=',', comments='#', skiprows=2, names=names,
                           usecols=(range(0,7)),
                           converters={0: mdates.strpdate2num('%Y-%m-%d %H:%M:%S')})
   return data

def makefig (fn):
    data = read_datafile(fn+'.csv.xz')
    fig = plt.figure(figsize=(10.24,7.68),dpi=80)

    ax1 = fig.add_subplot(211)
    ax1.set_title('heat meter on '+mdates.num2date(data['ts'][0]).strftime('%Y-%m-%d')
                  +' [%0.1f'%(np.sum(data['heat_quantity']))+' kWh]')    
    ax1.xaxis.set_major_locator(mdates.HourLocator())
    ax1.xaxis.set_major_formatter(mdates.DateFormatter('%H'))
    ax1.xaxis.set_minor_locator(mdates.MinuteLocator(range(0,59,15)))
    ax1.set_ylabel('heat power [kW]', color='red')
    ax1.set_ylim(0,20)
    ax1.step(x=data['ts'], y=data['heat_power'], where='post', color='red', label='heat_power')

    ax2 = ax1.twinx()
    ax2.set_ylabel('flow rate [l/s]', color='blue')
    ax2.set_ylim(0,1)
    ax2.step(x=data['ts'], y=data['flow_rate'], where='post', color='blue', label='flow_rate')

    ax3 = fig.add_subplot(212)
    ax3.set_xlabel('time')
    ax3.xaxis.set_major_locator(mdates.HourLocator())
    ax3.xaxis.set_major_formatter(mdates.DateFormatter('%H'))
    ax3.xaxis.set_minor_locator(mdates.MinuteLocator(range(0,59,15)))

    ax3.set_ylabel(u'absolute temperature [°C]', color='purple')
    ax3.set_ylim(0,70)
    ax3.plot_date(x=data['ts'], y=data['tforward'], tz=None, xdate=True, fmt='-', color='red', label='tforward')
    ax3.plot_date(x=data['ts'], y=data['tbackward'], tz=None, xdate=True, fmt='-', color='blue', label='tbackward')

    ax4 = ax3.twinx()
    ax4.set_ylabel('temperature difference [K]', color='green')
    ax4.set_ylim(0,30)
    ax4.plot_date(x=data['ts'], y=data['tdiff'], tz=None, xdate=True, fmt='-', color='green', label='tdiff')

    plt.savefig(fn+'.png')



for filename in os.listdir('.'):
    fn, ext = filename.split('.', 1) 
    fnnow = time.strftime('%y%m%d')+'_heatmeter'   
    if (ext == 'csv.xz' and fn != fnnow):
        if not os.path.isfile(fn+'.png'):
            print('creating '+fn+'.png')            
            makefig(fn)

