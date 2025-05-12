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

fig = plt.figure(figsize=(6,3))
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
                # if "PCWM-" in titles[i]:
                #     normalization[titles[i]] = []
                # for avgLog, stdevLog, param in zip(avgsLog, stdevsLog, params):
                #     if "PCWM-" in titles[i]:
                #         normalization[titles[i]] += [float(avgLog[13])]
                if "PCWM-" in titles[i]:
                    continue
                if "REP1" in titles[i]:
                    normalization[titles[i]] = []
                for avgLog, stdevLog, param in zip(avgsLog, stdevsLog, params):
                    if "REP1" in titles[i]:
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
                if "REP1" in titles[i]:
                    continue

                titleAxis = ""
                # if "PCWM2" in titles[i]:
                #     titleAxis += "linked, "
                # else:
                #     titleAxis += "sort, "
                # if "W1" in titles[i] and "REP1" in titles[i]:
                #     color = "red"
                #     titleAxis += "1 W/TX, 8 replay"
                if "W1" in titles[i] and "REP2" in titles[i]:
                    color = "red"
                    titleAxis += "1 W/TX, 2 replayers"
                    linestyle = "-"
                if "W1" in titles[i] and "REP4" in titles[i]:
                    color = "orange"
                    titleAxis += "1 W/TX, 4 replayers"
                    linestyle = ":"
                if "W1" in titles[i] and "REP8" in titles[i]:
                    color = "red"
                    titleAxis += "1 W/TX, 8 replayers"
                    linestyle = "-."

                # elif "W5" in titles[i] and "REP1" in titles[i]:
                #     color = "blue"
                #     titleAxis += "5 W/TX, 1 replayer"
                elif "W5" in titles[i] and "REP2" in titles[i]:
                    color = "blue"
                    titleAxis += "5 W/TX, 2 replayer"
                    linestyle = "-"
                elif "W5" in titles[i] and "REP4" in titles[i]:
                    color = "purple"
                    titleAxis += "5 W/TX, 4 replayer"
                    linestyle = ":"
                elif "W5" in titles[i] and "REP8" in titles[i]:
                    color = "blue"
                    titleAxis += "5 W/TX, 8 replayers"
                    linestyle = "solid"
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
                    print("n:", n[-3:], "title[i]:", titles[i][-3:])
                    if titles[i][-3:] == n[-3:]:
                        y = y / np.array(normalization[n])
                        yerr = yerr / np.array(normalization[n])
                        break
                # if "REP1" in titles[i]:
                #     ax.errorbar(x, y, yerr=yerr, label=titleAxis, linestyle="--", color=color)
                # else:
                #     ax.errorbar(x, y, yerr=yerr, label=titleAxis, color=color)
                ax.errorbar(x, y, yerr=yerr, label=titleAxis, linestyle=linestyle, color=color)

# plt.ylim(bottom=0.9, top=3.8)
plt.xlabel('Heap size (MB)', size=14)
# plt.ylabel('Throughput (M logged writes / s)', size=10.5)
# plt.ylabel('Linked log throughput normalized to \n sorting during replay', size=10.5)
plt.ylabel('Speedup vs seq. replayer', size=14)
plt.legend(prop={'size': 13})
plt.margins(0.01, 0.01)
plt.xscale("symlog", basex=2)
yticks = [0.0, 0.5, 1.0, 1.5, 2, 2.5, 3, 3.5]
# yticks_labels = [0, 1, 2, 3, 4, 5, 6, 7, 8]
plt.yticks(yticks, yticks, size = 12)
plt.xticks(x, [int(a) for a in x], rotation=70, size=12)
plt.gca().xaxis.grid(True, linestyle="--", linewidth=0.2)
plt.gca().yaxis.grid(True, linestyle="--", linewidth=0.2)

ax.yaxis.set_label_coords(-0.09,0.46)
ax.xaxis.set_label_coords(0.5,-0.225)
box = ax.get_position()
ax.set_position([box.x0 - box.width * 0.0, box.y0 + box.height * 0.158,
                box.width * 1.1, box.height * 0.97])

# plt.title("Single Threaded; 50M writes; 64 logs")
plt.savefig("plot_log_absVal_" + title + ".pdf", dpi=150)
