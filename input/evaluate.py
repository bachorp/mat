import math
import glob
import pickle
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
from mpl_toolkits.axes_grid1 import make_axes_locatable

params   = ['g', 'b', 'a', 'c']
instance = params + ['seed']
static   = instance + ['makespan', 'n_literals']

needed = static + ['config', 'result', 't_total', 'initial_bound', 'lower_bound']

pickle_jar = 'data.p'

N = 10

def process(data: dict, filename: str, ylabel: str, simple=False, lloc='upper right'):

    peqx = []
    peqy = []

    axMain = plt.gca()
    divider = make_axes_locatable(axMain)
    if not simple:
        axLin = divider.append_axes("top", size=1.2, pad=0, sharex=axMain)
        axLin: plt.Axes
    axMain: plt.Axes

    timeout = -1

    colors = plt.rcParams['axes.prop_cycle'].by_key()['color']
    c = 0

    for i in range(1, 10 + 1):
        d = data[i].values()
        partial = [math.nan] + list(map(lambda x: math.nan if x[1] >= .9 else x[0], d))
        axMain.plot(partial, linestyle='dashed', color=colors[c])
        full = [math.nan] + list(map(lambda x: math.nan if x[1] > .5 else x[0], d))
        axMain.plot(full, color=colors[c])
        if not simple:
            axLin.plot([math.nan] + list(map(lambda x: x[1] if x[1] > 0 else 0, d)), color=colors[c])
        axMain.plot(i, data[i][i][0], 'o', color=colors[c], label=i)
        c += 1

    full = [math.nan] + list(map(lambda x: math.nan if x[1] > .5 else x[0], data['r'].values()))
    axMain.plot(full, color='black', ls='dashdot', lw=2.5)

    if not simple:
        axMain.set_yscale('log')
        axMain.spines['top'].set_visible(False)
        axLin.spines['bottom'].set_visible(False)
        axLin.xaxis.set_ticks_position('top')
        plt.setp(axLin.get_xticklabels(), visible=False)
        axLin.axhline(y=0, color='black')
        axLin.set_ylabel('unsolved (%)')

    axMain.set_ylabel(ylabel)
    axMain.set_xlabel('a')
    axMain.xaxis.set_major_locator(ticker.MultipleLocator(1))
    if not simple:
        axLin.yaxis.set_minor_locator(ticker.MultipleLocator(.1))
        axLin.yaxis.set_major_formatter(ticker.PercentFormatter(1, symbol=None))
    axMain.yaxis.grid(True)
    if not simple:
        axLin.yaxis.grid(True)
        axLin.legend(range(1, 10 + 1), title='c', prop={'size': 7}, framealpha=1, borderaxespad=1, loc=lloc)
    else:
        axMain.legend(title='c', prop={'size': 7}, borderaxespad=1.5, loc=lloc)
    plt.savefig(filename, dpi=300, bbox_inches = 'tight')
    print("Plot saved to file {}.png".format(filename))
    plt.close()

def load(base_path: str) -> pd.DataFrame:

    df = pd.concat(map(pd.read_csv, glob.glob(base_path + '*.csv')))[needed]

    df = df[df['g'] > 3]
    ps = dict(tuple(df.groupby(params)))
    dfs = []
    for i, s in ps.items():
        s = s[s['result'].isna() | (s['result'] == 'Timeout')]
        s = s.sort_values('seed')
        assert(len(s) >= N)
        s = s.head(N)
        s['t_total'] = s.apply(lambda x: pd.NA if x['result'] == 'Timeout' else max(x['t_total'], 1) * 1e-3, axis=1, result_type='reduce')
        dfs.append(s)

    return pd.concat(dfs)

def get(data: pd.DataFrame, field='t_total') -> dict:

    result = dict()
    ps = dict(tuple(data.groupby(['a', 'c'])))
    for i, s in ps.items():
        result.setdefault(i[3 - 2], dict())
        result[i[3 - 2]][i[2 - 2]] = (s[field].mean(), s['result'].count() / len(s))

    return result

def do(field: str, kwargs):
    d = get(mat, field=field)
    e = get(mapf, field=field)
    d['r'] = dict()
    for k in e.keys():
        d['r'][k] = e[k][0]
    process(d, **kwargs)

def lucky(data: pd.DataFrame) -> int:
    equals = len(data[data['initial_bound'] == data['makespan']])
    higher = len(data[data['lower_bound'] > data['initial_bound']])
    dunno  = len(data) - higher - equals
    return equals, higher, dunno

def avgrelstdpar(data: pd.DataFrame, field='t_total') -> float:

    result = []
    ps = dict(tuple(data.groupby(params)))
    for _, s in ps.items():
        if (s[field].count() == N):
            result.append(s[field].std() / s[field].mean())

    return sum(result) / len(result)

def grid(data: pd.DataFrame, field='t_total') -> float:

    result = []
    ps = dict(tuple(data.groupby('g')))
    for i, s in ps.items():
        result.append((i, s[field].mean(), s[field].std(), s[field].count()))

    return result


if __name__ == '__main__':
    try:
        with open(pickle_jar, 'rb') as fp:
            print("Loading from jar")
            mat, mapf = pickle.load(fp)
    except FileNotFoundError:
        print("Loading from CSVs")
        mat, mapf = load('*/'), load('regular_mapf/*/')
        print("Saving to jar")
        with open(pickle_jar, 'wb') as fp:
            pickle.dump([mat, mapf], fp)

    out = "In {} cases the makespan is equal to the initial bound, in {} cases it is higher, in {} cases we don't know."
    print(out.format(*lucky(mat)))
    print("For instances not solved within half a second " + out.lower().format(*lucky(mat[~(mat['t_total'] < .5)])))

    print(" g | avg (s)|   σ    |    n")
    for g in grid(mat):
        out = "{:2} | {:6.2f} | {:6.2f} | {:4}"
        print(out.format(*g))

    print("The average relative standard deviation of the parameter sets is {:4.2f}%".format(avgrelstdpar(mat) * 100))

    do('t_total', {'ylabel': 'runtime (s)', 'filename': 'runtime'})
    do('makespan', {'simple': True, 'ylabel': 'makespan', 'filename': 'makespan'})
    do('n_literals', {'simple': True, 'ylabel': 'literals', 'filename': 'literals', 'lloc': 'upper left'})
