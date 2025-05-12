import matplotlib.pyplot as plt
from matplotlib.gridspec import GridSpec
from matplotlib.ticker import FuncFormatter
import numpy as np
import scipy.stats as st
import os
import pdfkit
import sys
import pylab





resultdir = 'output'
resultbounddir = 'output-bound'
plotfolder = 'graphs'
tablefolder = 'tables'

LogSize = 1024*1024*1024
HeapSize = 1024*1024*512
#HeapSize = 1024*1024*128

Workers = [16, 64]

###LogNUMA  = [1, 2]
#LogNUMA  = [2]
###LogType  = [0, 1]
LogType  = [0]

#threads = ['1', '2', '4', '8', '16', '32']
workers = ['1', '2', '4', '8', '16', '32']
replayers = ['1', '2', '4', '8', '16', '32']
#replayers = ['1', '2', '4', '8', '16']
#apps = ['replay-st', 'replay-nt', 'replay-dram', 'replay-wbinvd', 'replay-allf']
#apps = ['replay-st', 'replay-wbinvd', 'replay-allf', 'replay-nt' ]
#apps = ['replay-st', 'replay-wbinvd', 'replay-allf', 'replay-nt' ]
#apps = ['replay-st', 'bt-st', 'replay-wbinvd', 'bt-wbinvd', 'replay-allf', 'bt-allf', 'replay-nt', 'bt-nt']
#apps = ['bt-wbinvd', 'replay-wbinvd', 'bt-st', 'replay-st']
apps = ['replay-st', 'replay-wbinvd']
HeapNUMA = [1, 2]
style = ['o', 'x', 'd', 's', 'h'];
###style = ['o', 'x', 'd', 's', 'o', 'x', 'd', 's'];
#style = ['x', 'x', 'o', 'o', 'd', 'd', 's', 's'];
###lstyle = ['-', '-', '-', '-', '--', '--', '--', '--'];
lstyle = ['-', '-', '-', '-', '-', '--', '-', '--'];
#legend = ['wbinvd (optimal)', 'wbinvd (vanilla)',
#        'store-and-flush (optimal)', 'store-and-flush (vanilla)']

legend = ['store-and-flush (local)', 'store-and-flush (NUMA)', 'wbinvd (local)', 'wbinvd (NUMA)']


#bapps = ['bt-st', 'bt-nt', 'bt-wbinvd', 'bt-allf', 'bt-pmem', 'bt-pmemwb']
bapps = ['bt-st', 'bt-nt', 'bt-wbinvd', 'bt-allf']
numaconf = ['1', '2']
threads = ['1', '2', '4', '8', '16', '32']


def millions(x, pos):
    'The two args are the value and tick position'
    return '%1.0f' % (x * 1e-6)

def getTimestamp(filename):
    values=[]
    with open(filename) as f:
        for line in f:
            fields = line.strip().split()
            #if fields and fields[0] == 'Elapsed' and fields[1] == 'time:':
            if fields and fields[0] == 'Throughput:':
                values.append(float(fields[1]))

    return values


def get_mean_v(values):

    mean = -1
    std = -1
    # calculate the mean and standard deviation of the samples
    if values:
#        mean = np.mean(values)
#        std = np.std(values, ddof=1)
        n, min_max, mean, var, skew, kurt = st.describe(values)
        std=np.sqrt(var)

        ci = st.t.interval(0.95,len(values)-1,loc=mean,scale=std/np.sqrt(len(values)))
#        std = st.t.interval(0.95, len(values)-1, loc=mean, scale=st.sem(values))

    diff = (ci[1]-ci[0])/2

    return mean, diff


def get_mean(filename):

    # find the lines with the time stamp
    values = getTimestamp(filename)

    return get_mean_v(values)


def save_data(outfilename, filedir, execs, replayers, ltype, hnum, w):

    f = open(outfilename, 'wb')


    for ap in execs:

        for hn in hnum:

            mean_values = []
            std_values = []
            for t in replayers:

                workers = w
#            if (ltype == 1):
#                workers = t

                ###namet = str(LogSize) + '-' + str(HeapSize) + '-' + str(ltype) + '-' + str(hnum) + '-' + str(lnum)
                #namet = str(LogSize) + '-' + str(HeapSize) + '-' + str(ltype) + '-' + str(hn) + '-' + str(hn)
                namet = str(LogSize) + '-' + str(HeapSize) + '-' + str(ltype) + '-' + str(hn)
                if workers == 16:
                    #if hn == 2:     # with less than 32 worker threads, consider the logs local
                    namet = namet + '-1'
                else:
                    namet = namet + '-' + str(hn)
                namet = namet + '-' + str(workers)
                filename = filedir + '/' + ap + '-' + namet + '-' + t

                print(filename)
                m, s = get_mean(filename)

                if m != -1:
                    mean_values.append(m)
                    std_values.append(s)

            np.savetxt(f, (mean_values, std_values), fmt='%f',
                            delimiter=',', header=ap+namet)

    f.close()


def save_databound(outfilename, filedir, execs, threads, numa):

    f = open(outfilename, 'wb')


    for ap in execs:

        mean_values = []
        std_values = []
        for t in replayers:

            filename = filedir + '/' + ap + '-1GB-' + numa + '-' + t

            #print(filename)
            m, s = get_mean(filename)

            if m != -1:
                mean_values.append(m)
                std_values.append(s)

        np.savetxt(f, (mean_values, std_values), fmt='%f',
                        delimiter=',', header=ap+'bound-'+numa)

    f.close()


def plot_graphs_bound():

    if not os.path.exists(plotfolder):
        os.makedirs(plotfolder)

    if not os.path.exists(tablefolder):
        os.makedirs(tablefolder)

    for hnum in numaconf:

        print('Generating for log ', hnum)

        namet = str(hnum) + 'bound'

        filename = tablefolder + '/time-' + namet + '.csv'

        save_databound(filename, resultbounddir, bapps, threads, hnum)


        values = np.genfromtxt(filename, delimiter=',')

        # a list of lists with execution time for each tm configuration
        listVal = []
        listErr = []


        # 1st row is time, 2nd row is stddev
        items = int(len(values)/2)

        # reads the values for each tm configuratoin
        for dr in range(items):
            bars1_v = []
            bars1_i = []
            for i in values[dr*2]:
                bars1_v.append(i)
            for i in values[dr*2+1]:
                bars1_i.append(i)


            listVal.append(bars1_v)
            listErr.append(bars1_i)


        fig = plt.figure()
        ax = plt.axes()

        for i, row in enumerate(listVal):
            X = np.arange(len(row))
            ax.errorbar(replayers, row, yerr=listErr[i],linestyle = '-',
                    marker=style[i], label = bapps[i])
#        p = ax.plot(threads, row,
#            label = apps[i],
#            marker = style[i],
#            linestyle = '-')

        #ax.plot(threads, values[0], 'o-');
        #ax.errorbar(threads, values[0], yerr=values[1])


        #for i,j in zip(threads,values[0]):
        #    ax.annotate(str(j),xy=(i,j))

        title = 'UpperBound '

        if (hnum == '1'):
            title = title + ' (1 NUMA node)'
        else:
            title = title + ' (2 NUMA nodes)'


        plt.ylabel('Throughput (entry/s)')
        plt.xlabel('Threads')
        plt.title(title)
        ax.legend()
        filename = plotfolder + '/' + namet +  '.pdf'
        plt.savefig(filename, bbox_inches = 'tight', pad_inches = 0)



def plot_graphs():

    if not os.path.exists(plotfolder):
        os.makedirs(plotfolder)

    if not os.path.exists(tablefolder):
        os.makedirs(tablefolder)


    for ltype in LogType:
        for w in Workers:
#        for lnum in LogNUMA:
#            for hnum in HeapNUMA:

            #print('Generating for log ', ltype, ' with LN ', lnum, 'and HN ', hnum)
            print('Generating for log ', ltype, ' with ', str(w), ' workers')

            ###namet = str(LogSize) + '-' + str(HeapSize) + '-' + str(ltype) + '-' + str(lnum) + '-' + str(hnum)
            #namet = str(LogSize) + '-' + str(HeapSize) + '-' + str(ltype) + '-' + str(hnum) + '-' + str(hnum)
            namet = str(LogSize) + '-' + str(HeapSize) + '-' + str(ltype) + '-' + str(w)

            filename = tablefolder + '/time-' + namet + '.csv'

            save_data(filename, resultdir, apps, replayers, ltype, HeapNUMA, w)


            values = np.genfromtxt(filename, delimiter=',')

            # a list of lists with execution time for each tm configuration
            listVal = []
            listErr = []


            # 1st row is time, 2nd row is stddev
            items = int(len(values)/2)

            # reads the values for each tm configuratoin
            for dr in range(items):
                bars1_v = []
                bars1_i = []
                for i in values[dr*2]:
                    bars1_v.append(i)
                for i in values[dr*2+1]:
                    bars1_i.append(i)


                listVal.append(bars1_v)
                listErr.append(bars1_i)


            fig = plt.figure()
            ax = plt.axes()

            gap = .9 / len(listVal)
            for i, row in enumerate(listVal):
                X = np.arange(len(row))
                ax.errorbar(replayers, row, yerr=listErr[i],
                        linestyle = lstyle[i], marker=style[i], label =
                        legend[i])
                ##p = ax.bar(X + i * gap, row,
                ##        width = gap,
                ##        label = apps[i],          # tm labels for the legend
                ##        yerr=listErr[i],
                ##        edgecolor = "black")

            ##ind = np.arange(len(replayers))
            ##plt.xticks(ind + gap + 0.1, replayers)


            #for i,j in zip(threads,values[0]):
            #    ax.annotate(str(j),xy=(i,j))

#            if (ltype == 0):
#                title = 'Vanilla -'
#            else:
#                title = 'Shards -'

            ###if (lnum == 1):
            ###    title = title + ' (Logs: 1 NUMA)'
            ###else:
            ###    title = title + ' (Logs: 2 NUMA)'

#            if (hnum == 1):
#                title = title + ' (Heap: 1 NUMA)'
#            else:
#                title = title + ' (Heap: 2 NUMA)'

            title = 'Workers: ' + str(w)

            formatter = FuncFormatter(millions)
            ax.yaxis.set_major_formatter(formatter)

            plt.ylabel('Throughput ($10^6x$ entries/s)')
            plt.xlabel('Replayers')
            plt.title(title)
            ax.legend()
            filename = plotfolder + '/' + namet +  '.pdf'
            plt.savefig(filename, bbox_inches = 'tight', pad_inches = 0)




if __name__ == '__main__':

    SMALL_SIZE = 8
    MEDIUM_SIZE = 10
    BIGGER_SIZE = 10

    plt.rc('font', size=SMALL_SIZE)          # controls default text sizes
    plt.rc('axes', titlesize=SMALL_SIZE)     # fontsize of the axes title
    plt.rc('axes', labelsize=MEDIUM_SIZE)    # fontsize of the x and y labels
    plt.rc('xtick', labelsize=SMALL_SIZE)    # fontsize of the tick labels
    plt.rc('ytick', labelsize=SMALL_SIZE)    # fontsize of the tick labels
    plt.rc('legend', fontsize=SMALL_SIZE)    # legend fontsize
    plt.rc('figure', titlesize=BIGGER_SIZE)  # fontsize of the figure title

    plt.rc('grid', linestyle='dashed', color='black')


    plot_graphs()
#    plot_graphs_bound()

