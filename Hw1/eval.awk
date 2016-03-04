#!/usr/bin/awk -f
BEGIN {
	init = 0; count = 0;
	sum = 0; sum2 = 0;
}
/^[0-9]+\.[0-9]+/ {
	if(init != 0) {
		count = count + 1;
		target = init + count * 0.1;
		delta = $1 - target;
		if(delta < 0)
			delta = -delta;
		sum  = sum  + delta;
		sum2 = sum2 + delta * delta;
	} else {
		init = $1;
	}
}
END {
	avg = sum / count;
	var = sum2 / count - avg * avg;
	print count " samples: average = " avg "; variance = " var;
}
