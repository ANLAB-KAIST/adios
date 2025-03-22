import pandas

pandas.set_option("display.max_rows", 1000)
pth = "/opt/benchmark-out/tpcc/2024-10-13_10-48-16/rval_50krps_1/output.csv"

rdr = pandas.read_csv(pth, engine="pyarrow")

# rdr2 = rdr.sort_values("wr_id")[(rdr["wr_id"] >= 499900) & (rdr["wr_id"] <= 499980)]
rdr2 = rdr.sort_values("wr_id")[(rdr["dp_worker_id"] == 6)]

print(
    rdr2[
        [
            "wr_id",
            "lg_lat",
            "lg_cmd",
            "lg_count",
            "dp_queued",
            "dp_worker_id",
            "dp_pf",
            "dp_pf_cnt",
            "dp_num_enqueue",
            "dp_prev_wr_id",
        ]
    ]
)
