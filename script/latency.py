import csv
import sys
import pandas


FPATH = "/tmp/output.csv"

if len(sys.argv) > 1:
    FPATH = sys.argv[1]

rdr = pandas.read_csv(FPATH, engine="pyarrow")


# count = len(SORTED)
# if count == 0:
#     print("No data")
#     exit(1)

# lat_sum = sum((int(s["lg_lat"]) for s in SORTED))
# p50 = SORTED[int(count / 2)]
# p99 = SORTED[int(count * 99 / 100)]
# p999 = SORTED[int(count * 999 / 1000)]
lats = rdr["lg_lat"] / 1000
print("Avg Lat:", lats.mean())
print("P10 Lat:", lats.quantile(0.1, interpolation="lower"))
print("P50 Lat:", lats.quantile(0.5, interpolation="lower"))
print("P99 Lat:", lats.quantile(0.99, interpolation="lower"))
print("P99.9 Lat:", lats.quantile(0.999, interpolation="lower"))
