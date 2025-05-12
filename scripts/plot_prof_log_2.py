#!/usr/bin/env python3

import sys
import matplotlib.pyplot as plt
import csv
import numpy as np

from plot_map_title import map_title

title = sys.argv[1]
files = sys.argv[2:]
titles = [t[-2] for t in [f.split('_') for f in files]]
width = 0.16

fig, axs = plt.subplots(4, 1, figsize=(7,12), sharex=True,
    gridspec_kw={'hspace': 0.5, 'wspace': 0})

patterns = ('---', '...', 'xxx', '***', '///', 'ooo', 'OOO', '+++')

# first 2 files are the normalization factor
norm_total_time = np.array([])
norm_sorter_time = np.array([])
norm_apply_time = np.array([])
norm_flush_time = np.array([])
x = []

print("Norm avg file = " + files[0] + ", Norm std file = " + files[1])
with open(files[0], 'r') as avgFile:
    with open(files[1], 'r') as stdevFile:
        avgs = csv.reader(avgFile, delimiter='\t')
        stdevs = csv.reader(stdevFile, delimiter='\t')
        next(avgs)
        next(stdevs)
        for avg, stdev in zip(avgs, stdevs):
            x.append(int(avg[0]))
            norm_total_time = np.append(norm_total_time, float(avg[5]))
            norm_apply_time = np.append(norm_apply_time, float(avg[6]))
            norm_sorter_time = np.append(norm_sorter_time, float(avg[7]))
            norm_flush_time = np.append(norm_flush_time, float(avg[8]))

for i in range(2, len(files), 2):
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
    with open(files[i], 'r') as avgFile:
        with open(files[i+1], 'r') as stdevFile:
            avgs = csv.reader(avgFile, delimiter='\t')
            stdevs = csv.reader(stdevFile, delimiter='\t')
            next(avgs)
            next(stdevs)
            for avg, stdev in zip(avgs, stdevs):
                x.append(int(avg[0]))

                total_time = np.append(total_time, float(avg[5]))
                total_time_err = np.append(total_time_err, float(stdev[5]))

                apply_time = np.append(apply_time, float(avg[6]))
                apply_time_err = np.append(apply_time_err, float(stdev[6]))

                sorter_time = np.append(sorter_time, float(avg[7]))
                sorter_time_err = np.append(sorter_time_err, float(stdev[7]))

                flush_time = np.append(flush_time, float(avg[8]))
                flush_time_err = np.append(flush_time_err, float(stdev[8]))

            total_time_f = total_time / norm_total_time
            total_time_err_f = total_time_err / norm_total_time

            apply_time_f = apply_time / norm_apply_time
            apply_time_err_f = apply_time_err / norm_apply_time

            print("sorter_time = " + str(sorter_time[12]) + " vs " + str(norm_sorter_time[12]))
            sorter_time_f = sorter_time / norm_sorter_time
            sorter_time_err_f = sorter_time_err / norm_sorter_time

            flush_time_f = flush_time / norm_flush_time
            flush_time_err_f = flush_time_err / norm_flush_time

            ind = np.arange(len(x))
            ind = np.array(ind) + 0.10 * i

            titleLabel = map_title(titles[int(i)])

            ### Total
            # ax.title("Total time")
            axs[0].bar(ind, total_time_f, width, yerr=total_time_err_f, label=titleLabel,
                edgecolor='black', hatch=patterns[int(int(i)/2)])
            axs[0].set(title="total")

            ### Sorting
            # ax.title("Sorting")
            axs[1].bar(ind, sorter_time_f, width, yerr=sorter_time_err_f,
                edgecolor='black', hatch=patterns[int(int(i)/2)])
            axs[1].set(title="sorting", ylabel="normalized to " + map_title(titles[0]))

            ### Apply
            # ax.title("Apply")
            axs[2].bar(ind, apply_time_f, width, yerr=apply_time_err_f,
                edgecolor='black', hatch=patterns[int(int(i)/2)])
            axs[2].set(title="apply")

            ### Flush
            # ax = plt.subplot(4, 1, 4)
            # ax.title("Flush")
            axs[3].bar(ind, flush_time_f, width, yerr=flush_time_err_f,
                edgecolor='black', hatch=patterns[int(int(i)/2)])
            axs[3].set(title="flush") # , yscale="symlog"
            # axs[3].yscale("log")

plt.xlabel('Threads')
plt.margins(0.01, 0.0)
plt.xticks(ind, x)

for ax in axs:
    box = ax.get_position()
    ax.set_position([box.x0, box.y0,
                    box.width * 1.1, box.height * 1.2])
axs[0].legend(loc='upper center', bbox_to_anchor=(0.45, 1.5),
          fancybox=True, shadow=True, ncol=3, prop={'size': 8})

plt.savefig("plot_log_prof_" + title + ".png", dpi=150)

