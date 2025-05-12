#!/usr/bin/env python3

import sys
import matplotlib.pyplot as plt
import csv
import numpy as np

title = sys.argv[1]
files = sys.argv[2:]
titles = [t[-2] for t in [f.split('_') for f in files]]
width = 0.12

fig = plt.figure(figsize=(7,3))

ax = plt.subplot(1, 1, 1)
patterns = ('---', '...', 'xxx', '***', '///', 'ooo', 'OOO', '+++')

title_map = {
    "PhysicalClocks":"Phys Clocks",
    "LogicalClocks":"Logi Clocks",
    "PCWC-F":"Phys+Logic Clocks (w/ F)",
    "PCWC-NF":"Phys+Logic Clocks (w/o F)",
    "PCWM2":"Phys Clocks w/ M (log opt)",
    "PCWM":"Phys Clocks w/ M"
}

for i in range(0, len(files), 2):
    x = []
    total_time = np.array([])
    bytes_log = np.array([])

    total_time_err = np.array([])
    bytes_log_err = np.array([])
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

                total_time = np.append(total_time, float(avg[5]))
                total_time_err = np.append(total_time_err, float(stdev[5]))

                bytes_log = np.append(bytes_log, float(avg[2]))
                bytes_log_err = np.append(bytes_log_err, float(stdev[2]))

            total_time /= 2.3e9
            total_time_err /= 2.3e9

            bytes_log /= 1e6
            bytes_log_err /= 1e6

            bytes_log *= 8
            bytes_log_err *= 8

            throughput_f = bytes_log / total_time
            throughput_err_f = bytes_log_err / total_time 

            titleAxis = titles[i]
            for a in title_map:
                if a in titles[i]:
                    titleAxis = title_map[a]
                    break

            ind = np.arange(len(x))
            ind = np.array(ind) + 0.07 * i
            ax.bar(ind, throughput_f, width, yerr=throughput_err_f, label=titleAxis,
                hatch=patterns[int(int(i)/2)])

plt.xticks(ind, x)
plt.xlabel('Threads')
plt.ylabel('Apply log (MB/s)')

box = ax.get_position()
ax.set_position([box.x0, box.y0 + box.height * 0.2,
                 box.width * 1.1, box.height * 0.8])
ax.legend(loc='upper center', bbox_to_anchor=(0.45, -0.2),
          fancybox=True, shadow=True, ncol=3, prop={'size': 5})

plt.title(title)
plt.savefig("plot_log_bytes_" + title + ".png", dpi=150)

