#!/usr/bin/env python3

import sys
import matplotlib.pyplot as plt
import csv

from plot_map_title import map_title

title = sys.argv[1]
files = sys.argv[2:]
titles = [t[-2] for t in [f.split('_') for f in files]]

fig = plt.figure(figsize=(6,4))
ax = plt.subplot(1, 1, 1)

for i in range(0, len(files), 2):
    x = []
    y = []
    yerr = []
    with open(files[i], 'r') as avgFile:
        with open(files[i+1], 'r') as stdevFile:
            avgs = csv.reader(avgFile, delimiter='\t')
            stdevs = csv.reader(stdevFile, delimiter='\t')
            next(avgs)
            next(stdevs)
            for avg, stdev in zip(avgs, stdevs):
                x.append(int(avg[0]))
                y.append(float(avg[1]) / 1.0e6)
                yerr.append(float(stdev[1]) / 1.0e6)
            titleAxis = map_title(titles[i])
            plt.errorbar(x, y, yerr=yerr, label=titleAxis)
            plt.xlabel('Threads')
            plt.ylabel('Throughput (MTXs/s)')
            plt.legend(fontsize=6)

plt.margins(0.01, 0.01)
plt.xlabel('Number of Threads', size=12)
plt.ylabel('Throughput (M TXs/s)', size=12)
plt.legend(fontsize=10)

plt.gca().xaxis.grid(True, linestyle="--", linewidth=0.2)
plt.gca().yaxis.grid(True, linestyle="--", linewidth=0.2)

box = ax.get_position()
ax.set_position([box.x0 - box.width * 0.04, box.y0 - box.height * 0.02,
                 box.width * 1.16, box.height * 1.16])
#ax.legend(loc='upper center', bbox_to_anchor=(0.45, -0.2),
#          fancybox=True, shadow=True, ncol=4, prop={'size': 5})

plt.title("")
plt.savefig("plot_throughput_" + title + ".png", dpi=150)

