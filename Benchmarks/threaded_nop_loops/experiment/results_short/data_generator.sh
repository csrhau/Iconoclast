#!/bin/bash

echo "Frequency,One,Two,Three,Four" > header.tmp
for CORES in 1 2 3 4; do
  echo "Processing $CORES"
  grep Power *_${CORES}_* > ${CORES}.tmp
  echo "DONE"
done

#Get Index:
cut -f 2 -d '_' 1.tmp  > left.tmp


for CORES in 1 2 3 4; do
  awk '{print $3}' $CORES.tmp > $CORES.clean.tmp
done 

paste left.tmp 1.clean.tmp 2.clean.tmp 3.clean.tmp 4.clean.tmp > all.tmp

cat header.tmp all.tmp | sed 's/\t/,/g' > results.digest
rm *.tmp
