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

fig = plt.figure(figsize=(6,3.6))
ax = plt.subplot(1, 1, 1)

normalization = {}

for i in range(0, len(files), 2):
    with open("log_" + files[i], 'r') as avgLogFile:
        with open("log_" + files[i+1], 'r') as stdevLogFile:
            with open("params_" + files[i], 'r') as paramsFile:
                # avgs = csv.reader(avgFile, delimiter='\t')
                # stdevs = csv.reader(stdevFile, delimiter='\t')
                avgsLog = csv.reader(avgLogFile, delimiter='\t')
                params = csv.reader(paramsFile, delimiter='\t')
                stdevsLog = csv.reader(stdevLogFile, delimiter='\t')
                print(files[i] + ", " + files[i+1])
                # next(avgs)
                # next(stdevs)
                next(avgsLog)
                next(stdevsLog)
                next(params)
                if "PCWM-" in titles[i]:
                    normalization[titles[i]] = []
                for avgLog, stdevLog, param in zip(avgsLog, stdevsLog, params):
                    if "PCWM-" in titles[i]:
                        normalization[titles[i]] += [float(avgLog[13])]

for i in range(0, len(files), 2):
    x = []
    y = []
    yerr = []
    # with open(files[i], 'r') as avgFile:
    #     with open(files[i+1], 'r') as stdevFile:
    with open("log_" + files[i], 'r') as avgLogFile:
        with open("log_" + files[i+1], 'r') as stdevLogFile:
            with open("params_" + files[i], 'r') as paramsFile:
                # avgs = csv.reader(avgFile, delimiter='\t')
                # stdevs = csv.reader(stdevFile, delimiter='\t')
                avgsLog = csv.reader(avgLogFile, delimiter='\t')
                params = csv.reader(paramsFile, delimiter='\t')
                stdevsLog = csv.reader(stdevLogFile, delimiter='\t')
                print(files[i] + ", " + files[i+1])
                # next(avgs)
                # next(stdevs)
                next(avgsLog)
                next(stdevsLog)
                next(params)

                ### skips sorting (normalizes to it)
                if "PCWM-" in titles[i]:
                    continue

                titleAxis = ""
                # if "PCWM2" in titles[i]:
                #     titleAxis += "log linked, "
                # else:
                #     titleAxis += "order on replay, "
                if "W1" in titles[i] and "REP1" in titles[i]:
                    color = "red"
                    titleAxis += "1 W/TX, 1 replayer"
                if "W1" in titles[i] and "REP8" in titles[i]:
                    color = "red"
                    # color = "orange"
                    titleAxis += "1 W/TX, 8 replayers"
                elif "W5" in titles[i] and "REP1" in titles[i]:
                    color = "blue"
                    titleAxis += "5 W/TX, 1 replayer"
                elif "W5" in titles[i] and "REP8" in titles[i]:
                    color = "blue"
                    # color = "purple"
                    titleAxis += "5 W/TX, 8 replayers"
                # elif "REP64" in titles[i]:
                #     color = "green"
                #     titleAxis += "64thr"

                # for avg, stdev, avgLog, stdevLog in zip(avgs, stdevs, avgsLog, stdevsLog):
                for avgLog, stdevLog, param in zip(avgsLog, stdevsLog, params):
                    x.append(int(param[0]) / 1048576)

                    y.append(float(avgLog[13]))
                    yerr.append(float(stdevLog[13]))

                    # print("y:", float(avgLog[13]), ", np(y):", np.array(float(avgLog[13])))

                    # y.append(float(avgLog[13]) / 1e6)
                    # yerr.append(float(stdevLog[13]) / 1e6)

                for n in normalization:
                    # print("n:", n[-8:], "title[i]:", titles[i][-8:])
                    if titles[i][-8:] == n[-8:]:
                        y = y / np.array(normalization[n])
                        yerr = yerr / np.array(normalization[n])
                        break
                if "REP1" in titles[i]:
                    ax.errorbar(x, y, yerr=yerr, label=titleAxis, linestyle="--", color=color)
                else:
                    ax.errorbar(x, y, yerr=yerr, label=titleAxis, color=color)

plt.ylim(bottom=0.9, top=3.8)
plt.xlabel('Persistent Heap size (MB)', size=12)
# plt.ylabel('Throughput (M logged writes / s)', size=10.5)
plt.ylabel('Speedup of linking vs sorting', size=12, labelpad=6)
plt.legend(prop={'size': 10})
plt.margins(0.01, 0.01)
plt.xscale("symlog", basex=2)
yticks = [1.0, 1.2, 1.4, 1.6, 1.8, 2, 2.2, 2.5, 2.8, 3, 3.2, 3.5, 3.8]
plt.yticks(yticks, yticks, size=11)
plt.xticks(x, [int(a) for a in x], rotation=70, size=11)
ax.tick_params(axis="x", pad = -0.1, size=2, labelsize=11)
ax.tick_params(axis="y", pad = -0.1, size=2, labelsize=11)
plt.gca().xaxis.grid(True, linestyle="--", linewidth=0.2)
plt.gca().yaxis.grid(True, linestyle="--", linewidth=0.2)

box = ax.get_position()
ax.set_position([box.x0 - box.width * 0.04, box.y0 + box.height * 0.1,
                box.width * 1.15, box.height * 1.0])

# plt.title("Single Threaded; 50M writes; 64 logs")
plt.savefig("plot_log_" + title + ".pdf", dpi=150)
