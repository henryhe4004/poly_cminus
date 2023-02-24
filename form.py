import argparse
import sqlite3
import time
from typing import Dict, List

PANDAS = True
try:
    import pandas as pd
except ImportError:
    PANDAS = False


def get_all_results(db_path: str) -> Dict[str, List[Dict[str, float]]]:
    con = sqlite3.connect(db_path)
    cur = con.cursor()
    cur.execute("select testname, testtime, timecost, options from testresults")
    l = cur.fetchall()
    # print(l)
    all_result = {}
    for tr in l:
        str_time = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(tr[1]))
        str_time = f"{str_time} {tr[3]}"
        if all_result.get(tr[0]) is None:
            all_result[tr[0]] = {str_time: tr[2]}
        else:
            all_result[tr[0]][str_time] = tr[2]
    con.close()
    return all_result


def main():
    arg_par = argparse.ArgumentParser()
    arg_par.add_argument("-i", type=str, default="benchmark_results.db",
                         help="input file")
    arg_par.add_argument("--type", type=str, default="excel",
                         choices=["excel", "html", "json"], help="output type")
    arg_par.add_argument("-T", action="store_true",
                         help="transpose the form")
    arg_par.add_argument("-o",
                         help="output path")
    args = arg_par.parse_args()
    exts = {
        "json": ".json",
        "excel": ".xlsx",
        "html": ".html"
    }
    if args.o is None:
        output = "results" + exts[args.type]
    else:
        output = args.o
    results = get_all_results(args.i)
    if PANDAS:
        df = pd.DataFrame(results)
        df.sort_index(inplace=True)
        df.sort_index(axis=1, inplace=True)
        if args.T:
            df = df.T
        if args.type == "excel":
            df.to_excel(output)
        elif args.type == "html":
            df.to_html(output)
        elif args.type == "json":
            df.to_json(output, indent=4)
    else:
        import json
        print("cannot import pandas, only supports json.")
        json.dump(get_all_results(args.i), open("results.json", "w"), indent=4)


if __name__ == "__main__":
    main()
