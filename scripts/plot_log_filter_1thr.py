#!/usr/bin/env python3

import sys
import matplotlib.pyplot as plt
import numpy as np
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

fig = plt.figure(figsize=(6,4))
ax = plt.subplot(1, 1, 1)

filter_rate = []
first_run = True
nbSamples = 0
sumFilter = []

normalization = {}

for i in range(0, len(files), 2):
    # with open(files[i], 'r') as avgFile:
    #     with open(files[i+1], 'r') as stdevFile:
    with open("log_" + files[i], 'r') as avgLogFile:
        # avgs = csv.reader(avgFile, delimiter='\t')
        # stdevs = csv.reader(stdevFile, delimiter='\t')
        avgsLog = csv.reader(avgLogFile, delimiter='\t')
        next(avgsLog)

        if "PCWM2" in titles[i]:
            for avgLog in zip(avgsLog):
                avg = avgLog[0]
                if "H1048576B" in titles[i]:
                    normalization["H1048576B"] = float(avg[14])
                elif "H33554432B" in titles[i]:
                    normalization["H33554432B"] = float(avg[14])
                elif "H536870912B" in titles[i]:
                    normalization["H536870912B"] = float(avg[14])
                break

        ### normalize with respect to the first point
        # next(avgsLog)

        if "PCWM3" in titles[i]:
            nbSamples += 1
            i = 0
            for avgLog in zip(avgsLog):
                avg = avgLog[0]
                if first_run:
                    sumFilter += [1.0 - (float(avg[12]) / ((float(avg[3]) - 2.0 * float(avg[4])) / 2.0))]
                else:
                    sumFilter[i] += 1.0 - (float(avg[12]) / ((float(avg[3]) - 2.0 * float(avg[4])) / 2.0))
                    i += 1
            first_run = False

filter_rate = np.array(sumFilter) / nbSamples
print("dup array:", filter_rate)
print("norm array:", normalization)

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

            ### moves the first point
            # next(avgsLog)
            # next(stdevsLog)

            titleAxis = ""
            
            if "PCWM2" in titles[i]:
                titleAxis += "no-filter, "
            elif "PCWM3" in titles[i]:
                titleAxis += "filter, "

            if "1048576" in titles[i]:
                titleAxis += "1MB"
                color = "red"
                # if "PCWM2" in titles[i]:
                #     color = "orange"
            elif "33554432" in titles[i]:
                titleAxis += "32MB"
                color = "blue"
                # if "PCWM2" in titles[i]:
                #     color = "purple"
            elif "536870912" in titles[i]:
                titleAxis += "512MB"
                color = "green"
                # if "PCWM2" in titles[i]:
                #     color = "turquoise"

            # for avg, stdev, avgLog, stdevLog in zip(avgs, stdevs, avgsLog, stdevsLog):
            norm = 0
            for size in normalization:
                if size in titles[i]:
                    norm = normalization[size]
                    break

            for avgLog, stdevLog in zip(avgsLog, stdevsLog):
                # x = filter_rate[1:]
                x = filter_rate

                y.append(float(avgLog[14]) / 1e6)
                yerr.append(float(stdevLog[14]) / 1e6)

                # print("norm to:", norm)
                # y.append(float(avgLog[14]) / norm)
                # yerr.append(float(stdevLog[14]) / norm)

            if "PCWM2" in titles[i]:
                ax.errorbar(x, y, yerr=yerr, label=titleAxis, linestyle="--", color=color)
            else:
                ax.errorbar(x, y, yerr=yerr, label=titleAxis, color=color)


plt.xlabel('Duplicates in log', size=13)
plt.ylabel('Throughput (M logged writes / s)', size=13)
# plt.ylabel('Throughput normalized to \n ' + str(np.round(filter_rate[0], decimals=3)) + 'dup no-filter', size=12)
plt.legend(prop={'size': 10.2})
plt.margins(0.01, 0.01)

ax.set_ylim(bottom=2.2, top=55)
plt.yscale("log") #  , basex=10 linthreshy=0.0015
yticks = [2.5, 3.0, 4, 5, 7, 10, 15, 20, 30, 40, 50]
plt.yticks(yticks, yticks)
plt.xticks(rotation=70)
plt.xticks(x, np.round(x, decimals=3))

plt.gca().xaxis.grid(True, linestyle="--", linewidth=0.2)
plt.gca().yaxis.grid(True, linestyle="--", linewidth=0.2)

box = ax.get_position()
ax.set_position([box.x0 - box.width * 0.03, box.y0 + box.height * 0.105,
                box.width * 1.15, box.height * 1.046])

plt.savefig("plot_log_" + title + ".pdf", dpi=250)
