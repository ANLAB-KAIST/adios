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
        spf_lock_get_pte = np.fromfile(os.path.join(dpth, "spf_lats1"), dtype=np.uint32)
        spf_page_io = np.fromfile(os.path.join(dpth, "spf_lats2"), dtype=np.uint32)
        spf_load = np.fromfile(os.path.join(dpth, "spf_lats3"), dtype=np.uint32)
        spf_cgroup_account = np.fromfile(
            os.path.join(dpth, "spf_lats4"), dtype=np.uint32
        )
        spf_ret_to_user = np.fromfile(os.path.join(dpth, "spf_lats5"), dtype=np.uint32)
        spf_set_pagemap_unlock = np.fromfile(
            os.path.join(dpth, "spf_lats6"), dtype=np.uint32
        )
        spf_set_pte = np.fromfile(os.path.join(dpth, "spf_lats7"), dtype=np.uint32)
        spf_addr = spf_addr[10485760:]
        spf_total = spf_total[10485760:]
        spf_lock_get_pte = spf_lock_get_pte[10485760:]
        spf_page_io = spf_page_io[10485760:]
        spf_load = spf_load[10485760:]
        spf_cgroup_account = spf_cgroup_account[10485760:]
        spf_ret_to_user = spf_ret_to_user[10485760:]
        spf_set_pagemap_unlock = spf_set_pagemap_unlock[10485760:]
        spf_set_pte = spf_set_pte[10485760:]
        df = pandas.DataFrame(
            {
                "addr": spf_addr,
                "total": spf_total,
                "lock_get_pte": spf_lock_get_pte,
                "page_io": spf_page_io,
                "load": spf_load,
                "cgroup_account": spf_cgroup_account,
                "ret_to_user": spf_ret_to_user,
                "set_pagemap_unlock": spf_set_pagemap_unlock,
                "set_pte": spf_set_pte,
            }
        )
        df = df[df["addr"] != 0]
        df = df.sort_values(by="total", ascending=False)
        df = df.reset_index()
        print(df[:50])
        result = df.quantile(
            [0.1, 0.5, 0.99, 0.999, 0.9999, 0.99999, 0.999999], interpolation="lower"
        )


        df2 = df[df["total"] < df["load"]]
        print(df2)
