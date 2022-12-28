import glob
import math
import pickle
from typing import Dict, List, Optional, Tuple, Union

import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import pandas as pd
from mpl_toolkits.axes_grid1 import make_axes_locatable
from mpl_toolkits.axes_grid1.axes_divider import AxesDivider

params = ["g", "b", "a", "c"]
instance = params + ["seed"]
static = instance + ["makespan", "n_literals"]

needed = static + ["config", "result", "t_total"]

N = 10


def process(
    data: Dict[
        str, Dict[Union[int, str], Tuple[float, float]]
    ],  # FIXME: Key type should'nt be a union
    filename: str,
    ylabel: str,
    simple: bool = False,
    lloc: str = "upper right",
):
    filename = "out/" + filename
    axMain: plt.Axes = plt.gca()
    divider: AxesDivider = make_axes_locatable(axMain)
    axLin: Optional[plt.Axes] = (
        None if simple else divider.append_axes("top", size=1.2, pad=0, sharex=axMain)
    )

    colors = plt.rcParams["axes.prop_cycle"].by_key()["color"]
    c = 0

    for i in range(1, 10 + 1):
        d = data[i].values()
        partial = [math.nan] + list(map(lambda x: x[0], d))
        axMain.plot(partial, linestyle="dashed", color=colors[c])
        full = [math.nan] + list(map(lambda x: math.nan if x[1] > 0.5 else x[0], d))
        axMain.plot(
            full, color=colors[c], marker="D", markersize=3, label=i if simple else None
        )
        if axLin:
            axLin.plot(
                [math.nan] + list(map(lambda x: x[1] if x[1] > 0 else 0, d)),
                color=colors[c],
            )
            with_marker = [math.nan] + list(
                map(lambda x: x[1] if x[1] > 0.03 else math.nan, d)
            )
            axLin.plot(with_marker, color=colors[c], marker="D", markersize=2, label=i)
        axMain.plot(i, data[i][i][0], "o", color=colors[c])
        c += 1

    full = [math.nan] + list(
        map(lambda x: math.nan if x[1] > 0.5 else x[0], data["r"].values())
    )
    axMain.plot(full, color="black", ls="dashdot", lw=2.5)

    if axLin:
        axMain.set_yscale("log")
        axMain.spines["top"].set_visible(False)
        axLin.spines["bottom"].set_visible(False)
        axLin.xaxis.set_ticks_position("top")
        plt.setp(axLin.get_xticklabels(), visible=False)
        axLin.axhline(y=0, color="black")
        axLin.set_ylabel("Unsolved (%)")

    axMain.set_ylabel(ylabel)
    axMain.set_xlabel("a - Number of agents")
    axMain.xaxis.set_major_locator(ticker.MultipleLocator(1))
    if axLin:
        axLin.yaxis.set_minor_locator(ticker.MultipleLocator(0.1))
        axLin.yaxis.set_major_formatter(ticker.PercentFormatter(1, symbol=None))
    axMain.yaxis.grid(True)
    if axLin:
        axLin.yaxis.grid(True)
        axLin.legend(
            title="c", prop={"size": 7}, framealpha=1, borderaxespad=1, loc=lloc
        )
    else:
        axMain.legend(title="c", prop={"size": 7}, borderaxespad=1.5, loc=lloc)
    plt.savefig(filename, dpi=300, bbox_inches="tight")
    print("Plot saved to file {}.png".format(filename))
    plt.close()


def load(base_path: str) -> pd.DataFrame:
    df = pd.concat(map(pd.read_csv, glob.glob(base_path + "*.csv")))[needed]

    ps = dict(tuple(df.groupby(params)))
    dfs: List[pd.DataFrame] = []
    for i, s in ps.items():
        if int(i[0]) < 4:
            continue

        s = s[s["result"].isna() | (s["result"] == "Timeout")]
        s = s.sort_values("seed")
        assert len(s) >= N
        s = s.head(N)
        s["t_total"] = s.apply(
            lambda x: pd.NA
            if x["result"] == "Timeout"
            else max(x["t_total"], 1) * 1e-3,
            axis=1,
            result_type="reduce",
        )
        dfs.append(s)

    return pd.concat(dfs)


def get(
    data: pd.DataFrame, field: str = "t_total"
) -> Dict[str, Dict[int, Tuple[float, float]]]:

    result: Dict[str, Dict[int, Tuple[float, float]]] = dict()
    ps: Dict[Tuple[int, int], pd.DataFrame] = dict(tuple(data.groupby(["a", "c"])))
    for i, s in ps.items():
        result.setdefault(i[3 - 2], dict())
        result[i[3 - 2]][i[2 - 2]] = (s[field].mean(), s["result"].count() / len(s))

    return result


def do(field: str, kwargs):
    d: Dict[str, Dict[Union[int, str], Tuple[float, float]]] = get(mat, field=field)
    e = get(mapf, field=field)
    d["r"] = dict()
    for k in e.keys():
        d["r"][k] = e[k][0]
    process(d, **kwargs)


def grid(
    data: pd.DataFrame, field: str = "t_total"
) -> List[Tuple[int, float, float, int]]:

    result: List[Tuple[int, float, float, int]] = []
    ps: Dict[int, pd.DataFrame] = dict(tuple(data.groupby("g")))
    for i, s in ps.items():
        result.append((i, s[field].mean(), s[field].std(), s[field].count()))

    return result


if __name__ == "__main__":
    for prefix in ["mat", "fixed", "mapd", "non_blocking"]:
        print(prefix)
        pickle_jar = f"data_{prefix}.p"
        try:
            with open(pickle_jar, "rb") as fp:
                print("Loading from jar")
                mat, mapf = pickle.load(fp)

        except FileNotFoundError:
            print("Loading from CSVs")
            mat, mapf = load(f"data/{prefix}/*/"), load(
                "data/regular_mapf/*/"
            )
            print("Saving to jar")
            with open(pickle_jar, "wb") as fp:
                pickle.dump([mat, mapf], fp)

        print(" g | avg (s)|   σ    |    n")
        for g in grid(mat):
            out = "{:2} | {:6.2f} | {:6.2f} | {:4}"
            print(out.format(*g))

        do(
            "t_total",
            {
                "ylabel": "Runtime (s)",
                "filename": f"runtime{'_' + prefix if prefix else ''}",
            },
        )
        do(
            "makespan",
            {
                "simple": True,
                "ylabel": "Makespan",
                "filename": f"makespan{'_' + prefix if prefix else ''}",
            },
        )
        do(
            "n_literals",
            {
                "simple": True,
                "ylabel": "Number of literals (millions)",
                "filename": f"literals{'_' + prefix if prefix else ''}",
                "lloc": "upper left",
            },
        )
