import glob
import math
import pickle
import pandas as pd
import scipy.stats
import itertools
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

def plot(xlabel: str, ylabel: str, filename: str, legend=True):
    xpoints = ypoints = plt.xlim()
    plt.plot(xpoints, ypoints, linestyle='-', zorder=1)
    axes = plt.gca()
    axes.yaxis.grid(True, zorder=0)
    axes.xaxis.grid(True, zorder=0)
    plt.plot([1e3, 1e3], [0, 1e3], linestyle='-', color='salmon', zorder=2, lw=1)
    plt.plot([0, 1e3], [1e3, 1e3], linestyle='-', color='salmon', zorder=2, lw=1)
    plt.xscale('log')
    plt.yscale('log')
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.axis('square')

    f = axes.xaxis.get_major_formatter()
    fx = lambda x, pos: '⊥' if pos == 5 else 1 if pos == 2 else 10 if pos == 3 else f.__call__(x, pos)
    axes.xaxis.set_major_locator(ticker.LogLocator())
    axes.xaxis.set_major_formatter(ticker.FuncFormatter(fx))

    f = axes.yaxis.get_major_formatter()
    fy = lambda y, pos: '⊥' if pos == 5 else 1 if pos == 2 else 10 if pos == 3 else f.__call__(y, pos)
    axes.yaxis.set_major_locator(ticker.LogLocator())
    axes.yaxis.set_major_formatter(ticker.FuncFormatter(fy))

    if axes.get_legend_handles_labels()[0]:
        plt.legend(framealpha=1)
    plt.savefig(filename, dpi=300, bbox_inches = 'tight')
    print("Plot saved to file {}.png".format(filename))
    plt.close()

params   = ['g', 'b', 'a', 'c']
instance = params + ['seed']
static   = instance + ['makespan']

needed = static + ['config', 'result', 't_total']

pickle_jar = 'data.p'

encodings = [(4, 0), (3, 1), (5, 2), (6, 3)]

tail = '|600|4|1|1'

def load(encoding=False) -> dict:

    if encoding:
        ds = []
        for i, j in encodings:
            d = pd.concat(map(pd.read_csv, glob.glob(str(i) + '/*.csv')))
            d['config'] = j
            ds.append(d)
        df = pd.concat(ds)[needed]
    else:
        df = pd.concat(map(pd.read_csv, glob.glob('[0-3]/*.csv')))[needed]

    cs = dict(tuple(df.groupby(['config'])))

    p = pd.DataFrame(columns=static)
    for c, d in cs.items():
        p = p.merge(d[d['result'].isna()][static], on=static, how='outer')
        assert(not any(p[instance].duplicated()))

    result = dict()

    for c, d in cs.items():
        ps = dict(tuple(d.groupby(params)))
        for i, s in ps.items():
            s = s[s['result'].isna() | (s['result'] == 'Timeout')]
            t = s.apply(lambda x: pd.NA if x['result'] == 'Timeout' else max(x['t_total'], 1), axis=1, result_type='reduce')
            result.setdefault(i, dict())
            result[i][c] = (t.mean(), t.std())

    return result

cyc = itertools.cycle(['o', 'x'])

def do(data: dict, cf1, cf2, label=None):

    if label is not None:
        cf1 += tail
        cf2 += tail

    x = []
    y = []

    logs = []

    s1 = 0
    s2 = 0

    for i, r in data.items():
        v1 = r[cf1][0]
        v2 = r[cf2][0]

        if not pd.isna(v1):
            s1 += 1
        if not pd.isna(v2):
            s2 += 1

        if (pd.isna(v1)):
            v1 = 1e6
        if (pd.isna(v2)):
            v2 = 1e6

        logs.append(math.log2(v2) - math.log2(v1))

        if any(map(lambda x: r[x][0] < 5e2, configurations)):
            continue

        x.append(v1 / 1e3)
        y.append(v2 / 1e3)

    plt.plot(x, y, next(cyc), label=label, markersize=2.5, zorder=5)

    return sum(logs), s1, s2, mwu(data, cf1, cf2)

def prepare(data: dict, cf):
    result = []
    for i, r in data.items():
        result.append(1e6 if pd.isna(r[cf][0]) else r[cf][0])
    return result

def mwu(data: dict, cf1, cf2):
    vs1 = prepare(data, cf1)
    vs2 = prepare(data, cf2)
    return scipy.stats.mannwhitneyu(vs2, vs1, alternative='greater').pvalue

if __name__ == '__main__':

    result = enc = None

    try:
        with open(pickle_jar, 'rb') as fp:
            print("Loading from jar")
            result, enc = pickle.load(fp)
    except FileNotFoundError:
        print("Loading from CSVs")
        result = load()
        enc = load(True)
        print("Saving to jar")
        with open(pickle_jar, 'wb') as fp:
            pickle.dump([result, enc], fp)

    plt.rcParams['axes.xmargin'] = 1e-2
    plt.rcParams['axes.ymargin'] = 1e-2

    configurations = list(map(lambda x: x + tail, ['0|2', '1|2', '0|1.5', '1|1.5']))

    out = "{} vs. {}: Sum of difference of logarithms is {:.2f}, {} vs. {} solved ({:+}). p = {:.5f}"

    i, s1, s2, p = do(result, '1|1.5', '0|1.5', label='f = 1.5')
    print(out.format("f = 1.5: With", "without preprocessing", i, s1, s2, s1 - s2, p))
    i, s1, s2, p = do(result, '1|2', '0|2', label='f = 2')
    print(out.format("f = 2:   With", "without preprocessing", i, s1, s2, s1 - s2, p))
    plot('Preprocessed', 'Not preprocessed', 'p')

    i, s1, s2, p = do(result, '1|1.5', '1|2', label='preprocessed')
    print(out.format("With    preprocessing: f = 1.5", "f = 2", i, s1, s2, s1 - s2, p))
    i, s1, s2, p = do(result, '0|1.5', '0|2', label='not preprocessed')
    print(out.format("Without preprocessing: f = 1.5", "f = 2", i, s1, s2, s1 - s2, p))
    plot('f = 1.5', 'f = 2', 'f')

    i, s1, s2, p = do(result, '1|1.5', '0|2', label='')
    print(out.format("With optimizations", "without", i, s1, s2, s1 - s2, p))
    next(cyc)
    plot('Optimized', 'Unoptimized', 'o')

    configurations = [0, 1, 2, 3]

    i, s1, s2, p = do(enc, 1, 0)
    print(out.format("Sequential AMO (1)", "binomial AMO (0)", i, s1, s2, s1 - s2, p))
    next(cyc)
    plot('Sequential AMO', 'Binomial AMO', 'amo', False)

    i, s1, s2, p = do(enc, 2, 1)
    print(out.format("With edge variables (2)", "without edge variables (1)", i, s1, s2, s1 - s2, p))
    next(cyc)
    plot('With edge variables', 'Without edge variables', 'ev', False)

    i, s1, s2, p = do(enc, 3, 2)
    print(out.format("With movement variables (3)", "without movement variables (2)", i, s1, s2, s1 - s2, p))
    next(cyc)
    plot('With movement variables', 'Without movement variables', 'mv', False)

    for i, j in encodings:
        d = pd.concat(map(pd.read_csv, glob.glob(str(i) + '/*.csv')))
        out = "Encoding {} needs {:9.1f} clauses, {:9.1f} literals and {:8.1f} variables on average"
        print(out.format(j, d['n_clauses'].mean(), d['n_literals'].mean(), d['n_variables'].mean()))
