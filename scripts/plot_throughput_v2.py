#!/usr/bin/env python3

import sys
import matplotlib.pyplot as plt
import csv
import numpy as np

title = sys.argv[1]
xaxis = sys.argv[2]
yaxis = sys.argv[3]
files = sys.argv[4:]
titles = [t[-2] for t in [f.split('_') for f in files]]

for i in range(0, len(files), 2):
    x = []
    y = []
    yerr = []
    with open(files[i], 'r') as avgFile:
        with open(files[i+1], 'r') as stdevFile:
            with open('params_' + files[i], 'r') as avgParamsFile:
                with open('params_' + files[i], 'r') as stdevParamsFile:
                    avgsN = csv.reader(avgFile, delimiter='\t')
                    stdevsN = csv.reader(stdevFile, delimiter='\t')
                    avgsP = csv.reader(avgParamsFile, delimiter='\t')
                    stdevsP = csv.reader(stdevParamsFile, delimiter='\t')
                    
                    avgsN_ = [[a for a in t] for t in zip(avgsN)]
                    avgsN_ = [a[0] for a in avgsN_]
                    avgsP_ = [[a for a in t] for t in zip(avgsP)]
                    avgsP_ = [a[0] for a in avgsP_]
                    avgs = [a + b for a, b in zip(avgsN_, avgsP_)]
                    
                    stdevsN_ = [[a for a in t] for t in zip(stdevsN)]
                    stdevsN_ = [a[0] for a in stdevsN_]
                    stdevsP_ = [[a for a in t] for t in zip(stdevsP)]
                    stdevsP_ = [a[0] for a in stdevsP_]
                    stdevs = [a + b for a, b in zip(stdevsN_, stdevsP_)]

                    xtitle = avgs[0][int(xaxis)]
                    ytitle = stdevs[0][int(yaxis)]
                    
                    skipFirstLine = 0
                    for avg, stdev in zip(avgs, stdevs):
                        if skipFirstLine == 0:
                            skipFirstLine = 1 # ignores headers
                            continue
                        x.append(float(avg[int(xaxis)]))
                        y.append(float(avg[int(yaxis)]))
                        yerr.append(float(stdev[int(yaxis)]))

                    plt.errorbar(x, y, yerr=yerr, label=titles[i])
                    plt.xlabel(xtitle)
                    plt.ylabel(ytitle)
                    plt.legend()
plt.margins(0.01)
plt.subplots_adjust(left=0.15, right=0.98, top=0.93, bottom=0.1)
plt.title(title)
plt.savefig("plot_throughput_" + title + ".png", dpi=150)

