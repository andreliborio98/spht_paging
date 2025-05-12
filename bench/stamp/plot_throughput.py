#!/usr/bin/env python3

import sys
import matplotlib.pyplot as plt
import csv
import numpy as np

REPLAYER = 1
title = sys.argv[1]
files = sys.argv[2:]
titles = [t[-1] for t in [f.split("_") for f in files]]

for i in range(0, len(files), 2):
    x = []
    y = []
    yerr = []
    with open(files[i], "r") as avgFile:
        with open(files[i + 1], "r") as stdevFile:
            avgs = csv.reader(avgFile, delimiter="\t")
            stdevs = csv.reader(stdevFile, delimiter="\t")
            next(avgs)
            next(stdevs)
            for avg, stdev in zip(avgs, stdevs):
                x.append(int(avg[0]))
                if REPLAYER == 0:
                    y.append(float(avg[13]) / 1.0e3)
                    yerr.append(float(stdev[13]) / 1.0e3)
                if REPLAYER == 1:
                    y.append(float(avg[18]) / 1.0e3)
                    yerr.append(float(stdev[18]) / 1.0e3)

        if "usePCWM_" in files[i]:
            plt.errorbar(x, y, yerr=yerr, label="SPHT")  # "PCWM")
        if "usePCWMeADR_" in files[i]:
            plt.errorbar(x, y, yerr=yerr, label="SPHT-eADR")  # "PCWMeADR")
        if "usePCWMeADRT1_" in files[i]:
            plt.errorbar(x, y, yerr=yerr, label="Smart Close")  # "PCWMeADR+Flush")
        if "useSharedHTM_" in files[i]:
            plt.errorbar(x, y, yerr=yerr, label="Shared")  # useSharedHTM
        plt.xlabel("Threads")
        plt.ylabel("Throughput (kTXs/s)")
        plt.legend()
plt.title(title)
plt.savefig("plot_throughput_" + title + ".png", dpi=150)

# avgstring = "avg"
# stdevstring = "stdev"
# flagstd = 0
# flagavg = 0
# for i in range(0, len(files)):
#     x, y, e = [], [], []
#     if stdevstring in files[i]:
#         flagstd = 1
#         with open(files[i], "r") as stdevFile:
#             errs = csv.reader(stdevFile, delimiter="\t")
#             next(errs)
#             for err in errs:
#                 if REPLAYER == 0:
#                     e.append(float(err[13]) / 1.0e3)
#                 if REPLAYER == 1:
#                     e.append(float(err[15]) / 1.0e3)
#     if avgstring in files[i]:
#         flagavg = 1
#         with open(files[i], "r") as avgFile:
#             avgs = csv.reader(avgFile, delimiter="\t")
#             next(avgs)
#             for avg in avgs:
#                 if REPLAYER == 0:
#                     y.append(float(avg[13]) / 1.0e3)
#                 if REPLAYER == 1:
#                     y.append(float(avg[15]) / 1.0e3)

#         print(y)
#         print(e)
#         x = [1, 2, 4, 8, 12, 16, 20, 24]  # ,28,32,36,40,44,48]

#         if "usePCWM_" in files[i]:
#             plt.plot(x, y, label="SPHT")  # "PCWM")
#             # plt.errorbar(x, y, e, linestyle="None", marker="^")
#         if "usePCWMeADR_" in files[i]:
#             plt.plot(x, y, label="SPHT-eADR")  # "PCWMeADR")
#         if "usePCWMeADRT1_" in files[i]:
#             plt.plot(x, y, label="Smart Close")  # "PCWMeADR+Flush")
#         if "useSharedHTM_" in files[i]:
#             plt.plot(x, y, label="HTM Only (Shared)")  # useSharedHTM

#         plt.xlabel("Threads")
#         plt.ylabel("Throughput (kTXs/s)")
#         plt.legend()
#         plt.grid()

#     if flagstd == 1 or flagavg == 1:
#         flagstd = 0
#         flagavg = 0
#         i = i + 1

# plt.title(title)
# plt.savefig("plot_throughput_" + title + ".png", dpi=150)
