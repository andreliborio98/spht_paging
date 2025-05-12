#!/usr/bin/env python3

import sys
import matplotlib.pyplot as plt
import csv
import numpy as np

from plot_map_title import map_title

title = sys.argv[1]
files = sys.argv[2:]
titles = [t[-2] for t in [f.split('_') for f in files]]

map_colors = {
    "2097152"   : "red",
    "131070"    : "red",
    "13107"     : "blue",
    "262140"    : "orange",
    "33554432"  : "orange",
    "536870912" : "blue",
    "L134217720": "orange",
    "L67108860" : "red",
    "L6710886"  : "blue",
    "PCWM3"     : "orange",
    "PCWM2"     : "blue",
    "BACKWARD"  : "pink"
}

fig = plt.figure(figsize=(6,4))
ax = plt.subplot(1, 1, 1)

filter_rate = []

for i in range(0, len(files), 2):

    # with open(files[i], 'r') as avgFile:
    #     with open(files[i+1], 'r') as stdevFile:
    with open("log_" + files[i], 'r') as avgLogFile:
        # avgs = csv.reader(avgFile, delimiter='\t')
        # stdevs = csv.reader(stdevFile, delimiter='\t')
        avgsLog = csv.reader(avgLogFile, delimiter='\t')
        next(avgsLog)

        # if "BACKWARD" in titles[i]:
        if "PCWM3" in titles[i]:
            nbSamples = 0
            sumFilter = 0.0
            for avgLog in zip(avgsLog):
                avg = avgLog[0]
                nbSamples += 1
                sumFilter += 1.0 - (float(avg[12]) / ((float(avg[3]) - 2.0 * float(avg[4])) / 2.0))
            res = (sumFilter / float(nbSamples))
            print(titles[i], ":", res)
            filter_rate += [ res ]

filter_rate = np.round(filter_rate, decimals=3)
print("dup array:", filter_rate)
            
for i in range(0, len(files), 2):
    x = []
    y = []
    yerr = []
    with open("log_" + files[i], 'r') as avgLogFile:
        with open("log_" + files[i+1], 'r') as stdevLogFile:
            # avgs = csv.reader(avgFile, delimiter='\t')
            # stdevs = csv.reader(stdevFile, delimiter='\t')
            avgsLog = csv.reader(avgLogFile, delimiter='\t')
            stdevsLog = csv.reader(stdevLogFile, delimiter='\t')
            # next(avgs)
            # next(stdevs)
            next(avgsLog)
            next(stdevsLog)

            titleAxis = ""
            if "PCWM2" in titles[i]:
                titleAxis += "no-filter, "
            if "PCWM3" in titles[i]:
                titleAxis += "filter, "

            # print(filter_rate)
            titleAxis += str(filter_rate[int(int(i/2) % (len(files) / 4))]) + " dup"

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
            # if "BUFFER-WBINVD" in titles[i]:
            if "PCWM2" in titles[i]:
                ax.errorbar(np.array(x) + np.array(x) * 0.025, y, yerr=yerr, label=titleAxis, linestyle="--", color=color)
            else:
                ax.errorbar(x, y, yerr=yerr, label=titleAxis, color=color)

plt.margins(0.01, 0.01)
plt.xlabel('Number of Threads', size=12)
plt.ylabel('Throughput (M TXs/s)', size=12)
lgd = plt.legend(fontsize=10)

plt.gca().xaxis.grid(True, linestyle="--", linewidth=0.2)
plt.gca().yaxis.grid(True, linestyle="--", linewidth=0.2)

plt.xlabel('Nb Replayers')
plt.ylabel('Throughput (M entries / s)')
plt.legend(prop={'size': 10})
plt.xscale("symlog", basex=2)
plt.xticks(x, np.round(x))
plt.title("")

box = ax.get_position()
ax.set_position([box.x0 - box.width * 0.04, box.y0 + box.height * 0.01,
                 box.width * 1.16, box.height * 1.12])

handles, labels = plt.gca().get_legend_handles_labels()
order = [2,0,1,5,3,4]
plt.legend([handles[idx] for idx in order],[labels[idx] for idx in order])

lgd.remove()
plt.savefig("plot_log_" + title + ".pdf", dpi=150)

