#!/usr/bin/env python3

import sys
import matplotlib.pyplot as plt
import csv

from plot_map_title import map_title

title = sys.argv[1]
files = sys.argv[2:]
titles = [t[-2] for t in [f.split('_') for f in files]]

map_colors = {
    "2097152"   : "red",
    "33554432"  : "orange",
    "536870912" : "blue",
    "L134217720": "green",
    "L67108860" : "orange",
    "L6710886"  : "purple",
    "PCWM3"     : "orange",
    "PCWM2"     : "blue",
    "BACKWARD"  : "pink"
}

fig = plt.figure(figsize=(6,7))
ax = plt.subplot(1, 1, 1)

for i in range(0, len(files), 2):
    x = []
    y = []
    yerr = []
    # with open(files[i], 'r') as avgFile:
    #     with open(files[i+1], 'r') as stdevFile:
    with open("log_" + files[i], 'r') as avgLogFile:
        with open("log_" + files[i+1], 'r') as stdevLogFile:
            # avgs = csv.reader(avgFile, delimiter='\t')
            # stdevs = csv.reader(stdevFile, delimiter='\t')
            avgsLog = csv.reader(avgLogFile, delimiter='\t')
            stdevsLog = csv.reader(stdevLogFile, delimiter='\t')
            print(files[i] + ", " + files[i+1])
            # next(avgs)
            # next(stdevs)
            next(avgsLog)
            next(stdevsLog)

            titleAxis = map_title(titles[i])

            # for avg, stdev, avgLog, stdevLog in zip(avgs, stdevs, avgsLog, stdevsLog):
            for avgLog, stdevLog in zip(avgsLog, stdevsLog):
                x.append(int(avgLog[1]))
                # y.append((float(avgLog[6]) / 2300000000.0))
                # yerr.append((float(stdevLog[6]) / 2300000000.0))
                # y.append(float(avgLog[4]) / (float(avgLog[6]) / 2300000000.0) / 1e6)
                # yerr.append(1.0 / (float(stdevLog[6]) / 2300000000.0)  / 1e6)
                y.append(float(avgLog[13]) / 1e6)
                yerr.append(float(stdevLog[13]) / 1e6)
            color = "red"
            for c in map_colors:
                if c in titles[i]:
                    color = map_colors[c]
                    break
            if "PCWM2" in titles[i]:
                ax.errorbar(x, y, yerr=yerr, label=titleAxis, linestyle="--", color=color)
            else:
                ax.errorbar(x, y, yerr=yerr, label=titleAxis, color=color)

plt.xlabel('Nb Replayers')
plt.ylabel('Throughput (M entries / s)')
plt.legend(prop={'size': 6})
plt.xscale("symlog", basex=2)
plt.title("Parallel Replayer")
plt.savefig("plot_log_" + title + ".png", dpi=150)

