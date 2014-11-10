#!/bin/bash

echo <<LimitString "
\begin{table}
\centering
\small
\begin{tabular}{@{}ccccc@{}} \toprule
&\multicolumn{4}{c}{CPU Cores Active} \\\\ \\cmidrule(r){2-5}
Frequency (GHz) & 1 & 2 & 3 & 4 \\\\ \midrule "
LimitString
tail -n+2 Data/digest.csv | sed 's/\,/ \& /g' | sed 's/$/ \\\\ /g'

echo <<TailString "
\\bottomrule
\end{tabular}
\end{table} "
TailString
