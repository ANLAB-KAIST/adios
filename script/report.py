import configparser
import sys


FPATH = "/tmp/report.txt"

if len(sys.argv) > 1:
    FPATH = sys.argv[1]

config = configparser.ConfigParser()
config.read(FPATH)

if "REPORT" in config:
    for key in config["REPORT"]:
        print(key, ":", config["REPORT"][key])


for sec in config:
    if sec.startswith("WORKER_"):
        print("----", sec, "----")
        for key in config[sec]:
            print(sec, key, ":", config[sec][key])
