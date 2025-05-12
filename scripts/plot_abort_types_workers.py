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

fig = plt.figure(figsize=(10,4)) #figsize=(6,4)

ax = plt.subplot(1, 1, 1)
patterns = ('---', '...', 'xxx', '***', '///', 'ooo', 'OOO', '+++')

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
    conflict = np.array([])
    capacity = np.array([])
    explicit = np.array([])
    other = np.array([])
    htm_commits_err = []
    sgl_commits_err = []
    conflict_err = []
    capacity_err = []
    explicit_err = []
    other_err = []
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
                conflict = np.append(conflict, float(avg[5]))
                conflict_err = np.append(conflict_err, float(stdev[5]))
                capacity = np.append(capacity, float(avg[6]))
                capacity_err = np.append(capacity_err, float(stdev[6]))
                explicit = np.append(explicit, float(avg[7]))
                explicit_err = np.append(explicit_err, float(stdev[7]))
                other = np.append(other, float(avg[8]))
                other_err = np.append(other_err, float(stdev[8]))

            # all_cases = htm_commits + sgl_commits + aborts
            # htm_commits = (htm_commits) / all_cases
            # htm_commits_err = (htm_commits_err) / all_cases
            # sgl_commits = sgl_commits / all_cases
            # sgl_commits_err = sgl_commits_err / all_cases
            # aborts = aborts / all_cases
            # aborts_err = aborts_err / all_cases
            conflict = conflict / aborts
            conflict_err = conflict_err / aborts
            capacity = capacity / aborts
            capacity_err = capacity_err / aborts
            explicit = explicit / aborts
            explicit_err = explicit_err / aborts
            other = other / aborts
            other_err = other_err / aborts

            titleAxis = map_title(titles[i])

            ind = np.arange(len(x))
            ind = np.array(ind) + 0.103 * i

            ax.bar(ind, conflict, width, bottom=capacity+explicit+other, yerr=conflict_err, label="Conflict (" + titleAxis + ")",
                edgecolor='black', hatch=patterns[int(int(i)/2)])
            ax.bar(ind, capacity, width, bottom=explicit+other, yerr=capacity_err, label="Capacity (" + titleAxis + ")")
            ax.bar(ind, explicit, width, bottom=other, yerr=explicit_err, label="Explicit (" + titleAxis + ")")
            if (i == 0):
                ax.bar(ind, other, width, yerr=other_err, label="Other (" + titleAxis + ")", hatch=patterns[int(int(i)/2)],
                    edgecolor='black', color='white')
            else:
                ax.bar(ind, other, width, yerr=other_err, hatch=patterns[int(int(i)/2)],
                    edgecolor='black', color='white')

plt.ylim(top=1.1)

plt.gca().xaxis.grid(True, linestyle="--", linewidth=0.2)
plt.gca().yaxis.grid(True, linestyle="--", linewidth=0.2)

plt.margins(0.01, 0.01)
plt.xticks(ind, x)
plt.xlabel('Number of threads', size=14, labelpad=0)
plt.ylabel('Normalized abort types', size=14)
plt.title(str(title))
ax.yaxis.set_label_coords(-0.08, 0.35)

plt.xticks(ind, x, size=12)

yticks1 = [0.0, 0.2, 0.4, 0.6, 0.8, 1.0]
plt.yticks(yticks1, yticks1, size=12)

box = ax.get_position()
#ax.set_position([box.x0 + box.width * 0.02, box.y0 + box.height * 0.35,
 #                box.width * 1.1, box.height * 0.8])
ax.legend(loc='upper center', bbox_to_anchor=(0.42, -0.29),
          fancybox=True, shadow=True, ncol=4, handletextpad=0.15, columnspacing=0.3, prop={'size': 9})

plt.savefig("plot_abort_types" + title + ".pdf", dpi=250)

