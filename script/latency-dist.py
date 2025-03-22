import sys
import pandas


FPATH = "/tmp/output.csv"

if len(sys.argv) > 1:
    FPATH = sys.argv[1]


rdr = pandas.read_csv(FPATH)
rdr["lat"] = rdr["lg_lat"] / 1000

stats = (
    rdr.groupby("lat")["lat"]
    .agg("count")
    .pipe(pandas.DataFrame)
    .rename(columns={"lat": "frequency"})
)

stats["pdf"] = stats["frequency"] / sum(stats["frequency"])
stats = stats.sort_values(by="lat")
stats["cdf"] = stats["pdf"].cumsum().round(4)


SAMPLE = 500

if len(stats.index) > SAMPLE:
    step = int(len(stats.index) / SAMPLE)
    stats = stats[::step]

stats["cdf"].to_csv(sys.stdout, sep="\t")
