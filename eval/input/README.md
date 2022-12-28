This directory contains a set of test data used to evaluate the effect of input characteristics.
We generated 20 instances per parameter set (4 ≤ g ≤ 12, b ∈ {10, 20}, 1 ≤ a, c ≤ 10).
For the evaluation we included exactly 10 instances for each parameter set none of which was recognized as unsolvable.

You can generate the plots and table from the associated subsection of the paper as follows.

```shell
python3 evaluate.py > out.txt
```
To speed up consecutive evaluations, the data are serialized and stored into the file `data.p`. Remove this file if data change.

Requires Pandas and Matplotlib.
