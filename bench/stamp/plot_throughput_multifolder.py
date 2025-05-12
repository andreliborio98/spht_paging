#!/usr/bin/env python3

import sys
import matplotlib.pyplot as plt
import csv
import numpy as np

bench = [
    "GENOME",
    "INTRUDER",
    "KMEANS_LOW",
    "KMEANS_VLOW",
    "LABYRINTH",
    "SSCA2",
    "VACATION_LOW",
    "YADA",
]
folder1 = "2024-05-06T09-54-10wreplayer/"
folder2 = "2024-05-06T09-53-03woreplayer/"

i = 7  # CHANGE MANUALLY OR SELECT BENCH, FOR LOOP DOESNT WORK FOR SOME REASON (accumulates line graphs)
# for i in range(0, len(bench)):
imp = [
    bench[i],
    folder1 + bench[i] + "_usePCWM_avg.tsv",
    folder1 + bench[i] + "_usePCWM_stdev.tsv",
    folder2 + bench[i] + "_usePCWM_avg.tsv",
    folder2 + bench[i] + "_usePCWM_stdev.tsv",
]
title = imp[0]
files = imp[1:]
titles = [t[-1] for t in [f.split("_") for f in files]]
avgstring = "avg"
stdevstring = "stdev"

for j in range(0, len(files)):
    if avgstring in files[j]:
        x, y = [], []
        yerr = []
        with open(files[j], "r") as avgFile:
            with open(files[j + 1], "r") as stdevFile:
                avgs = csv.reader(avgFile, delimiter="\t")
                stdevs = csv.reader(stdevFile, delimiter="\t")
                next(avgs)
                next(stdevs)
                for avg, stdev in zip(avgs, stdevs):
                    x.append(int(avg[0]))
                    if folder2 in files[j]:
                        y.append(float(avg[13]) / 1.0e3)
                        yerr.append(float(stdev[13]) / 1.0e3)
                    else:
                        y.append(float(avg[15]) / 1.0e3)
                        yerr.append(float(stdev[15]) / 1.0e3)

            # x = [1, 2, 4, 8, 12, 16, 20, 24]  # ,28,32,36,40,44,48]

            # if "usePCWMeADR_" in files[i]:
            #     plt.plot(x, y, label="SPHT-eADR")  # "PCWMeADR")
            # if "usePCWMeADRT1_" in files[i]:
            #     plt.plot(x, y, label="Smart Close")  # "PCWMeADR+Flush")
            # if "useSharedHTM_" in files[i]:
            #     plt.plot(x, y, label="HTM Only (Shared)")  # useSharedHTM
            if "usePCWM_" and "worep" in files[j]:
                plt.errorbar(
                    x, y, yerr=yerr, label=bench[i] + " Without Replayer"
                )  # "PCWM")
            if "usePCWM_" and "wrep" in files[j]:
                plt.errorbar(
                    x, y, yerr=yerr, label=bench[i] + " With Replayer"
                )  # "PCWM")

    else:
        j = j + 1

plt.xlabel("Threads")
plt.ylabel("Throughput (kTXs/s)")
# ax = plt.subplot(111)
# box = ax.get_position()
# ax.set_position([box.x0, box.y0 + box.height * 0.1, box.width, box.height * 0.9])
# ax.legend(
#     loc="upper center",
#     bbox_to_anchor=(0.5, -0.15),
#     fancybox=True,
#     shadow=True,
#     ncol=2,
# )
plt.legend()
plt.grid()
plt.title(title)
print(title)
plt.savefig("plot_throughput_multifolder_" + title + ".png", dpi=150)
