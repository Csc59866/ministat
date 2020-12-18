# ministat
A small tool to do the statistics legwork on benchmarks etc. The goal is to optimize ministat to take advantage modern CPU features and compiler optimizations.

## Milestones 

### 1. Implement micro-optimizations
- [x] a. Implement a new data structure for inserting new data points.
<p>: Initially, change the algorithm to just use <strong>realloc</strong> without using calloc or memcpy.</p>

*Before*:
```
static void
AddPoint(struct dataset *ds, double a)
{
	double *dp;

	if (ds->n >= ds->lpoints) {
		dp = ds->points;
		ds->lpoints *= 4;
		ds->points = calloc(sizeof *ds->points, ds->lpoints);
		memcpy(ds->points, dp, sizeof *dp * ds->n);
		free(dp);
	}
	ds->points[ds->n++] = a;
	ds->sy += a;
	ds->syy += a * a;
}
```

*After*:
```
static void
AddPoint(struct dataset *ds, double a)
{
   if (ds->n >= ds->lpoints) {
       ds->lpoints *= 4;
       ds->points = realloc(ds->points, sizeof(ds->points) * ds->lpoints);
   }
   ds->points[ds->n++] = a;
   ds->sy += a;
   ds->syy += a * a;
}

```

- [x] b. Use an_qsort to implement a final merge sort.
<p>: Implement <i>#include "an_qsort.inc"</i></p>

*Before*:

```
static struct dataset *
ReadSet(const char *n, int column, const char *delim)
{
	...
	.....
	qsort(s->points, s->n, sizeof *s->points, dbl_cmp);
	..

}
```

*After*:

```
static void *
ReadSet(void *readset_context)
{
	...
	......
	
	s->points = malloc(s->n * sizeof *s->points);
	double *sp = s->points;

	for (struct miniset *ms = s->head; ms != NULL; ms = ms->next) {
		memcpy(sp, ms->points, ms->n * sizeof *sp);
	}

	an_qsort_C(s->points, s->n);
	
	...
}	
```

- [x] c. Implement a new option '-v' that emits verbose timing data.
<p>: Instead of implementing a new option '-v', we put <i>clock_gettime()</i> so tha twe can collect timing information that we're interested in optimizing and print it out.</p>

*After*:

```
#include <time.h>
...
static void *
ReadSet(void *readset_context)
{
	clock_gettime(CLOCK_MONOTONIC, &start);
	....
	..
	clock_gettime(CLOCK_MONOTONIC, &stop);
	...	
}

```

- [x] d. Implement an alternative *strtod* function.
<p>Implemented <i>#include "dtoa/strtod-lite.c"</i> </p>

*Before*:

```
static struct dataset *
ReadSet(const char *n, int column, const char *delim)
{
		...
		.....
		d = strtod(t, &p);
		if (p != NULL && *p != '\0')
			err(2, "Invalid data on line %d in %s\n", line, n);
		if (*buf != '\0')
			AddPoint(s, d);
			
}

int
main(int argc, char **argv)
{
...
....
	case 'c':
		a = strtod(optarg, &p);
...

}

```

<p><i>strtod_fast()</i> converts string to double, use FE_TONEAREST mode</p>
*After*:
```
	d = strtod_fast(t, &p);
	if (strcspn(p, context->file->delim))
		err(2, "Invalid data on line %d in %s\n", line, context->file->n);
	if (*str != '\0')
		AddPoint(ms, d);
```

- [x] e. Implement a raw I/O interface using read, write, open and close.
<p>: Provide real timing data demonstrating the new parsing is better. Benchmark against multiple block sizes. Generate visualizations.</p>

*Before*:
```
```

*After*:
```
```
- [x] f. Implement more efficient string tokenization.
<p>: Provide real timing data demonstrating the new parsing is better. Generate visualizations.</p>
*Before*:
```
```

*After*:
```
```

### 2. Validate performance improvements and Visualizations
- Original ministat vs. with-optimizations 1.a., 1.e., and 1.a.+ 1.e.
<p><strong> Linear Scale </strong></p>
<img src="/images/linear_scale_1.png" />

<p><strong> Logarithmic Scale </strong></p>
<img src="/images/logarithmic_scale_1.png"/> 

- Original ministat vs. with parallel file reading and parsing (4 threads). 
<p><strong> Linear Scale </strong></p>
<img src="/images/linear_scale_4_threads.png" />

<p><strong> Logarithmic Scale </strong></p>
<img src="/images/logarithmic_scale_4_threads.png"/> 



### 3. Switch to a multi-threaded architecture

### 4. Implement integer mode

### 5. Validate performance improvements 


## Contributors

| Name          | Github        | 
| ------------- | ------------- | 
| Kangming Deng | <a href="https://github.com/Kamide">@Kamide</a>  | 
| Leah Meza | <a href="https://github.com/leahmezacs">@leahmezacs</a>  |
| Jonathan So | <a href="https://github.com/Jonathan668">@Jonathan668</a>  |	
| Jiseon Yu | <a href="https://github.com/JiseonYu">@JiseonYu</a> | 
