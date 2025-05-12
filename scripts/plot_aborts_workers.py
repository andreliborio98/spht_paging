#!/usr/bin/env python3

import sys
import matplotlib.pyplot as plt
import csv
import numpy as np
from matplotlib.patches import Ellipse, Polygon

from plot_map_title import map_title

title = sys.argv[1]
files = sys.argv[2:]
titles = [t[-2] for t in [f.split('_') for f in files]]
width = 0.20

fig = plt.figure(figsize=(10,4))

ax = plt.subplot(1, 1, 1)
patterns = ('---', '...', 'xxx', '***', '///', 'ooo', 'OOO', '+++')

# title_map = {
#     "PhysicalClocks":"Phys Clocks",
#     "LogicalClocks":"DudeTM",
#     "PCWC-F":"Phys+Logic Clocks (w/ F)",
#     "PCWC-NF":"Phys+Logic Clocks (w/o F)",
#     "PCWM2":"Phys Clocks w/ M (log opt)",
#     "PCWM":"Phys Clocks w/ M"
# }

new_files = []
for i in range(0, len(files)):
    if "PSTM" in files[i]:
        continue
    new_files += [files[i]]

files = new_files

for i in range(0, len(files), 2):
    x = []
    labels = []
    all_cases = np.array([])
    htm_commits = np.array([])
    sgl_commits = np.array([])
    aborts = np.array([])
    #conflicts = []
    #capacity = []
    htm_commits_err = []
    sgl_commits_err = []
    aborts_err = []
    with open(files[i], 'r') as avgFile:
        with open(files[i+1], 'r') as stdevFile:
            avgs = csv.reader(avgFile, delimiter='\t')
            stdevs = csv.reader(stdevFile, delimiter='\t')
            next(avgs)
            next(stdevs)
            j = 0
            for avg, stdev in zip(avgs, stdevs):
                j = j + 1
                xVal = int(avg[0])
                #if xVal == 6 or xVal == 12 or xVal == 24 or xVal == 40 or xVal == 48 or xVal == 56:
                #    continue
                x.append(int(avg[0]))
                htm_commits = np.append(htm_commits, float(avg[2]))
                htm_commits_err = np.append(htm_commits_err, float(stdev[2]))
                sgl_commits = np.append(sgl_commits, float(avg[3]))
                sgl_commits_err = np.append(sgl_commits_err, float(stdev[3]))
                aborts = np.append(aborts, float(avg[4]))
                aborts_err = np.append(aborts_err, float(stdev[4]))

            all_cases = htm_commits + sgl_commits + aborts
            htm_commits = (htm_commits) / all_cases
            htm_commits_err = (htm_commits_err) / all_cases
            sgl_commits = sgl_commits / all_cases
            sgl_commits_err = sgl_commits_err / all_cases
            aborts = aborts / all_cases
            aborts_err = aborts_err / all_cases

            titleAxis = map_title(titles[i])

            ind = np.arange(len(x))
            ind = np.array(ind) + 0.103 * i

            ax.bar(ind, htm_commits, width, bottom=aborts+sgl_commits, yerr=htm_commits_err, label="HTM (" + titleAxis + ")",
                edgecolor='black', hatch=patterns[int(int(i)/2)])
            ax.bar(ind, sgl_commits, width, bottom=aborts, yerr=sgl_commits_err, label="SGL (" + titleAxis + ")")
            if (i == 0):
                ax.bar(ind, aborts, width, yerr=aborts_err, label="Aborts (" + titleAxis + ")", hatch=patterns[int(int(i)/2)],
                    edgecolor='black', color='white')
            else:
                ax.bar(ind, aborts, width, yerr=aborts_err, hatch=patterns[int(int(i)/2)],
                    edgecolor='black', color='white')
            # if "Crafty" in titleAxis:
            #     with open("prof_" + files[i], 'r') as avgFile:
            #         with open("prof_" + files[i+1], 'r') as stdevFile:
            #             avgs = csv.reader(avgFile, delimiter='\t')
            #             stdevs = csv.reader(stdevFile, delimiter='\t')

            #             redo_htm_commits = np.array([])
            #             redo_sgl_commits = np.array([])
            #             redo_aborts = np.array([])
            #             redo_htm_commits_err = []
            #             redo_sgl_commits_err = []
            #             redo_aborts_err = []

            #             next(avgs)
            #             next(stdevs)
            #             j = 0
            #             for avg, stdev in zip(avgs, stdevs):
            #                 j = j + 1
            #                 xVal = int(avg[0])
            #                 if xVal == 6 or xVal == 12 or xVal == 24 or xVal == 40 or xVal == 48 or xVal == 56:
            #                     continue
            #                 # if j == 1 or j > 11:
            #                 #     continue
            #                 redo_htm_commits = np.append(redo_htm_commits, float(avg[6]))
            #                 redo_htm_commits_err = np.append(redo_htm_commits_err, float(stdev[6]))
            #                 redo_sgl_commits = np.append(redo_sgl_commits, float(avg[7]))
            #                 redo_sgl_commits_err = np.append(redo_sgl_commits_err, float(stdev[7]))
            #                 redo_aborts = np.append(redo_aborts, float(avg[8]))
            #                 redo_aborts_err = np.append(redo_aborts_err, float(stdev[8]))

            #             all_cases = redo_htm_commits + redo_sgl_commits + redo_aborts
            #             redo_htm_commits = (redo_htm_commits + redo_sgl_commits) / all_cases
            #             redo_htm_commits_err = (redo_htm_commits_err + redo_sgl_commits_err) / all_cases
            #             redo_aborts = (redo_aborts) / all_cases
            #             redo_aborts_err = (redo_aborts_err) / all_cases

            #             ind = np.arange(len(x))
            #             ind = np.array(ind) + 0.103 * (i + 6)

            #             ax.bar(ind, redo_htm_commits, width,
            #                 bottom=redo_aborts, yerr=redo_htm_commits_err,
            #                 label="Commits (" + titleAxis + ", REDO)",
            #                 edgecolor='black', hatch=patterns[int(int(i)/2)])
            #             #ax.bar(ind, sgl_commits, width, bottom=aborts, yerr=htm_commits_err, label="SGL (" + titles[int(i)] + ")")
            #             ax.bar(ind, redo_aborts, width, yerr=redo_aborts_err,
            #                 label="Aborts (" + titleAxis + ", REDO)", hatch=patterns[int(int(i)/2)],
            #                 edgecolor='black', color='white')

            # ax.fill([1, 3, 3, 1], [1, 1, 2, 2], fill=False, hatch='\\')

            # plt.gca().set_prop_cycle(None)

plt.ylim(top=1.1)

plt.gca().xaxis.grid(True, linestyle="--", linewidth=0.2)
plt.gca().yaxis.grid(True, linestyle="--", linewidth=0.2)

plt.margins(0.01, 0.01)
plt.xticks(ind, x)
plt.xlabel('Number of threads', size=14, labelpad=0)
plt.ylabel('Normalized \n commits and aborts', size=14)
plt.title(str(title))
ax.yaxis.set_label_coords(-0.08, 0.35)

plt.xticks(ind, x, size=12)

yticks1 = [0.0, 0.2, 0.4, 0.6, 0.8, 1.0]
plt.yticks(yticks1, yticks1, size=12)

box = ax.get_position()
#ax.set_position([box.x0 + box.width * 0.02, box.y0 + box.height * 0.35,
#                 box.width * 1.1, box.height * 0.8])
ax.legend(loc='upper center', bbox_to_anchor=(0.42, -0.29),
          fancybox=True, shadow=True, ncol=4, handletextpad=0.15, columnspacing=0.3, prop={'size': 9})

plt.savefig("plot_aborts_" + title + ".pdf", dpi=250)

