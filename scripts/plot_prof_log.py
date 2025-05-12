#!/usr/bin/env python3

import sys
import matplotlib.pyplot as plt
import csv
import numpy as np

from plot_map_title import map_title

title = sys.argv[1]
files = sys.argv[2:]
titles = [t[-2] for t in [f.split('_') for f in files]]
width = 0.12

fig = plt.figure(figsize=(7,5))

ax = plt.subplot(1, 1, 1)
# fig.set(figsize=(7,3))

patterns = ('---', '...', 'xxx', '***', '///', 'ooo', 'OOO', '+++', 'XXX', '\\\\\\')

for i in range(0, len(files), 2):
    x = []
    total_time = np.array([])
    sorter_time = np.array([])
    apply_time = np.array([])
    flush_time = np.array([])
    map_time = np.array([])
    set_time = np.array([])

    total_time_err = np.array([])
    sorter_time_err = np.array([])
    apply_time_err = np.array([])
    flush_time_err = np.array([])
    map_time_err = np.array([])
    set_time_err = np.array([])
    ### TODO: it creates a hole in the plot
    # if titles[int(i)] == "useFastPCWC" or titles[int(i)] == "useLogicalClocks":
    #     continue
    with open(files[i], 'r') as avgFile:
        with open(files[i+1], 'r') as stdevFile:
            avgs = csv.reader(avgFile, delimiter='\t')
            stdevs = csv.reader(stdevFile, delimiter='\t')
            next(avgs)
            next(stdevs)
            j = 0
            for avg, stdev in zip(avgs, stdevs):
                j = j + 1
                x.append(int(avg[0]))

                total_time = np.append(total_time, float(avg[6]))
                total_time_err = np.append(total_time_err, float(stdev[6]))

                apply_time = np.append(apply_time, float(avg[7]))
                apply_time_err = np.append(apply_time_err, float(stdev[7]))

                sorter_time = np.append(sorter_time, float(avg[8]))
                sorter_time_err = np.append(sorter_time_err, float(stdev[8]))

                flush_time = np.append(flush_time, float(avg[9]))
                flush_time_err = np.append(flush_time_err, float(stdev[9]))

                map_time = np.append(map_time, float(avg[10]))
                map_time_err = np.append(map_time_err, float(stdev[10]))

                set_time = np.append(set_time, float(avg[11]))
                set_time_err = np.append(set_time_err, float(stdev[11]))

            sorter_time_f = sorter_time #/ total_time
            sorter_time_err_f = sorter_time_err #/ total_time

            apply_time_f = apply_time #/ total_time
            apply_time_err_f = apply_time_err #/ total_time

            flush_time_f = flush_time #/ total_time
            flush_time_err_f = flush_time_err #/ total_time

            barTitle = map_title(titles[int(i)])

            if "REP2" in titles[int(i)]:
                sorter_time_f /= 2
                apply_time_f /= 2
                flush_time_f /= 2
            if "REP3" in titles[int(i)]:
                sorter_time_f /= 3
                apply_time_f /= 3
                flush_time_f /= 3
            if "REP4" in titles[int(i)]:
                sorter_time_f /= 4
                apply_time_f /= 4
                flush_time_f /= 4
            if "REP8" in titles[int(i)]:
                sorter_time_f /= 8
                apply_time_f /= 8
                flush_time_f /= 8

            ind = np.arange(len(x))
            ind = np.array(ind) + 0.08 * i
            ax.bar(ind, flush_time_f, width, bottom=apply_time_f+sorter_time_f, hatch=patterns[int(i/2)],
                yerr=flush_time_err_f, label="flush (" + barTitle + ")")
            ax.bar(ind, sorter_time_f, width, bottom=apply_time_f, hatch=patterns[int(i/2)],
                yerr=sorter_time_err_f, label="sorter (" + barTitle + ")")

            if "BACKWARD" in titles[int(i)]:
                ax.bar(ind, map_time, width,  bottom=apply_time_f-map_time, edgecolor='black', hatch=patterns[int(i/2)],
                    yerr=map_time_err, label="track (" + barTitle + ")")
                ax.bar(ind, apply_time_f-map_time, width, edgecolor='black', color='white', hatch=patterns[int(i/2)],
                    yerr=apply_time_err_f, label="apply (" + barTitle + ")")
            elif "BUFFER-FLUSHES" in titles[int(i)]:
                ax.bar(ind, set_time, width, bottom=apply_time_f-set_time, edgecolor='black', hatch=patterns[int(i/2)],
                    yerr=set_time_err, label="track (" + barTitle + ")")
                ax.bar(ind, apply_time_f-set_time, width, edgecolor='black', color='white', hatch=patterns[int(i/2)],
                    yerr=apply_time_err_f, label="apply (" + barTitle + ")")
            else:
                ax.bar(ind, apply_time_f, width, edgecolor='black', color='white', hatch=patterns[int(i/2)],
                    yerr=apply_time_err_f, label="apply (" + barTitle + ")")


plt.xticks(ind, x)
plt.xlabel('Threads')
plt.ylabel('Time per replayer (cycles)')

ax.margins(0.01, 0.01)
box = ax.get_position()
ax.set_position([box.x0 - box.width * 0.06, box.y0 + box.height * 0.5,
                 box.width * 1.19, box.height * 0.6])
ax.legend(loc='upper center', bbox_to_anchor=(0.45, -0.3),
          fancybox=True, shadow=True, ncol=2, prop={'size': 6})

plt.title(title)
plt.savefig("plot_log_prof_" + title + ".png", dpi=150)

