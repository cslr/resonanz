#!/bin/bash
#
# learn neural networks from datasets
# uses dinrhiw-tools (tst)

nonempty=0
total=0

for i in datasets/*.ds; do
    let total++;
    if $(test ! -s $i); then continue; fi # filter out empty files
    
    # filter out near empty datasets
    output=$(dstool -list $i);
    points=$(echo $output | perl -pe "s/.*?(\d+) points.*/\1/s");
    
    if test $points -ge 20; then # non-empty dataset with more than 20 data points
	# adds preprocessing if needed
	dstool -padd:0:meanvar $i;
	dstool -padd:0:pca $i;
	dstool -padd:1:meanvar $i;

	# calculates nnetwork name
	network=$(echo $i | perl -pe 's/datasets/nnetworks/' | perl -pe 's/ds/cfg/');
	
	# learns from the data and test the results
	nntool $i ?-10-? $network grad
	nntool $i ?-10-? $network use
	
	let nonempty++;
    fi
    
done

echo "$nonempty datasets processed ($total datasets total)";
