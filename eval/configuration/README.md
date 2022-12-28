This directory contains a set of test data used to evaluate possible optimizations.
We generated a single instance per parameter set (4 ≤ g ≤ 12, b ∈ {10, 20}, 1 ≤ a, c ≤ 10) per configuration (with and without preprocessing, f ∈ {1.5, 2}).
Additionally, we evaluate the effect of different encodings of the SAT formula.

```shell
python3 evaluate.py > out.txt
```

To speed up consecutive evaluations, the data are serialized and stored into the file `data.p`. Remove this file if data change.
