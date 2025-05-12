#!/usr/bin/env python3

import sys
import matplotlib.pyplot as plt
import csv
import numpy as np

title = sys.argv[1]
files = sys.argv[2:]
titles = [t[-1] for t in [f.split('_') for f in files]]

# for i in range(0, len(files), 2):
#     x = []
#     y = []
#     yerr = []
#     with open(files[i], 'r') as avgFile:
#         with open(files[i+1], 'r') as stdevFile:
#             avgs = csv.reader(avgFile, delimiter='\t')
#             stdevs = csv.reader(stdevFile, delimiter='\t')
#             next(avgs)
#             next(stdevs)
#             for avg, stdev in zip(avgs, stdevs):
#                 x.append(int(avg[0]))
#                 if "PSTM" in titles[i]:
#                     y.append(float(avg[4]) / 1.0e3)
#                     yerr.append(float(stdev[4]) / 1.0e3)
#                 else:
#                     y.append(float(avg[9]) / 1.0e3)
#                     yerr.append(float(stdev[9]) / 1.0e3)
#             plt.errorbar(x, y, yerr=yerr, label=titles[i])
#             plt.xlabel('Threads')
#             plt.ylabel('Throughput (kTXs/s)')
#             plt.legend()
# plt.title(title)
# plt.savefig("plot_throughput_" + title + ".png", dpi=150)

#FUNCTIONAL VERSION, NO ERROR BAR
avgstring="avg"
for i in range(0, len(files)):
	if avgstring in files[i]:
	    x,y1,y2 = [],[],[]
	    with open(files[i], 'r') as avgFile:
	        avgs = csv.reader(avgFile, delimiter='\t')
	        next(avgs)
	        for avg in avgs:
	                y1.append(float(avg[9]))
	                y2.append(float(avg[10]))
	                # y3.append(float(avg[11]))
	                # y4.append(float(avg[12]))

	        # print (title)
	        # print (files[i])
	        # print (y)
	        x=[1,2,4,8,12,16,20,24] #,28,32,36,40,44,48]

	        if "usePCWM_" in files[i]:
	            plt.plot(x, y1, label = "SPHT PI") #"PCWM")
	            plt.plot(x, y2, label = "SPHT PO") #"PCWM")
	            # plt.plot(x, y3, label = "SPHT POActiv") #"PCWM")
	            # plt.plot(x, y4, label = "SPHT RAM Usage (%)") #"PCWM")

	        if "usePCWMeADR_" in files[i]:
	            plt.plot(x, y1, label = "SPHT-eADR PI") #"PCWMeADR")
	            plt.plot(x, y2, label = "SPHT-eADR PO") #"PCWMeADR")
	            # plt.plot(x, y3, label = "SPHT-eADR POActiv") #"PCWMeADR")
	            # plt.plot(x, y4, label = "SPHT-eADR RAM Usage (%)") #"PCWM")
	        if "usePCWMeADRT1_" in files[i]:
	            plt.plot(x, y1, label = "Smart Close PI") #"PCWMeADR+Flush")
	            plt.plot(x, y2, label = "Smart Close PO") #"PCWMeADR+Flush")
	            # plt.plot(x, y3, label = "Smart Close POActiv") #"PCWMeADR+Flush")
	            # plt.plot(x, y4, label = "Smart Close RAM Usage (%)") #"PCWMeADR+Flush")
	        if "useSharedHTM_" in files[i]:
	            plt.plot(x, y1, label = "HTM Only (Shared) PI") #useSharedHTM
	            plt.plot(x, y2, label = "HTM Only (Shared) PO") #useSharedHTM
	            # plt.plot(x, y3, label = "HTM Only (Shared) POActiv") #useSharedHTM
	            # plt.plot(x, y4, label = "HTM Only (Shared) RAM Usage (%)") #useSharedHTM
	        
	        plt.xlabel('Threads')
	        plt.ylabel('Page In & Page Out')
	        plt.legend()
	        plt.grid()
	else:
		i=i+1
plt.title(title)
plt.savefig("plot_paging_" + title + ".png", dpi=150)

