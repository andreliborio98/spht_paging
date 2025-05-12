#!/usr/bin/env python3

import sys
import matplotlib.pyplot as plt
import csv
import numpy as np

from plot_map_title import map_title

title = sys.argv[1]
files = sys.argv[2:]
titles = [t[-2] for t in [f.split('_') for f in files]]
width = 0.38

fig, axs = plt.subplots(2, 1, figsize=(7,3), sharex=True,
    gridspec_kw={'hspace': 0.12, 'wspace': 0})

patterns = ('---', '...', 'xxx', '***', '///', 'ooo', 'OOO', '+++')

# first 2 files are the normalization factor
norm_total_time = np.array([])
norm_sorter_time = np.array([])
norm_apply_time = np.array([])
norm_flush_time = np.array([])
x = []

for i in range(0, len(files), 2):
    x = []
    total_time = np.array([])
    apply_time = np.array([])
    sorter_time = np.array([])
    flush_time = np.array([])

    total_time_err = np.array([])
    apply_time_err = np.array([])
    sorter_time_err = np.array([])
    flush_time_err = np.array([])
    ### TODO: it creates a hole in the plot
    # if titles[int(i)] == "useFastPCWC" or titles[int(i)] == "useLogicalClocks":
    #     continue
    print("avg file = " + files[i] + ", std file = " + files[i+1])
    with open("log_" + files[i], 'r') as avgFile:
        with open("log_" + files[i+1], 'r') as stdevFile:
            with open("param_" + files[i], 'r') as paramFile:
                avgs = csv.reader(avgFile, delimiter='\t')
                stdevs = csv.reader(stdevFile, delimiter='\t')
                params = csv.reader(paramFile, delimiter='\t')
                next(avgs)
                next(stdevs)
                next(params)
                for avg, stdev, param in zip(avgs, stdevs, params):
                    x.append(int(int(param[0]) / 1048576))

                    total_time = np.append(total_time, float(avg[5]))
                    total_time_err = np.append(total_time_err, float(stdev[5]))

                    apply_time = np.append(apply_time, float(avg[6]))
                    apply_time_err = np.append(apply_time_err, float(stdev[6]))

                    sorter_time = np.append(sorter_time, float(avg[7]))
                    sorter_time_err = np.append(sorter_time_err, float(stdev[7]))

                    flush_time = np.append(flush_time, float(avg[8]))
                    flush_time_err = np.append(flush_time_err, float(stdev[8]))

                ind = np.arange(len(x))
                ind = np.array(ind) + 0.19 * i

                if "WBINVD" in titles[int(i)]:
                    titleLabel = "WBINVD"
                else:
                    titleLabel = "CLWB range"

                ### Flush
                # ax.title("flush")
                axs[0].bar(ind, flush_time, width, yerr=flush_time_err, label=titleLabel,
                    edgecolor='black', hatch=patterns[int(int(i)/2)])
                axs[1].bar(ind, flush_time, width, yerr=flush_time_err, label=titleLabel,
                    edgecolor='black', hatch=patterns[int(int(i)/2)])
                # axs[1].set(ylabel="flush time (cycles)")

axs[0].set_ylim(3e7, 1.6e8)
axs[1].set_ylim(0.0, 3e7)

axs[0].spines['bottom'].set_visible(False)
axs[1].spines['top'].set_visible(False)
axs[0].xaxis.tick_top()
axs[0].tick_params(labeltop=False)  # don't put tick labels at the top
axs[1].xaxis.tick_bottom()
axs[1].tick_params(labelsize=12)

d = .005  # how big to make the diagonal lines in axes coordinates
# arguments to pass to plot, just so we don't keep repeating them
kwargs = dict(transform=axs[0].transAxes, color='k', clip_on=False)
axs[0].plot((-d, +d), (-d, +d), **kwargs)        # top-left diagonal
axs[0].plot((1 - d, 1 + d), (-d, +d), **kwargs)  # top-right diagonal

kwargs.update(transform=axs[1].transAxes)  # switch to the bottom axes
axs[1].plot((-d, +d), (1 - d, 1 + d), **kwargs)  # bottom-left diagonal
axs[1].plot((1 - d, 1 + d), (1 - d, 1 + d), **kwargs)  # bottom-right diagonal


plt.xlabel('Heap Size (MB)', size=14)
plt.ylabel('Flush (cycles)', size=14)
plt.margins(0.01, 0.0)
plt.xticks(ind, x)

for ax in axs:
    box = ax.get_position()
    ax.set_position([box.x0 - box.width * 0.06, box.y0 + box.height * 0.19,
                    box.width * 1.16, box.height * 0.97])
axs[0].legend(loc='upper center', bbox_to_anchor=(0.45, 1.0),
          fancybox=True, shadow=True, ncol=3, prop={'size': 14})

plt.savefig("plot_flush_" + title + ".pdf", dpi=150)

