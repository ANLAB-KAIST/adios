import sys
import numpy as np
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

        spf_addr = np.fromfile(os.path.join(dpth, "spf_addrs"), dtype=np.uint64)

        buffer_start = spf_addr[0]

        buffer_end2 = spf_addr[10485760 - 1]
        buffer_end = int(buffer_start + int(40 * 1024 * 1024 * 1024))

        print(hex(buffer_start), hex(buffer_end), hex(buffer_end2))

        spf_total = np.fromfile(os.path.join(dpth, "spf_lats0"), dtype=np.uint32)
        spf_load = np.fromfile(os.path.join(dpth, "spf_lats3"), dtype=np.uint32)
        spf_addr = spf_addr[10485760:]
        spf_total = spf_total[10485760:]
        spf_load = spf_load[10485760:]
        df = pandas.DataFrame(
            {
                "addr": spf_addr,
                "total": spf_total,
                "load": spf_load,
            }
        )

        count = np.count_nonzero((buffer_start <= spf_addr) & (spf_addr < buffer_end))
        print("count:", count)

        df = df[(buffer_start <= df["addr"]) & (df["addr"] < buffer_end)]
        df["idx"] = (df["addr"] - buffer_start) // 8
        df = df.reset_index(drop=True)

        print(df[:20])

        opth = os.path.join(dpth, "output.csv")
        rdr = pandas.read_csv(opth)
        rdr = rdr.sort_values(by="wr_id")

        rdr = rdr.reset_index(drop=True)

        df_last = len(df)
        rdr_last = len(df)
        df_idx = 0

        rdrs = []

        for wid in range(8):
            rdrs.append(rdr[rdr["dp_worker_id"] == wid].reset_index(drop=True))
        rdr_idxs = [0] * len(rdrs)

        rdr_lens = [len(rdr) for rdr in rdrs]

        while df_idx < df_last:
            found = False
            for wid in range(8):
                idx_start = rdr_idxs[wid]
                while (
                    rdr_idxs[wid] < rdr_lens[wid]
                    and rdrs[wid].at[rdr_idxs[wid], "lg_idx"] != df.at[df_idx, "idx"]
                ):
                    rdr_idxs[wid] += 1
                    if rdr_idxs[wid] - idx_start > 10:
                        rdr_idxs[wid] = idx_start
                        break
                if (
                    rdr_idxs[wid] < rdr_lens[wid]
                    and rdrs[wid].at[rdr_idxs[wid], "lg_idx"] == df.at[df_idx, "idx"]
                ):
                    found = True
                    rdrs[wid].at[rdr_idxs[wid], "dp_pf"] = df.at[df_idx, "total"]
                    rdrs[wid].at[rdr_idxs[wid], "dp_polling"] = df.at[df_idx, "load"]

                    break
            if not found:
                print("not found")
            df_idx += 1
            if df_idx % 1000 == 0:
                print(df_idx)

        rdr = pandas.concat(rdrs)

        rdr = rdr.sort_values(by="wr_id")

        rdr = rdr.reset_index(drop=True)
        rdr = rdr.set_index("wr_id")

        # now update dp_prev_total_polling

        for rowid in range(len(rdr)):
            this_wr_id = rdr.index[rowid]
            wr_id = this_wr_id
            count = rdr.at[wr_id, "dp_num_pending"]
            total_polling = 0
            total_total = 0
            try:
                while count > 0:
                    prev_wr_id = rdr.at[this_wr_id, "dp_prev_wr_id"]
                    total_total += rdr.at[prev_wr_id, "dp_worker"]
                    total_polling += rdr.at[prev_wr_id, "dp_polling"]
                    this_wr_id = prev_wr_id
                    count -= 1
            except KeyError:
                continue


            rdr.at[wr_id, "dp_prev_total_worker"] = total_total
            rdr.at[wr_id, "dp_prev_total_polling"] = total_polling
            if rowid % 1000 == 0:
                print(rowid)

        print(rdr[["dp_polling", "dp_pf"]])
        rdr["lat"] = rdr["lg_lat"] / 1000
        rdr["rdma"] = rdr["dp_polling"] / 1000
        rdr["etc"] = (rdr["dp_worker"] - rdr["dp_polling"]) / 1000
        rdr["queue"] = rdr["dp_queued"] / 1000
        rdr["rdma_pending_ratio"] = (
            rdr["dp_prev_total_polling"] / rdr["dp_prev_total_worker"]
        )

        lats = rdr[["lat", "rdma", "queue", "etc", "rdma_pending_ratio"]]

        assert len(lats.index) == len(rdr.index)

        # for a in lats.index.duplicated():
        #     if a:
        #         print(a)

        result = lats.quantile([0.1, 0.5, 0.99, 0.999], interpolation="lower")
        result.index.name = "quantile"
        result["P"] = "P" + (result.index * 100).astype(str).str.replace("\.0", "")

        ret = result.to_csv(sep="\t")

        f.write("== %s ==\n\n" % pth)
        f.write(ret)
        f.write("\n\n")
