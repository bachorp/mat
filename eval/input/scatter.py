#!/usr/bin/env python3
import pickle
from typing import Any, Dict, List, Literal

import matplotlib.pyplot as plt
import pandas as pd

params = ["g", "b", "a", "c"]
instance = params + ["seed"]

required = instance + ["makespan", "result", "t_total"]


def load(variant: str) -> List[pd.DataFrame]:
    data: List[pd.DataFrame] = []
    for g in range(4, 12 + 1):
        for s in range(20):
            data.append(pd.read_csv(f"{variant}/{g}/{s}.csv")[required])

    return data


Variant = Literal["mat", "non_blocking", "fixed", "mapd"]


def scatter(d0: pd.Series, d1: pd.Series, times: bool):
    assert len(d0) == len(d1)
    points: List[Any] = []
    for i in range(len(d0)):
        # Hack for unsolvable instances
        if pd.isna(d0[i]) or pd.isna(d1[i]):
            continue

        # Hack for timed out instances
        if times:
            points.append(
                (1.5 * 1e6 if d0[i] == 0 else d0[i], 1.5 * 1e6 if d1[i] == 0 else d1[i])
            )

        else:
            points.append((d0[i], d1[i]))

    return points


def plot(variant0: Variant, variant1: Variant, on: str):
    points: List[Any] = []

    d0 = data[variant0]
    d1 = data[variant1]
    assert len(d0) == len(d1)
    for i in range(len(d0)):
        points += scatter(d0[i][on], d1[i][on], on == "t_total")

    df = pd.DataFrame(points, columns=[variant0, variant1])
    df.plot.scatter(x=variant0, y=variant1)
    xpoints = ypoints = plt.xlim()
    plt.plot(xpoints, ypoints, linestyle="-", zorder=1)
    plt.savefig(
        f"scatter_{on}_{variant0}__{variant1}.png", dpi=300, bbox_inches="tight"
    )


if __name__ == "__main__":
    pickle_jar = "data.p"
    variants = ("mat", "non_blocking", "fixed", "mapd")

    try:
        with open(pickle_jar, "rb") as fp:
            print("Loading from jar")
            data: Dict[str, List[pd.DataFrame]] = pickle.load(fp)

    except FileNotFoundError:
        print("Loading from CSVs")
        data = dict(map(lambda x: (x, load(x)), variants))
        print("Saving to jar")
        with open(pickle_jar, "wb") as fp:
            pickle.dump(data, fp)

    for on in ("makespan", "t_total"):
        for v0, v1 in (
            ("mat", "non_blocking"),
            ("mat", "fixed"),
            ("mat", "mapd"),
            ("mapd", "fixed"),
            ("mapd", "non_blocking"),
        ):
            plot(v0, v1, on)
