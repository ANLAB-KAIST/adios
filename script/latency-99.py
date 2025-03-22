import sys
import os
import re
import pandas

import configparser

ROOT = "/opt/benchmark-out/"
MAX = 10000000

X_DIVIDE = 1

if len(sys.argv) < 2:
    print("require exp")
    exit(0)


EXP = sys.argv[1]
EXP_PATH = os.path.join(ROOT, EXP)


if len(sys.argv) < 4:
    FNAMES = [sorted(os.listdir(EXP_PATH))[-1]]
else:
    FNAMES = sys.argv[3:]


FPATHS = [os.path.join(EXP_PATH, FNAME) for FNAME in FNAMES]

if len(sys.argv) < 3:
    OUTPUT_FPATH = ""
else:
    OUTPUT_FPATH = os.path.join(sys.argv[2], "--".join(FNAMES) + ".txt")


print("FPATHS:", FPATHS)
print("OUTPUT_FPATH:", OUTPUT_FPATH)


def handle_csv(pth, m=MAX, dropped=None):
    # rdr = csv.DictReader(open(pth, "r"))
    rdr = pandas.read_csv(pth, engine="pyarrow")
    lats = rdr["lg_lat"] / 1000
    lats = lats.sort_values()
    if dropped and dropped != 0:
        lats = pandas.concat([lats, *[lats.tail(1)] * dropped])
    else:
        dropped = 0

    droprate = dropped / len(lats)

    avg = min(lats.mean(), m)

    p10 = min(lats.quantile(0.1, interpolation="lower"), m) if droprate < 0.9 else m
    p50 = min(lats.quantile(0.5, interpolation="lower"), m) if droprate < 0.5 else m
    p99 = min(lats.quantile(0.99, interpolation="lower"), m) if droprate < 0.01 else m
    p99_9 = (
        min(lats.quantile(0.999, interpolation="lower"), m) if droprate < 0.001 else m
    )

    return {
        "avg": avg,
        "p10": p10,
        "p50": p50,
        "p99": p99,
        "p99.9": p99_9,
    }


def handle_log(pth, m=MAX):
    with open(pth, "r") as f:
        text = f.read()
        result = {
            1: [],
            2: [],
            3: [],
            4: [],
            5: [],
            6: [],
            7: [],
            8: [],
        }
        for id, _, rpgc in re.findall(
            r"ID:([0-9a-f]+?)\n(RPGC|GRPS):([0-9a-f]+?)\n", text
        ):
            id = int(id, 16)
            rpgc = int(rpgc, 16)
            if id == 0 or id > 8:
                # for now, hard-code for 8 workers
                continue
            result[id].append(rpgc)

        total = 0

        for id in result:
            if result[id]:
                total += sum(result[id]) / len(result[id])
        rdmaps = total * 2  # 2GHz
        return rdmaps
    return 0


TABLE_LIST = ["Dropped", "DropRate", "p99", "p99.9", "p50", "avg", "p10", "rdmaps"]

RESULT = {}
RESULT_EXTRA = {}
MODES = set()

for FPATH in FPATHS:
    for pth in sorted(os.listdir(FPATH)):
        if not os.path.isdir(os.path.join(FPATH, pth)):
            continue
        mth = re.match(r"(.+)_([0-9]+k?)rps(_[0-9]+G)?_([0-9]+)", pth)
        pth = os.path.join(FPATH, pth)
        mode = mth.group(1)
        rps_offered = int(mth.group(2).replace("k", "000"))
        mem = mth.group(3)
        try_ = int(mth.group(4))
        if mem:
            mode = mode + mem
        report_pth = os.path.join(pth, "report.txt")

        if not os.path.exists(report_pth):
            print("not exist: ", report_pth)
            continue
        config = configparser.ConfigParser()
        config.read(report_pth)
        dropped = config["REPORT"]["dropped"]
        num_operation = int(config["REPORT"]["num_operation"])
        rps = int(config["REPORT"]["rps"])
        rx_lasts = []
        tx_firsts = []
        for section in config.sections():
            if not section.startswith("WORKER_"):
                continue
            rx_lasts.append(int(config[section]["rx_last"]))
            tx_firsts.append(int(config[section]["tx_first"]))
        duration_ns = max(rx_lasts) - min(tx_firsts)
        tp = (num_operation - int(dropped)) * 1000 * 1000 / duration_ns
        output_pth = os.path.join(pth, "output.csv")
        dispatcher_pth = os.path.join(pth, "dispatcher.log")

        if not os.path.exists(output_pth):
            print("not exist: ", output_pth)
            continue
        if os.stat(output_pth).st_size == 0:
            continue

        rdmaps = 0
        if os.path.exists(dispatcher_pth):
            rdmaps = handle_log(dispatcher_pth)

        output = handle_csv(output_pth, dropped=int(dropped))
        output["Dropped"] = dropped
        droprate = int(dropped) * 100 / num_operation
        output["DropRate"] = droprate
        output["rdmaps"] = rdmaps

        MODES.add(mode)
        if try_ not in RESULT:
            RESULT[try_] = {}

        for table in TABLE_LIST:
            if table not in RESULT[try_]:
                RESULT[try_][table] = {}

            if rps_offered not in RESULT[try_][table]:
                RESULT[try_][table][rps_offered] = {}
            RESULT[try_][table][rps_offered][mode] = output[table]
            RESULT[try_][table][rps_offered][mode + "-tput"] = int(tp) / X_DIVIDE

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
            if part.startswith("10"):
                output = handle_csv(opth, MAX * 10, dropped=int(dropped))
            else:
                output = handle_csv(opth, dropped=int(dropped))
            if try_ not in RESULT_EXTRA[part]:
                RESULT_EXTRA[part][try_] = {}

            for table in TABLE_LIST[2:-1]:
                if table not in RESULT_EXTRA[part][try_]:
                    RESULT_EXTRA[part][try_][table] = {}

                if rps_offered not in RESULT_EXTRA[part][try_][table]:
                    RESULT_EXTRA[part][try_][table][rps_offered] = {}
                RESULT_EXTRA[part][try_][table][rps_offered][mode] = output[table]
                RESULT_EXTRA[part][try_][table][rps_offered][mode + "-tput"] = (
                    int(tp) / X_DIVIDE
                )


MODES = sorted(list(MODES))
MODES_TP = [mode + "-tput" for mode in MODES]
MODES += MODES_TP

OUTPUT = ""
for try_ in RESULT:
    for table in TABLE_LIST:
        OUTPUT += "%s (Try: %s)\n\n" % (table, try_)
        OUTPUT += "\t".join(["RPS"] + MODES) + "\n"
        for krps in sorted(RESULT[try_][table]):
            OUTPUT += (
                "\t".join(
                    [str(krps)]
                    + [str(RESULT[try_][table][krps].get(mode, "")) for mode in MODES]
                )
                + "\n"
            )

        OUTPUT += "\n\n\n"

for part in sorted(RESULT_EXTRA.keys()):
    OUTPUT += "\n\n ==== %s ====\n\n" % part
    for try_ in RESULT_EXTRA[part]:
        for table in TABLE_LIST[2:-1]:
            OUTPUT += "%s (Try: %s)\n\n" % (table, try_)
            OUTPUT += "\t".join(["RPS"] + MODES) + "\n"
            for rps_offered in sorted(RESULT_EXTRA[part][try_][table]):
                OUTPUT += (
                    "\t".join(
                        [str(rps_offered)]
                        + [
                            str(
                                RESULT_EXTRA[part][try_][table][rps_offered].get(
                                    mode, ""
                                )
                            )
                            for mode in MODES
                        ]
                    )
                    + "\n"
                )

            OUTPUT += "\n\n\n"

print(OUTPUT)

if OUTPUT_FPATH:
    with open(OUTPUT_FPATH, "w") as f:
        f.write(OUTPUT)
