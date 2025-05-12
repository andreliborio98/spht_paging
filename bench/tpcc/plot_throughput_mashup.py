#!/usr/bin/env python3

import sys
import matplotlib.pyplot as plt
import csv

warehouses = sys.argv[1]
title = sys.argv[2]
files = sys.argv[3:]
titles = [t[-3] for t in [f.split("_") for f in files]]

fig, axs = plt.subplots(
    nrows=1, ncols=1, figsize=(7, 3.2), sharex=False, gridspec_kw={"hspace": 0.1}
)

patterns = [
    [1, 0],
    [1, 2],
    [1, 2],
    [1, 0],
    [4, 2],
    [1, 0],
    [4, 2],
    [1, 0],
    [1, 0],
    [1, 0],
    [2, 2],
    [3, 2],
    [5, 2],
    [6, 2],
    [2, 2],
    [4, 2],
    [5, 2],
    [1, 0],
    [1, 0],
]

# patterns = [
#     [0, 0],
#     [1, 2],
#     [1, 2],
#     [0, 0],
#     [4, 2],
#     [0, 0],
#     [4, 2],
#     [0, 0],
#     [0, 0],
#     [0, 0],
#     [2, 2],
#     [3, 2],
#     [5, 2],
#     [6, 2],
#     [2, 2],
#     [4, 2],
#     [5, 2],
#     [0, 0],
#     [0, 0],
# ]

title_map = {
    "useCcHTM": "cc-HTM",
    "usePSTM": "PSTM",
    "useCraftyImmDur": "Crafty (Dur)",
    "useCrafty": "Crafty",
    "useLogicalClocks": "DudeTM",
    "usePhysicalClocks": "NV-HTM",
    "useSharedHTMUndo": "HTM+Undo (Shared)",
    "useHTMUndo": "HTM+Undo (Private)",
    "useSharedHTM": "HTM Only (Shared)",
    "useHTM": "HTM Only (Private)",
    "usePCWMeADRT1": "Smart Close",
    "usePCWMeADRT2": "No Write",
    "usePCWMeADRT3": "Spin 1000",
    "usePCWMeADRT4": "Spin 100000",
    "usePCWMeADR": "SPHT-eADR",
    "usePCWM2": "SPHT-FL",
    "usePCWM3": "SPHT-BL",
    "usePCWM": "SPHT-NL",
    "ORDERSTATUSdata1P": "SPHT+Replayer_O",
    "ORDERSTATUSdata2P": "SPHT+Pag_O",
    "ORDERSTATUSdata3P": "SPHT+Rep+Pag_O",
    "ORDERSTATUSdata4P": "SPHT+Rep+Pag+Hash_O",
    "ORDERSTATUSdata4NP": "SPHT+Rep+PagNoPageout+Hash_O",
    "ORDERSTATUSdata5P": "SPHT+Rep+Pag+Swap_O",
    "ORDERSTATUSdata6P": "SPHT_O",
    "ORDERSTATUSdata1H": "HTM+Replayer_O",
    "ORDERSTATUSdata2H": "HTM+Pag_O",
    "ORDERSTATUSdata3H": "HTM+Rep+Pag_O",
    "ORDERSTATUSdata4H": "HTM+Rep+Pag+Hash_O",
    "ORDERSTATUSdata5H": "HTM+Rep+Pag+Swap_O",
    "ORDERSTATUSdata6H": "HTM_O",
    "ORDSTATUSnPAYdata1P": "SPHT+Replayer_O+P",
    "ORDSTATUSnPAYdata2P": "SPHT+Pag_O+P",
    "ORDSTATUSnPAYdata3P": "SPHT+Rep+Pag_O+P",
    "ORDSTATUSnPAYdata4P": "SPHT+Rep+Pag+Hash_O+P",
    "ORDSTATUSnPAYdata4NP": "SPHT+Rep+PagNoPageout+Hash_O+P",
    "ORDSTATUSnPAYdata5P": "SPHT+Rep+Pag+Swap_O+P",
    "ORDSTATUSnPAYdata6P": "SPHT_O+P",
    "ORDSTATUSnPAYdata1H": "HTM+Replayer_O+P",
    "ORDSTATUSnPAYdata2H": "HTM+Pag_O+P",
    "ORDSTATUSnPAYdata3H": "HTM+Rep+Pag_O+P",
    "ORDSTATUSnPAYdata4H": "HTM+Rep+Pag+Hash_O+P",
    "ORDSTATUSnPAYdata5H": "HTM+Rep+Pag+Swap_O+P",
    "ORDSTATUSnPAYdata6H": "HTM_O+P",
    "ORDSTnPAYn50DELdata1P": "SPHT+Replayer_O+P+D",
    "ORDSTnPAYn50DELdata2P": "SPHT+Pag_O+P+D",
    "ORDSTnPAYn50DELdata3P": "SPHT+Rep+Pag_O+P+D",
    "ORDSTnPAYn50DELdata4P": "SPHT+Rep+Pag+Hash_O+P+D",
    "ORDSTnPAYn50DELdata4NP": "SPHT+Rep+PagNoPageout+Hash_O+P+D",
    "ORDSTnPAYn50DELdata5P": "SPHT+Rep+Pag+Swap_O+P+D",
    "ORDSTnPAYn50DELdata6P": "SPHT_O+P+D",
    "ORDSTnPAYn50DELdata1H": "HTM+Replayer_O+P+D",
    "ORDSTnPAYn50DELdata2H": "HTM+Pag_O+P+D",
    "ORDSTnPAYn50DELdata3H": "HTM+Rep+Pag_O+P+D",
    "ORDSTnPAYn50DELdata4H": "HTM+Rep+Pag+Hash_O+P+D",
    "ORDSTnPAYn50DELdata5H": "HTM+Rep+Pag+Swap_O+P+D",
    "ORDSTnPAYn50DELdata6H": "HTM_O+P+D",
    "PAYMENTdata1P": "SPHT+Replayer_P",
    "PAYMENTdata2P": "SPHT+Pag_P",
    "PAYMENTdata3P": "SPHT+Rep+Pag_P",
    "PAYMENTdata4P": "SPHT+Rep+Pag+Hash_P",
    "PAYMENTdata4NP": "SPHT+Rep+PagNoPageout+Hash_P",
    "PAYMENTdata5P": "SPHT+Rep+Pag+Swap_P",
    "PAYMENTdata6P": "SPHT_P",
    "PAYMENTdata1H": "HTM+Replayer_P",
    "PAYMENTdata2H": "HTM+Pag_P",
    "PAYMENTdata3H": "HTM+Rep+Pag_P",
    "PAYMENTdata4H": "HTM+Rep+Pag+Hash_P",
    "PAYMENTdata5H": "HTM+Rep+Pag+Swap_P",
    "PAYMENTdata6H": "HTM_P",
    "STOCKLVdata1P": "SPHT+Replayer_S",
    "STOCKLVdata2P": "SPHT+Pag_S",
    "STOCKLVdata3P": "SPHT+Rep+Pag_S",
    "STOCKLVdata4P": "SPHT+Rep+Pag+Hash_S",
    "STOCKLVdata4NP": "SPHT+Rep+PagNoPageout+Hash_S",
    "STOCKLVdata5P": "SPHT+Rep+Pag+Swap_S",
    "STOCKLVdata6P": "SPHT_S",
    "Adata1P": "SPHT+R_WorkloadA",
    "Adata4P": "SPHT+R+P+H_WorkloadA",
    "Adata4NP": "SPHT+R+PNoPageout+H_WorkloadA",
    "Adata6P": "SPHT_WorkloadA",
    "Adatahashposttx32": "SPHT+R+P+HpostTX_WorkloadA",
    "Adata4hashposttx32": "SPHT+R+P+HpostTX_WorkloadA",
    "ORDSTATUSnPAYdata4hashposttx32": "SPHT+R+P+HpostTX_O+P",
    "PAYMENTdata4hashposttx32": "SPHT+R+P+HpostTX_P",
    "ORDERSTATUSdata4hashposttx32": "SPHT+R+P+HpostTX_O",
    "ORDERSTATUSdata4hunP": "SPHT+Rep+Pag+Hash_O_100s",
    "ORDSTATUSnPAYdata4hunP": "SPHT+Rep+Pag+Hash_O+P_100s",
    "PAYMENTdata4hunP": "SPHT+Rep+Pag+Hash_P_100s",
}

for i in range(0, len(files), 2):
    x = []
    y = []
    yerr = []
    linewidth = 0.9
    marker = "."
    alpha = 0.7
    with open(files[i], "r") as avgFile:
        with open(files[i + 1], "r") as stdevFile:
            avgs = csv.reader(avgFile, delimiter="\t")
            stdevs = csv.reader(stdevFile, delimiter="\t")
            next(avgs)
            next(stdevs)
            for avg, stdev in zip(avgs, stdevs):
                x.append(int(avg[0]) + (i - len(files) / 2) * 0.05)
                y.append(((float(avg[2]) + float(avg[3])) / float(avg[1])) / 1.0e6)
                yerr.append(
                    ((float(stdev[2]) + float(stdev[3])) / float(avg[1])) / 1.0e6
                )

            titleAxis = ""
            for t in title_map:
                if t in titles[i]:
                    # print(title_map)
                    # print(titles)
                    titleAxis += title_map[t]
                    break

            # if "PCWM" in titles[i]:
            #     linewidth = 1.3
            # if "PCWM2" in titles[i] or "PCWM3" in titles[i]:
            #     linewidth = 1.5
            # if "PCWM2" in titles[i]:
            #     marker = "v"
            # if "PCWM3" in titles[i]:
            #     marker = "^"
            # if "Log" in titles[i]:
            #     marker = "+"
            # if "Crafty" in titles[i]:
            #     marker = "x"
            # if "Phy" in titles[i]:
            #     marker = "1"
            # if "CcHTM" in titles[i]:
            #     marker = "2"
            # if "PSTM" in titles[i]:
            #     marker = "p"
            if "PAYMENTdataP32" in titles[i]:
                marker = "x"
            if "ORDERSTATUSdataP32" in titles[i]:
                marker = "2"
            if "ORDSTATUSnPAYdataP32" in titles[i]:
                marker = "v"

            if "PAYMENTdataP64" in titles[i]:
                marker = "x"
            if "ORDERSTATUSdataP64" in titles[i]:
                marker = "2"
            if "ORDSTATUSnPAYdataP64" in titles[i]:
                marker = "v"

            if "PAYMENTdataS32" in titles[i]:
                marker = "x"
            if "ORDERSTATUSdataS32" in titles[i]:
                marker = "2"
            if "ORDSTATUSnPAYdataS32" in titles[i]:
                marker = "v"

            if "PAYMENTdataS64" in titles[i]:
                marker = "x"
            if "ORDERSTATUSdataS64" in titles[i]:
                marker = "2"
            if "ORDSTATUSnPAYdataS64" in titles[i]:
                marker = "v"

            if "PAYMENTdataH32" in titles[i]:
                marker = "x"
            if "ORDERSTATUSdataH32" in titles[i]:
                marker = "2"
            if "ORDSTATUSnPAYdataH32" in titles[i]:
                marker = "v"

            if "PAYMENTdataH64" in titles[i]:
                marker = "x"
            if "ORDERSTATUSdataH64" in titles[i]:
                marker = "2"
            if "ORDSTATUSnPAYdataH64" in titles[i]:
                marker = "v"

            # axs.plot([x2+(i-6)*0.08 for x2 in x ], y, alpha = alpha,
            #     label=title, linewidth=linewidth, markersize=8, marker=marker, dashes=patterns[int(i/2)])
            # print("pattern=" + str(patterns[int(i / 2)]))
            axs.errorbar(
                [x2 + (i - 6) * 0.06 for x2 in x],
                y,
                label=titleAxis,
                yerr=yerr,
                linewidth=linewidth,
                markersize=8,
                marker=marker,
                dashes=patterns[int(i / 2)],
            )
            # line.set_dashes(patterns[int(i/2)])

# axs.set_title("TPC-C", pad=1.5)
axs.tick_params(axis="y", labelsize=10, rotation=60, pad=-2)

box = axs.get_position()
axs.set_position(
    [
        box.x0 - box.width * (0.05),
        box.y0 + box.height * (0.05),
        box.width * 0.65,
        box.height * 1.1,
    ]  # was box.width * 0.70,
)
plt.margins(0.02, 0.02)

plt.xlabel("Threads", size=14)
plt.ylabel("Throughput (MTXs/s)", size=14)
plt.legend(bbox_to_anchor=(1.0, 1.0), loc="upper left", ncol=1)

plt.gca().xaxis.grid(True, linestyle="--", linewidth=0.2)
plt.gca().yaxis.grid(True, linestyle="--", linewidth=0.2)

# xticks = [1, 4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 64]
xticks = [1, 2, 4, 8, 12, 16, 20, 24]
plt.yticks(size=12)
plt.xticks(xticks, xticks, size=12)

# plt.title("TPC-C 32 warehouses 98% pay. 1% n.o. 1 del. TXs")
title = "mashup"
plt.savefig("mashupStats" + warehouses + "/plot_throughput_" + title + ".pdf")
# plt.show()
