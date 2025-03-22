import sys
import pandas
import os


ROOT = "/opt/benchmark-out/"
MAX = 1000

print(sys.argv)

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

with open(os.path.join(sys.argv[2], FNAME), "w") as f:
    for pth in sorted(os.listdir(FPATH)):
        dpth = os.path.join(FPATH, pth)
        if not os.path.isdir(dpth):
            continue
        opth = os.path.join(dpth, "output.csv")
        rdr = pandas.read_csv(opth)
        rdr["lat"] = rdr["lg_lat"] / 1000
        rdr["rdma"] = rdr["dp_polling"] / 1000
        rdr["etc"] = (rdr["dp_worker"] - rdr["dp_polling"]) / 1000
        rdr["queue"] = rdr["dp_queued"] / 1000
        rdr["rdma_pending_ratio"] = (
            rdr["dp_prev_total_polling"] / rdr["dp_prev_total_worker"]
        )

        rdr = rdr.sort_values(by="lat")
        lats = rdr[["lat", "rdma", "queue", "etc", "rdma_pending_ratio"]]

        assert len(lats.index) == len(rdr.index)

        for a in lats.index.duplicated():
            if a:
                print(a)

        result = lats.quantile([0.1, 0.5, 0.99, 0.999], interpolation="lower")
        result.index.name = "quantile"
        result["P"] = "P" + (result.index * 100).astype(str).str.replace("\.0", "")

        ret = result.to_csv(sep="\t")

        f.write("== %s ==\n\n" % pth)
        f.write(ret)
        f.write("\n\n")
