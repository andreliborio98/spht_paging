#!/usr/bin/env python3

import sys
import matplotlib.pyplot as plt
import csv
import numpy as np

title = sys.argv[1]
files = sys.argv[2:]
titles = [t[-1] for t in [f.split("_") for f in files]]

# for i in range(0, len(files), 2):
#     x = []
#     y = []
#     yerr = []
#     with open(files[i], 'r') as avgFile:
#         with open(files[i+1], 'r') as stdevFile:
#             avgs = csv.reader(avgFile, delimiter='\t')
#             stdevs = csv.reader(stdevFile, delimiter='\t')
#             next(avgs)
#             next(stdevs)
#             for avg, stdev in zip(avgs, stdevs):
#                 x.append(int(avg[0]))
#                 if "PSTM" in titles[i]:
#                     y.append(float(avg[4]) / 1.0e3)
#                     yerr.append(float(stdev[4]) / 1.0e3)
#                 else:
#                     y.append(float(avg[9]) / 1.0e3)
#                     yerr.append(float(stdev[9]) / 1.0e3)
#             plt.errorbar(x, y, yerr=yerr, label=titles[i])
#             plt.xlabel('Threads')
#             plt.ylabel('Throughput (kTXs/s)')
#             plt.legend()
# plt.title(title)
# plt.savefig("plot_throughput_" + title + ".png", dpi=150)

# FUNCTIONAL VERSION, NO ERROR BAR
avgstring = "avg"
for i in range(0, len(files)):
    if avgstring in files[i]:
        x, y1, y2, r, t = [], [], [], [], []
        with open(files[i], "r") as avgFile:
            avgs = csv.reader(avgFile, delimiter="\t")
            next(avgs)
            for avg in avgs:
                y1.append(float(avg[13]))
                y2.append(float(avg[14]))
                t.append(float(avg[2]))
                r.append(float(avg[15]) / float(avg[14]))

            # print (title)
            # print (files[i])
            # print (y)
            x = [1, 2, 4, 8, 12, 16, 20, 24]  # ,28,32,36,40,44,48]

            # threads _ times replay is activated _ htm commit _ times act/throughput
            np.savetxt(
                "replayAdvStats" + title + ".txt",
                np.column_stack((x, t, y2, r)),
                delimiter="\t",
                fmt="%.3f",
            )
            if "usePCWM_" in files[i]:
                plt.plot(x, y1, label="SPHT Added to Replay Memory")  # "PCWM")
                # plt.plot(x, y2, label="SPHT Replay Activations")  # "PCWM")
                # plt.plot(x, y3, label = "SPHT POActiv") #"PCWM")
                # plt.plot(x, y4, label = "SPHT RAM Usage (%)") #"PCWM")

            if "usePCWMeADR_" in files[i]:
                plt.plot(x, y1, label="SPHT-eADR Added to Replay Memory")  # "PCWMeADR")
                # plt.plot(x, y2, label="SPHT-eADR Replay Activations")  # "PCWMeADR")
                # plt.plot(x, y3, label = "SPHT-eADR POActiv") #"PCWMeADR")
                # plt.plot(x, y4, label = "SPHT-eADR RAM Usage (%)") #"PCWM")
            if "usePCWMeADRT1_" in files[i]:
                plt.plot(
                    x, y1, label="Smart Close Added to Replay Memory"
                )  # "PCWMeADR+Flush")
                # plt.plot(
                #     x, y2, label="Smart Close Replay Activations"
                # )  # "PCWMeADR+Flush")
                # plt.plot(x, y3, label = "Smart Close POActiv") #"PCWMeADR+Flush")
                # plt.plot(x, y4, label = "Smart Close RAM Usage (%)") #"PCWMeADR+Flush")
            if "useSharedHTM_" in files[i]:
                plt.plot(
                    x, y1, label="HTM Only (Shared) Added to Replay Memory"
                )  # useSharedHTM
                # plt.plot(
                #     x, y2, label="HTM Only (Shared) Replay Activations"
                # )  # useSharedHTM
                # plt.plot(x, y3, label = "HTM Only (Shared) POActiv") #useSharedHTM
                # plt.plot(x, y4, label = "HTM Only (Shared) RAM Usage (%)") #useSharedHTM

            plt.xlabel("Threads")
            plt.ylabel("Value")
            plt.legend()
            plt.grid()
    else:
        i = i + 1
plt.title(title)
plt.savefig("plot_replayPages_" + title + ".png", dpi=150)
