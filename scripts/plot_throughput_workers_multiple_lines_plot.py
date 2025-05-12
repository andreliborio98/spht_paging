#!/usr/bin/env python3

import sys
import matplotlib.pyplot as plt
import csv

from plot_map_title import map_title

fig = plt.figure(figsize=(10,6))
ax = plt.subplot(1, 1, 1)

qtt_plot_lines=3

for j in range (0, qtt_plot_lines):
    titles = []
    files = []
    titles.append(sys.argv[j+1])
    files.append(sys.argv[qtt_plot_lines+1+2*j])
    files.append(sys.argv[qtt_plot_lines+2+2*j])
    plotname = sys.argv[qtt_plot_lines+1+qtt_plot_lines*2]
    # print (titles)
    # print (files)

    titles = [t[-2] for t in [f.split('_') for f in files]]
    vals_x = []
    vals_y = []
    vals_yerr = []
    vals_label = []

    for i in range(0, len(files), 2):
        x = []
        y = []
        yerr = []
        with open(files[i], 'r') as avgFile:
            with open(files[(i+1)], 'r') as stdevFile:
                avgs = csv.reader(avgFile, delimiter='\t')
                stdevs = csv.reader(stdevFile, delimiter='\t')
                next(avgs)
                next(stdevs)
                for avg, stdev in zip(avgs, stdevs):
                    x.append(int(avg[0]))
                    y.append(float(avg[1]) / 1.0e6)
                    yerr.append(float(stdev[1]) / 1.0e6)
                titleAxis = map_title(titles[i*j])
                vals_x += [x]
                vals_y += [y]
                vals_yerr += [yerr]
                vals_label += [titles[0]]

    # vals_x = vals_x[2:6] + vals_x[:2] + vals_x[6:9]
    # vals_y = vals_y[2:6] + vals_y[:2] + vals_y[6:9]
    # vals_yerr = vals_yerr[2:6] + vals_yerr[:2] + vals_yerr[6:9]
    # vals_label = vals_label[2:6] + vals_label[:2] + vals_label[6:9]

    for i in range(len(vals_x)):
        x = [x+0.05*(i-4) for x in vals_x[i]]
        y = vals_y[i]
        yerr = vals_yerr[i]
        titleAxis = vals_label[i]
        plt.errorbar(x, y, yerr=yerr, label=titleAxis, linewidth=0.9)

plt.margins(0.01, 0.01)
plt.xlabel('Number of Threads', size=14)
plt.ylabel('Throughput (M TXs/s)', size=14)
plt.title(plotname)
plt.legend(fontsize=10, ncol=1, framealpha=0.4)

# plt.xscale("log")

xticks = [1,2,4,8,16,24] #[1,4,8,12,16,24,32,40,48]
plt.yticks(size=12)
plt.xticks(xticks, xticks, size=12)

plt.gca().xaxis.grid(True, linestyle="--", linewidth=0.2)
plt.gca().yaxis.grid(True, linestyle="--", linewidth=0.2)

box = ax.get_position()
#ax.set_position([box.x0 - box.width * 0.064, box.y0 + box.height * 0.03,
#                 box.width * 1.182, box.height * 1.12])
#ax.legend(loc='upper center', bbox_to_anchor=(0.45, -0.2),
#          fancybox=True, shadow=True, ncol=4, prop={'size': 5})

plt.savefig("ml_plot_throughput_" + plotname + ".pdf", dpi=150)
