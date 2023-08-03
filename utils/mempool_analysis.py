import sys
import os
import numpy as np
import matplotlib.pyplot as plt

def post_process(filename, show=False):
    n_allocated = 0
    # filename = 'logs/log_m{}_M{}_i{}.csv'.format(minimum, maximum, inertia)
    with open(filename) as logfile:
        n_mallocs = 0
        n_effective_mallocs = 0
        logs = logfile.read().split("\n")
        toplot = np.empty([0,6])
        # header = logs[0].split(",")
        # toplot = np.append(toplot, [[n_allocated, header[1], header[2], header[3]]], axis=0)
        for i, logline in enumerate(logs[1:-1]):
            log = logline.split(",")
            if log[0] == "alloc":
                n_mallocs = n_mallocs + 1
                n_allocated = n_allocated + 1
                if log[4] == "1":
                    n_effective_mallocs = n_effective_mallocs + 1
            elif log[0] == "free":
                n_allocated = n_allocated - 1
            if show:
                toappend = [i, n_allocated, int(log[1]), int(log[2]), int(log[3]), int(log[4])]
                toplot = np.append(toplot, [toappend], axis=0)
        toplot = toplot.transpose()
        x = toplot[0]


        if show:
            fig, ax = plt.subplots()
            ax.plot(x, toplot[1], label="user malloc")
            ax.plot(x, toplot[2], label="allocated buffers")
            ax.plot(x, toplot[3], label="available buffers")
            ax.plot(x, toplot[4], label="inertia")
            ax.plot(x, toplot[5], label="effective malloc and free")
            
            ax.legend(loc='upper left', shadow=True, fontsize='x-large')
            ax.set_xlabel("mempool operations")
            ax.set_title("ratio : {}".format(n_effective_mallocs / n_mallocs))
            print("malloc to effective malloc ratio : {}".format(n_effective_mallocs / n_mallocs))
            plt.show()

        if(n_effective_mallocs > 0):
            return n_effective_mallocs / n_mallocs
        else:
            return 1

plage_inertia = range(4, 40, 2)
plage_maximum = range(4,40,2)
# results = [0 for _ in plage_inertia]
results = [[], [], []]

def logfile(minimum, maximum, inertia):
    return 'log_m{}_M{}_i{}.csv'.format(minimum, maximum, inertia)

def run(minimum, maximum, inertia, additionals, binary, out):
    os.system("make -s {}".format(binary))
    os.system("./{} {} {} {} {}".format(binary, minimum, maximum, inertia, additionals))
    lf = logfile(minimum, maximum, inertia)
    os.system("mv {} logs/{}".format(lf, out))

def run_3d(binary):
    minimum = 10
    maximum = 100
    init = 100
    mallocs=1000
    for j,maximum in enumerate(plage_maximum):
        for i,inertia in enumerate(plage_inertia):
            print(maximum, inertia)
            s = 0
            for _ in range(100):
                run(minimum, maximum, inertia, "{} {}".format(init, mallocs), binary, logfile(minimum, maximum, inertia))
                s = s + post_process("logs/{}".format(logfile(minimum, maximum, inertia)))
            results[0].append(maximum)
            results[1].append(inertia)
            results[2].append(s/100)
    ax = plt.axes(projection='3d')
    ax.set_xlabel("mempool maximum size")
    ax.set_ylabel("mempool maximum inertia")
    ax.set_zlabel("malloc to effective malloc ratio")
    ax.scatter3D(results[0], results[1], results[2], c=results[2])
    # plt.plot(plage_inertia, results)
    plt.show()

def run_inertia(binary):
    minimum = 10
    maximum = 100
    init=100
    mallocs=1000
    plage_inertia = range(2, 50, 2)
    results = [0 for _ in plage_inertia]
    for i,inertia in enumerate(plage_inertia):
        print(inertia)
        s = 0
        for _ in range(100):
            out = logfile(minimum, maximum, inertia)
            run(minimum, maximum, inertia, "{} {}".format(init, mallocs), binary, out)
            s = s + post_process("logs/{}".format(out), show=False)
        results[i] =  s / 100
    plt.title("initial mallocs : {}\niterations : {}".format(init, mallocs))

    # ax = plt.axes(projection='3d')
    # ax.scatter3D(results[0], results[1], results[2], c=results[2])
    plt.xlabel("inertia")
    plt.ylabel("malloc to effective malloc ratio")
    plt.plot(plage_inertia, results)
    plt.show()

def run_2d(minimum, maximum, inertia, init, mallocs, binary):
    run(minimum, maximum, inertia, "{} {}".format(init, mallocs), binary, logfile(minimum, maximum, inertia))
    post_process("logs/{}".format(logfile(minimum, maximum, inertia)), show=True)

minimum = 10
maximum = 100
init = 50
mallocs = 1000
inertia=100
run_binary = ""
display_file = ""
additionals=""
out = "out.csv"

help = """
This script is used to run and to display informations on the lcp mempool
Usage : python process.py [options]

Options :
    -m          set mempool minimum size
    -M          set mempool maximum size
    -i          set mempool max inertia
    -a          set additional arguments
    -r          set binary to run
    -d          set log file to display
    -rd         set binary to run and display
    -h          display help"""

for i,arg in enumerate(sys.argv[1:]):
    if(arg == "-m"):
        minimum = sys.argv[i+2]
    if(arg == "-M"):
        maximum = sys.argv[i+2]
    if(arg == "-i"):
        inertia = sys.argv[i+2]
    if(arg == "-a" ):
        additionals = sys.argv[i+2]
    if(arg == "-o"):
        out = sys.argv[i+2]
    if(arg == "-rd" or arg == "-dr"):
        run_binary = sys.argv[i+2]
        # display_file = logfile(minimum, maximum, inertia)
        if out != "":
            display_file = "logs/{}".format(out)
        else:
            display_file = "logs/out.csv"
    if(arg == "-r"):
        run_binary = sys.argv[i+2]
    if(arg == "-d"):
        display_file = sys.argv[i+2]
    if(arg == "-h"):
        print(help)
if(run_binary != ""):
    run(minimum, maximum, inertia, additionals, run_binary, out)

if(display_file != ""):
    post_process(display_file, show=True)

# run_3d("mempool_random")
