#!/usr/bin/env python3

import sys
import matplotlib.pyplot as plt
import csv
import numpy as np

from plot_map_title import map_title

title = sys.argv[1]
files = sys.argv[2:]
titles = [t[-2] for t in [f.split('_') for f in files]]
width = 0.20

fig, axs = plt.subplots(2, 1, figsize=(7,4), sharex=True,
    gridspec_kw={'hspace': 0.12, 'wspace': 0})
patterns = ('---', '...', 'xxx', '***', '///', 'ooo', 'OOO', '+++')

for i in range(0, len(files), 2):
    x = []
    nbThreads = np.array([])
    nbCommits = np.array([])
    total_time = np.array([])
    after_tx_time = np.array([])
    tx_time = np.array([])
    wait_time = np.array([])
    total_time_err = np.array([])
    after_tx_time_err = np.array([])
    tx_time_err = np.array([])
    wait_time_err = np.array([])
    scan_time = np.array([])
    scan_time_err = np.array([])
    flush_time = np.array([])
    flush_time_err = np.array([])
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
                nbThreads = np.append(nbThreads, float(avg[0]))
                nbCommits = np.append(nbCommits, float(avg[1]))

                total_time = np.append(total_time, float(avg[2]))
                total_time_err = np.append(total_time_err, float(stdev[2]))

                after_tx_time = np.append(after_tx_time, float(avg[3]))
                after_tx_time_err = np.append(after_tx_time_err, float(stdev[3]))

                wait_time = np.append(wait_time, float(avg[5]))
                wait_time_err = np.append(wait_time_err, float(stdev[5]))

                if "useCcHTM" in titles[int(i)] or "useCrafty" in titles[int(i)]:
                    flush_time = after_tx_time - wait_time
                    flush_time_err = after_tx_time

                if "useLogi" in titles[int(i)] or "usePhys" in titles[int(i)]:
                    tx_time = total_time - after_tx_time
                    tx_time_err = total_time_err
                    flush_time = after_tx_time - wait_time
                    flush_time_err = after_tx_time_err
                else:
                    tx_time = np.append(tx_time, float(avg[4]))
                    tx_time_err = np.append(tx_time_err, float(stdev[4]))


                if "usePCWM" in titles[int(i)] or "pcwm" in titles[int(i)]:
                    flush_time = np.append(flush_time, float(avg[6]))
                    flush_time_err = np.append(flush_time_err, float(stdev[6]))

                if ("usePCWM2" in titles[int(i)] or "pcwm2" in titles[int(i)] or
                        "usePCWM3" in titles[int(i)] or "pcwm3" in titles[int(i)]):
                    scan_time = np.append(scan_time, float(avg[7]))
                    scan_time_err = np.append(scan_time_err, float(stdev[7]))

            if ("usePCWM2" in titles[int(i)] or "pcwm2" in titles[int(i)] or
                    "usePCWM3" in titles[int(i)] or "pcwm3" in titles[int(i)]):
                flush_time -= scan_time # is contained

                scan_time_f = scan_time / nbCommits
                scan_time_err_f = scan_time_err / nbCommits


            wait_time_f = wait_time / nbCommits #(2.3e9 * nbThreads)
            wait_time_err_f = wait_time_err / nbCommits #(2.3e9 * nbThreads)

            after_tx_time_f = (after_tx_time - wait_time) / nbCommits #(2.3e9 * nbThreads)
            after_tx_time_err_f = (after_tx_time_err) / nbCommits #(2.3e9 * nbThreads)

            tx_time_f = tx_time / nbCommits #(2.3e9 * nbThreads)
            tx_time_err_f = (tx_time_err) / nbCommits #(2.3e9 * nbThreads)

            flush_time_f = flush_time / nbCommits #(2.3e9 * nbThreads)
            flush_time_err_f = flush_time_err / nbCommits #(2.3e9 * nbThreads)

            if "usePCWM" in titles[int(i)] or "pcwm" in titles[int(i)]:
                after_tx_time_f = (after_tx_time - wait_time - flush_time) / nbCommits #(2.3e9 * nbThreads)
                after_tx_time_err_f = (after_tx_time_err) / nbCommits #(2.3e9 * nbThreads)

            if "usePCWM2" in titles[int(i)] or "pcwm2" in titles[int(i)] or "usePCWM3" in titles[int(i)] or "pcwm3" in titles[int(i)]:
                after_tx_time_f = (after_tx_time - wait_time - flush_time - scan_time) / nbCommits #(2.3e9 * nbThreads)
                after_tx_time_err_f = (after_tx_time_err) / nbCommits #(2.3e9 * nbThreads)

            ind = np.arange(len(x))
            ind = np.array(ind) + 0.11 * i

            titleAxis = map_title(titles[i])

            for ax in axs:
                if "usePCWM2" in titles[int(i)] or "pcwm2" in titles[int(i)] or "usePCWM3" in titles[int(i)] or "pcwm3" in titles[int(i)]:
                    ax.bar(ind, scan_time_f, width, bottom=tx_time_f+wait_time_f+flush_time_f+after_tx_time_f, yerr=scan_time_err_f,
                        label="Scan (" + titleAxis + ")", hatch=patterns[int(int(i)/2)])
                if "usePCWM" in titles[int(i)]:
                    ax.bar(ind, after_tx_time_f, width, bottom=tx_time_f+wait_time_f, label="Marker (" + titleAxis + ")",
                        edgecolor='black', hatch=patterns[int(int(i)/2)])
                ax.bar(ind, flush_time_f, width, bottom=tx_time_f+wait_time_f+after_tx_time_f, yerr=flush_time_err_f,
                    label="Flush (" + titleAxis + ")")
                ax.bar(ind, wait_time_f, width, bottom=tx_time_f, yerr=wait_time_err_f, label="Wait (" + titleAxis + ")",
                    edgecolor='black', hatch=patterns[int(int(i)/2)])
                ax.bar(ind, tx_time_f, width, yerr=tx_time_err_f, label="TX (" + titleAxis + ")",
                    edgecolor='black', color='white', hatch=patterns[int(int(i)/2)])

            # plt.gca().set_prop_cycle(None)

ax = axs
axs[0].set_ylim(20000, 40000)
axs[1].set_ylim(0.0, 20000)

yticks1        = [0, 5000, 10000, 15000, 20000]
yticks_labels1 = [0, 5, 10, 15, 20]
yticks2        = [20000, 30000, 40000]
yticks_labels2 = ["", 30, 40]

axs[0].spines['bottom'].set_visible(False)
axs[1].spines['top'].set_visible(False)
axs[0].xaxis.tick_top()
axs[0].tick_params(labeltop=False)  # don't put tick labels at the top
axs[1].xaxis.tick_bottom()
# axs[1].tick_params(labelsize=9)

d = .005  # how big to make the diagonal lines in axes coordinates
# arguments to pass to plot, just so we don't keep repeating them
kwargs = dict(transform=axs[0].transAxes, color='k', clip_on=False)
axs[0].plot((-d, +d), (-d, +d), **kwargs)        # top-left diagonal
axs[0].plot((1 - d, 1 + d), (-d, +d), **kwargs)  # top-right diagonal

kwargs.update(transform=axs[1].transAxes)  # switch to the bottom axes
axs[1].plot((-d, +d), (1 - d, 1 + d), **kwargs)  # bottom-left diagonal
axs[1].plot((1 - d, 1 + d), (1 - d, 1 + d), **kwargs)  # bottom-right diagonal

plt.xlabel('Number of threads')
plt.ylabel('                  Thousand cycles per Transaction')

plt.margins(0.01, 0.01)

axs[1].legend(loc='upper center', bbox_to_anchor=(0.46, -0.18),
        fancybox=True, shadow=True, ncol=4, prop={'size': 7.9})

for ax in axs:
    ax.xaxis.grid(True, linestyle="--", linewidth=0.2)
    ax.yaxis.grid(True, linestyle="--", linewidth=0.2)

ax = axs[0]
box = ax.get_position()
ax.set_position([box.x0 - box.width * 0.06, box.y0 + box.height * 0.88,
                box.width * 1.18, box.height * 0.4])
plt.sca(ax)
plt.yticks(yticks2, yticks_labels2)

ax = axs[1]
box = ax.get_position()
ax.set_position([box.x0 - box.width * 0.06, box.y0 + box.height * 0.5,
                box.width * 1.18, box.height * 1.46])

plt.sca(ax)
plt.yticks(yticks1, yticks_labels1)
plt.xticks([a for a in range(len(x))], x)

plt.title("")
plt.savefig("plot_prof_" + title + ".pdf", dpi=250)
