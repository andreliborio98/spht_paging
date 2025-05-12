#!/usr/bin/env python3

import sys
import matplotlib.pyplot as plt
import csv

from plot_map_title import map_title

title = sys.argv[1]
files = sys.argv[2:]
titles = [t[-2] for t in [f.split('_') for f in files]]

for i in range(0, len(files), 2):
    x = []
    y = []
    yerr = []
    with open(files[i], 'r') as avgFile:
        with open(files[i+1], 'r') as stdevFile:
            with open("log_" + files[i], 'r') as avgLogFile:
                with open("log_" + files[i+1], 'r') as stdevLogFile:
                    avgs = csv.reader(avgFile, delimiter='\t')
                    stdevs = csv.reader(stdevFile, delimiter='\t')
                    avgsLog = csv.reader(avgLogFile, delimiter='\t')
                    stdevsLog = csv.reader(stdevLogFile, delimiter='\t')
                    next(avgs)
                    next(stdevs)
                    next(avgsLog)
                    next(stdevsLog)

                    titleAxis = map_title(titles[i])

                    for avg, stdev, avgLog, stdevLog in zip(avgs, stdevs, avgsLog, stdevsLog):
                        x.append(int(avgLog[0]))
                        # y.append((float(avgLog[6]) / 2300000000.0))
                        # yerr.append((float(stdevLog[6]) / 2300000000.0))
                        # y.append(float(avgLog[4]) / (float(avgLog[6]) / 2300000000.0) / 1e6)
                        # yerr.append(1.0 / (float(stdevLog[6]) / 2300000000.0)  / 1e6)
                        y.append(float(avgLog[12]) / 1e6)
                        yerr.append(float(stdevLog[12]) / 1e6)
                    plt.errorbar(x, y, yerr=yerr, label=titleAxis)
                    plt.xlabel('Nb Logs')
                    plt.ylabel('Throughput (M entries / s)')
                    plt.legend(prop={'size': 6})

plt.title("Solutions that sort the log")
plt.savefig("plot_log_" + title + ".png", dpi=150)

