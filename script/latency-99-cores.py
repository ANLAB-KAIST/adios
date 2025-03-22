import sys
import os
import re
import pandas

import configparser

ROOT = "/opt/benchmark-out/"
MAX = 10000



if len(sys.argv) < 2:
    print("require exp")
    exit(0)

EXP = sys.argv[1]
EXP_PATH = os.path.join(ROOT, EXP)



if len(sys.argv) < 4:
    FNAME = sorted(os.listdir(EXP_PATH))[-1]
else:
    FNAME = sys.argv[3]


FPATH = os.path.join(EXP_PATH, FNAME)

if len(sys.argv) < 3:
    OUTPUT_FPATH = ""
else:
    OUTPUT_FPATH = os.path.join(sys.argv[2], FNAME + ".txt")



print("FPATH:", FPATH)
print("OUTPUT_FPATH:", OUTPUT_FPATH)


def handle_csv(pth):
    # rdr = csv.DictReader(open(pth, "r"))
    rdr = pandas.read_csv(pth)
    lats = rdr["lg_lat"]/1000
    lats = lats.sort_values()
    return {
        "avg": min(lats.mean(),MAX),
        "p10": min(lats.quantile(0.1, interpolation="lower"),MAX),
        "p50": min(lats.quantile(0.5, interpolation="lower"),MAX),
        "p99": min(lats.quantile(0.99, interpolation="lower"),MAX),
        "p99.9": min(lats.quantile(0.999, interpolation="lower"),MAX),
    }



TABLE_LIST = ["Dropped", "p99", "p99.9", "p50", "avg", "p10"]

RESULT = {} 
RESULT_EXTRA = {}
MODES = set()


for pth in os.listdir(FPATH):
    if not os.path.isdir(os.path.join(FPATH,pth)):
        continue
    mth = re.match(r"(.+)_([0-9]+)krps_([0-9]+)_([0-9]+)", pth)
    pth = os.path.join(FPATH,pth)
    mode = mth.group(3)
    krps = int(mth.group(2))
    # cores = int(mth.group(3))
    try_ = int(mth.group(4))
    report_pth = os.path.join(pth, "report.txt")

    if not os.path.exists(report_pth):
        print("not exist: ", report_pth)
        continue
    config = configparser.ConfigParser()
    config.read(report_pth)
    dropped = config["REPORT"]["dropped"]

    output_pth = os.path.join(pth, "output.csv")

    if not os.path.exists(output_pth):
        print("not exist: ", output_pth)
        continue
    if os.stat(output_pth).st_size == 0:
        continue
    output = handle_csv(output_pth)
    output["Dropped"] = dropped
    MODES.add(mode)
    if try_ not in RESULT:
        RESULT[try_] = {}
    
    
    for table in TABLE_LIST:
        if table not in RESULT[try_]:
            RESULT[try_][table] = {}

        if krps not in RESULT[try_][table]:
            RESULT[try_][table][krps] = {}
        RESULT[try_][table][krps][mode] = output[table]

    for opth in os.listdir(pth):
        mth = re.match(r"output-(.+?).csv", opth)
        if not mth:
            continue
        part = mth.group(1)
        if part not in RESULT_EXTRA:
            RESULT_EXTRA[part] = {}
        opth = os.path.join(pth, opth)
        if os.stat(opth).st_size == 0:
            continue
        output = handle_csv(opth)
        if try_ not in RESULT_EXTRA[part]:
            RESULT_EXTRA[part][try_] = {}
        
        for table in TABLE_LIST[1:]:
            if table not in RESULT_EXTRA[part][try_]:
                RESULT_EXTRA[part][try_][table] = {}

            if krps not in RESULT_EXTRA[part][try_][table]:
                RESULT_EXTRA[part][try_][table][krps] = {}
            RESULT_EXTRA[part][try_][table][krps][mode] = output[table]

    
    


MODES = sorted(list(MODES))

OUTPUT=""
for try_ in RESULT:
    for table in TABLE_LIST:
        OUTPUT += "%s (Try: %s)\n\n" % (table, try_)
        OUTPUT += "\t".join(["kRPS"] + MODES) + "\n"
        for krps in sorted(RESULT[try_][table]):
           
            OUTPUT += "\t".join([str(krps)] + [ str(RESULT[try_][table][krps].get(mode,"")) for mode in MODES] ) + "\n"


        OUTPUT +="\n\n\n"

for part in RESULT_EXTRA:
    OUTPUT += "\n\n ==== %s ====\n\n" % part
    for try_ in RESULT_EXTRA[part]:
        for table in TABLE_LIST[1:]:
            OUTPUT += "%s (Try: %s)\n\n" % (table, try_)
            OUTPUT += "\t".join(["kRPS"] + MODES) + "\n"
            for krps in sorted(RESULT_EXTRA[part][try_][table]):
            
                OUTPUT += "\t".join([str(krps)] + [ str(RESULT_EXTRA[part][try_][table][krps].get(mode,"")) for mode in MODES] ) + "\n"


            OUTPUT +="\n\n\n"

print(OUTPUT)

if OUTPUT_FPATH:
    with open(OUTPUT_FPATH, "w") as f:
        f.write(OUTPUT)