#!/usr/bin/env python3

import sys
import matplotlib.pyplot as plt
import csv
import numpy as np

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

map_title = {
    "L262140"     : "d=90%",
    "L131070"     : "d=80%",
    "L13107"      : "d=20%",
    "L134217720"  : "d=90%",
    "L67108860"   : "d=80%",
    "L6710886"    : "d=20%"
}

map_1M = {
    "L262140"  : "blue",
    "L131070"  : "green",
    "L13107"   : "red"
}

map_512M = {
    "L134217720"  : "blue",
    "L67108860"   : "green",
    "L6710886"    : "red"
}

fig, axs = plt.subplots(1, 2, figsize=(6,3), sharey=False, gridspec_kw={'wspace': 0.0})

xoffset = -0.02

### TODO:
# files_sorted = []

# for f in files:
#     if "L13107_" in f:
#         files_sorted += [f]

# for f in files:
#     if "L131070_" in f:
#         files_sorted += [f]

# for f in files:
#     if "L262140_" in f:
#         files_sorted += [f]

# for f in files:
#     if "L6710886_" in f:
#         files_sorted += [f]

# for f in files:
#     if "L67108860_" in f:
#         files_sorted += [f]

# for f in files:
#     if "L134217720_" in f:
#         files_sorted += [f]

# for f in files_sorted:
#     print(f)

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
            # print(files[i] + ", " + files[i+1])
            # next(avgs)
            # next(stdevs)
            next(avgsLog)
            next(stdevsLog)

            # titleAxis = "linked, 5 W/TX, "
            titleAxis = ""

            if "PCWM3" in titles[i]:
                titleAxis += "filter, "
            for a in map_title:
                if a in titles[i]:
                    titleAxis += map_title[a]
                    break

            if "PCWM2" in titles[i]:
                titleAxis = "no-filter"

            axNb = 0
            color = "red"
            for a in map_1M:
                if a in titles[i]:
                    axNb = 0
                    color = map_1M[a]
                    break
            for b in map_512M:
                if b in titles[i]:
                    axNb = 1
                    color = map_512M[b]
                    break

            # for avg, stdev, avgLog, stdevLog in zip(avgs, stdevs, avgsLog, stdevsLog):
            for avgLog, stdevLog in zip(avgsLog, stdevsLog):
                x.append(int(avgLog[1]))
                # y.append((float(avgLog[6]) / 2300000000.0))
                # yerr.append((float(stdevLog[6]) / 2300000000.0))
                # y.append(float(avgLog[4]) / (float(avgLog[6]) / 2300000000.0) / 1e6)
                # yerr.append(1.0 / (float(stdevLog[6]) / 2300000000.0)  / 1e6)
                y.append(float(avgLog[13]) / 1e6)
                yerr.append(float(stdevLog[13]) / 1e6)
            x_offsetted = np.array(x) + xoffset * np.array(x)
            new_y = np.array(y) #/ y[0]
            new_yerr = np.array(yerr) #/ y[0]
            print("adding:", titleAxis)
            if "PCWM2" in titles[i]:
                axs[axNb].errorbar(x_offsetted, new_y, yerr=new_yerr, label=titleAxis, linestyle="--", color=color)
            else:
                axs[axNb].errorbar(x_offsetted, new_y, yerr=new_yerr, label=titleAxis, color=color)
            xoffset += 0.005

plt.setp(axs)
axs[0].set_xlabel('Number of Replayers', x=1.0, size=12)
axs[0].set_ylabel("Throughput (MTXs/s)", size=12, labelpad=0.6)
# axs[0].set_ylabel('Throughput normalized to 1 thread (M entries / s)')
plt.sca(axs[0])
plt.legend( ncol=1, handletextpad=0.15, columnspacing=0.9, prop={'size': 12})

for ax in axs:
    ax.set_xscale("log", basex=2)
    ax.xaxis.grid(True, linestyle="--", linewidth=0.2)
    ax.yaxis.grid(True, linestyle="--", linewidth=0.2)
# axs[0].margins(0.01, 0.05)
# axs[1].margins(0.05, 0.01)
# axs[0].set_xticks(xticks, xticks)
# axs[1].set_xticks(xticks, xticks)
# plt.xticks(xticks, xticks)

plt.sca(axs[0])
xticks = [1, 2, 4, 8, 16, 32, 64]
plt.xticks(xticks, xticks, size=10)
axs[0].tick_params(axis="y", labelrotation = 45, pad = -0.1, size=1, labelsize=11)

plt.sca(axs[1])
xticks = [1, 2, 4, 8, 16, 32, 64]
#yticks = [0.5, 1, 1.5, 2, 2.5, 3]
plt.xticks(xticks, xticks)
#axs[1].set_yticks(yticks, yticks)
axs[1].tick_params(axis="y", labelrotation = 45, pad = -0.1, size=1, labelsize=11)

box = axs[0].get_position()
axs[0].set_position([
    box.x0 - box.width * 0.12,
    box.y0 + box.height * 0.053,
    box.width * 1.12,
    box.height * 1.01
])

box = axs[1].get_position()
axs[1].set_position([
    box.x0 + box.width * 0.13,
    box.y0 + box.height * 0.053,
    box.width * 1.12,
    box.height * 1.01
])

plt.sca(axs[0])
plt.title("1MB heap")


plt.sca(axs[1])
plt.title("512MB heap")

# axs[0].set_ylim(0, 1.5)
# axs[1].set_ylim(0, 3)

# yticks = [0.2, 0.4, 0.6, 0.8, 1, 1.2, 1.4]
# yticks_l = ["0.2", "0.4", "0.6", "0.8", "1.0", "1.2", "1.4"]
# plt.sca(axs[0])
# plt.yticks(yticks, yticks_l)

plt.sca(axs[1])
xticks = [1, 2, 4, 8, 16, 32, 64]
# yticks = [0.5, 1, 1.5, 2, 2.5, 3]
# yticks_ = ["0.5", "1.0", "1.5", "2.0", "2.5", "3.0"]
# yticks = [10, 20, 30, 40, 50]
# plt.yticks(yticks, yticks)

plt.margins(0.04)

plt.savefig("plot_log_" + title + ".png", dpi=150)

