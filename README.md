# ministat
A small tool to do the statistics legwork on benchmarks etc. The goal is to optimize ministat to take advantage modern CPU features and compiler optimizations.

## COMPILE

In the folder of ministat.c, simply enter: ``` $ make ```

The executable will be ```./ministat``` found in the same folder.

## Table of Contents
- [ministat](#ministat)
	- [COMPILE](#compile)
	- [Table of Contents](#table-of-contents)
	- [Milestones](#milestones)
		- [Implement micro-optimizations](#implement-micro-optimizations)
		- [Validate performance improvements and Visualizations](#validate-performance-improvements-and-visualizations)
			- [Data Specification](#data-specification)
			- [Visualizations](#visualizations)
		- [Switch to a multi-threaded architecture](#switch-to-a-multi-threaded-architecture)
		- [Implement parallel sorting and quicksort](#implement-parallel-sorting-and-quicksort)
		- [Implement integer mode](#implement-integer-mode)
		- [Final Analysis](#final-analysis)
	- [Contributors](#contributors)

## Milestones

### Implement micro-optimizations
- [x] a. Implement a new data structure for inserting new data points.
<p>: Initially, change the algorithm to just use <strong>realloc</strong> without using calloc or memcpy.</p>

*Before*:
```c
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
```c
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

- [x] b. Use *an_qsort* to implement a final merge sort.
<p>: Implemented <code>#include "an_qsort.inc"</code></p>

*Before*:

```c
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

```c
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
<p>: Instead of implementing a new option '-v', we put <code>clock_gettime()</code> so that we can collect timing information that we're interested in optimizing.</p>

*After*:

```c
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
<p>: Implemented <code>#include "dtoa/strtod-lite.c"</code>. <code>strtod_fast()</code> converts string to double, use FE_TONEAREST mode</p>

*Before*:

```c
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

*After*:

```c
	d = strtod_fast(t, &p);
	if (strcspn(p, context->file->delim))
		err(2, "Invalid data on line %d in %s\n", line, context->file->n);
	if (*str != '\0')
		AddPoint(ms, d);
```

- [x] e. Implement a raw I/O interface using read, write, open and close.
<p>: Provide real timing data demonstrating the new parsing is better. Benchmark against multiple block sizes.</p>

*After*:

```c
// requires #include <fcntl.h>
static struct dataset *
ReadSet(const char *n, int column, const char *delim)
{
   int f;
   char buf[BUFSIZ], str[BUFSIZ + 25], *p, *t;
   struct dataset *s;
   double d;
   int line;
   int i;
   int bytes_read;
   size_t offset = 0;

   if (n == NULL) {
       f = STDIN_FILENO;
       n = "<stdin>";
   } else if (!strcmp(n, "-")) {
       f = STDIN_FILENO;
       n = "<stdin>";
   } else {
       f = open(n, O_RDONLY);
   }
   if (f == -1)
       err(1, "Cannot open %s", n);
   s = NewSet();
   s->name = strdup(n);
   line = 0;
   for (;;) {
       bytes_read = read(f, buf, sizeof buf - 1);

       if (bytes_read <= 0) {
           break;
       }

       buf[bytes_read] = '\0';
       char *c = buf;
       char *str_start = c;

       for (; *c != '\0'; ++c) {
           if (*c == '\n') {
               line++;
               *c = '\0';
               strcpy(str + offset, str_start);
               offset = 0;
               i = strlen(str);

               for (i = 1, t = strtok(str, delim);
                   t != NULL && *t != '#';
                   i++, t = strtok(NULL, delim)) {
                   if (i == column)
                       break;
               }
               if (t == NULL || *t == '#')
                   continue;

               d = strtod(t, &p);
               if (p != NULL && *p != '\0')
                   err(2, "Invalid data on line %d in %s\n", line, n);
               if (*str != '\0')
                   AddPoint(s, d);

               str_start = c + 1;
           }
       }

       if (buf[bytes_read - 1] != '\0') {
           strcpy(str, str_start);
           offset = strlen(str);
       }
   }
   ....
}

```

- [x] f. Implement more efficient string tokenization.
<p>: Provide real timing data demonstrating the new parsing is better. We replaced strtok with <code>strsep</code> and then we replaced strsep with <code>strcspn</code> again.(strtok() -> strsep() -> strcspn()) </p>

*Before*:

```c
static void *
ReadSet(const char *n, int column, const char *delim)
{
	clock_gettime(CLOCK_MONOTONIC, &start);
	int f;
	char buf[BUFSIZ], str[BUFSIZ + 25], *p, *t;
	struct dataset *s;
	double d;
	int line;
	...
	.....
	*c = '\0';
	strcpy(str + offset, str_start);
	offset = 0;
	i = strlen(str);

	for (i = 1, t = strtok(str, delim);
		t != NULL && *t != '#';
		i++, t = strtok(NULL, delim)) {
		if (i == column)
			break;
	}
	....
	err(2, "Invalid data on line %d in %s\n", line, n);

	if (*str != '\0')
		AddPoint(s, d);

	str_start = c + 1;
    }
	.....

}

```

*After(strtok() -> strsep())*:

```c
static void *
ReadSet(const char *n, int column, const char *delim)
{
	clock_gettime(CLOCK_MONOTONIC, &start);
	int f;
	char buf[BUFSIZ], str[BUFSIZ + 25], *p, *t, *str_0;
	struct dataset *s;
	double d;
	int line;
	...
	.....
	*c = '\0';
	strcpy(str + offset, str_start);
	offset = 0;
	str_start = c + 1;
	str_0 = str;

	for (i = 1, t = strsep(&str_0, delim);
		t != NULL && *t != '#';
		i++, t = strsep(&str_0, delim)) {
		if (i == column)
			break;
	}
	if (t == NULL || *t == '#')
		continue;

	d = strtod_fast(t, &p);
	if (p != NULL && *p != '\0')
		err(2, "Invalid data on line %d in %s\n", line, n);

	if (*str != '\0')
		AddPoint(s, d);

    }
	.....

}
```

*After(strsep() -> strcspn())*:

```c
static void *
ReadSet(const char *n, int column, const char *delim)
{
	clock_gettime(CLOCK_MONOTONIC, &start);
	int f;
	char buf[BUFSIZ], str[BUFSIZ + 25], *p, *t;
	struct dataset *s;
	double d;
	int line;
	...
	.....
	*c = '\0';
	strcpy(str + offset, str_start);
	offset = 0;
	str_start = c + 1;
	str_0 = str;

	for(i = 1, t = p = str; *t != '#'; i++){
		t = p;
		p += strcspn(p, delim);
		if (*p != '\0')
			p++;
		if (i == column)
			break;
	}
	if (t == p || *t == '#')
		continue;

	d = strtod_fast(t, &p);
	if (strcspn(p, delim))
		err(2, "Invalid data on line %d in %s\n", line, n);

	if (*str != '\0')
		AddPoint(s, d);

    }
	.....

}
```

### Validate performance improvements and Visualizations
#### Data Specification
We are testing three main components in these graphs below:
- timing data on various test file sizes (lines) during different stages of micro-optimizations
- timing data on various test file sizes (lines) during different stages of parallelization
- timing data on various test file arguments given to files from origin ministat.c to improved ministat.c

Using a test files generator file, included in our report, we generated 11 files. Starting from the smallest file with 2^16 lines, it will generate up to the biggest file size, 2^26 lines.

__These numbers were executed with the *time utility* and using the following -q command:__
	```
	time ./ministat -q testfile ....
	```

#### Visualizations
- Original ministat vs. with-optimizations 1.a - 1.f

*timing data on various test file sizes (lines) during different stages of micro-optimizations*
<p><strong> Linear Scale </strong></p>
<img src="/images/linear_scale_1.png" width="70%" />

<p><strong> Logarithmic Scale </strong></p>
<img src="/images/logarithmic_scale_1.png" width="70%" />

- Original ministat vs. with parallel file reading & parsing (4 threads) vs. with parallel file sorting

*timing data on various test file sizes (lines) during different stages of parallelization*
<p><strong> Linear Scale </strong></p>
<img src="/images/linear_scale_4_threads.png" width="70%" />

<p><strong> Logarithmic Scale </strong></p>
<img src="/images/logarithmic_scale_4_threads.png" width="70%" />

- Timing data of original ministat vs. parallelized ministat

*timing data on various test file arguments given to files from origin ministat.c to improved ministat.c*
<p><strong> Linear Scale </strong></p>
<img src="/images/parallelized_linear.png" width="70%" />

<p><strong> Logarithmic Scale </strong></p>
<img src="/images/parallelized_log.png" width="70%" />


### Switch to a multi-threaded architecture

- We removed the mutex when adding the new miniset

*Before*:

```
struct readsetworker_context {
	struct readset_context *file;
	size_t start, end;
	struct dataset *s;
};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void *
ReadSetWorker(void *readsetworker_context)
{
	struct readsetworker_context *context = readsetworker_context;
	struct miniset *ms = NewMiniSet();
	char buf[BUFSIZ], str[BUFSIZ + 25], *p, *t;
	double d;		double d;
	int line = 0;
	....
	..
}

static void *
ReadSetWorker(void *readsetworker_context)
{
	...
	....
	pthread_mutex_lock(&mutex);
	AddMiniSet(context->s, ms);
	pthread_mutex_unlock(&mutex);

	return NULL;
}

static void *
ReadSet(void *readset_context)
{
	ctx_start = 0;
	ctx_end = share;

	pthread_t threads[READSET_THREAD_COUNT];
	pthread_t *t = threads;
	size_t thread_count = 0;
	.....
	...

	struct readsetworker_context *worker_context = malloc(sizeof *worker_context);

	...
	.....
	if (pthread_join(*t++, NULL) != 0) {
			err(1, "Failed to join a ReadSetWorker thread");
	}
    }
    close(f);

}

```

*After*:

```
struct readsetworker_context {
	...
	struct miniset *m;
};

static void *
ReadSetWorker(void *readsetworker_context)
{
	struct readsetworker_context *context = readsetworker_context;
	struct miniset *ms = context->m = NewMiniSet();

	....
	..
}

static void *
ReadSetWorker(void *readsetworker_context)
{
	...
	....
	pthread_mutex_lock(&mutex);
	AddMiniSet(context->s, ms);
	pthread_mutex_unlock(&mutex);

	return NULL;
}

static void *
ReadSet(void *readset_context)
{
	ctx_start = 0;
	ctx_end = share;

	struct readsetworker_context *workers[READSET_THREAD_COUNT];
	pthread_t threads[READSET_THREAD_COUNT];
	pthread_t *t = threads;
	.....
	...

	struct readsetworker_context *worker_context = workers[i] = malloc(sizeof *worker_context);

	...
	.....
	if (pthread_join(*t++, NULL) != 0) {
			err(1, "Failed to join a ReadSetWorker thread");
	}

	AddMiniSet(s, workers[i]->m);
    }
    close(f);

}
```
- Parallelize appending of miniset points in *ReadSet* instead of *ReadSetWorker*.

*Before*:

```
struct miniset {
	struct arraylist *head, *tail;
	double sy, syy;
	unsigned n;
	struct miniset *next;
};

static void *
ReadSetWorker(void *readsetworker_context)
{
 	struct readsetworker_context *context = readsetworker_context;
	struct miniset *ms = context->m = NewMiniSet();
	char buf[BUFSIZ], str[BUFSIZ + 25], *p, *t;
	double d;
	int line = 0;
	....

	return NULL;
}

static void *
ReadSet(void *readset_context)
{
   ...
   double *sp = s->points;

   for (struct miniset *ms = s->head; ms != NULL; ms = ms->next) {
	for (struct arraylist *al = ms->head; al != NULL; al = al->next) {
		memcpy(sp, al->points, al->n * sizeof *sp);
		sp += al->n;
	}
  }
  ..
  ....
```

*After*:

```
struct miniset {
	struct arraylist *head, *tail;
	double *points;
	double sy, syy;
	unsigned n;
	struct miniset *next;
};

static void *
ReadSetWorker(void *readsetworker_context)
{
	struct readsetworker_context *context = readsetworker_context;
	struct miniset *ms = context->m = NewMiniSet();
	char buf[BUFSIZ], str[BUFSIZ + 25], *p, *t;
	double d, *points;
	int line = 0;
	....
	points = ms->points = malloc(ms->n * sizeof *points);

	for (struct arraylist *al = ms->head; al != NULL; al = al->next) {
		memcpy(points, al->points, al->n * sizeof *points);
		points += al->n;
	}

	return NULL;
}

static void *
ReadSet(void *readset_context)
{
   ...
   double *sp = s->points;

   for (struct miniset *ms = s->head; ms != NULL; ms = ms->next) {
		memcpy(sp, ms->points, ms->n * sizeof *sp);
   }
  ..
  ....

```

### Implement parallel sorting and quicksort
- We used *dataset* instead of using miniset.

*After*:

```
struct dataset {
	char *name;
	double *points;
	double sy, syy;
	unsigned n;
	struct arraylist *head, *tail;
}

static void
Addpoint(struct dataset *ds, double a)
{
	 clock_gettime(CLOCK_MONOTONIC, &start);
	 if (ds->tail->n >= ARRAYLIST_SIZE) {
	 	ds->tail = ds->tail->next = NewArrayList();
	 }
	 ds->tail->points[ds->tail->n++] = a;
	 ds->sy += a;
	 ds->syy += a * a;
	 ds->n += 1;
	 ....

}
```

### Implement integer mode

// an_qsort_inc -> an_qsort_int.inc file

*After*:

```
#define AN_QSORT_SUFFIX D
...
#define AN_QSORT_TYPE long int
...
.....
```

// ministat.c file

*After*:

```
struct arraylist_int {
	long int *points;
	unsigned n;
	struct arraylist_int *next;
};

struct arraylist_int *
NewArrayList_Int(void)
{
	struct arraylist_int *al = calloc(1, sizeof *al);
	al->points = calloc(ARRAYLIST_SIZE, sizeof *al->points);
	return al;
}

struct dataset_int {
	char *name;
	long int *points;
	long long int sy, syy;
	unsigned n;
	struct arraylist_int *head, *tail;
};

static void
AddPoint_Int(struct dataset_int *ds, long int a)
{
	clock_gettime(CLOCK_MONOTONIC, &start); //------------ time point start ------------//
	if (ds->tail->n >= ARRAYLIST_SIZE) {
		ds->tail = ds->tail->next = NewArrayList_Int();
	}
	ds->tail->points[ds->tail->n++] = a;
	ds->sy += a;
	ds->syy += a * a;
	ds->n += 1;
	clock_gettime(CLOCK_MONOTONIC, &stop); //------------ time point stop ------------//
	ts[0] = elapsed_us(&start, &stop);
}

static void
DataSetUnion_Int(struct dataset_int *destination, struct dataset_int *source)
{
	destination->sy += source->sy;
	destination->syy += source->syy;
	destination->n += source->n;
}

static struct dataset_int *
NewDataSet_Int(void)
{
	struct dataset_int *ds;

	ds = calloc(1, sizeof *ds);
	return(ds);
}

static long int
Min_Int(struct dataset_int *ds)
{

	return (ds->points[0]);
}

static long int
Max_Int(struct dataset_int *ds)
{

	return (ds->points[ds->n - 1]);
}

static double
Avg_Int(struct dataset_int *ds)
{

	return((double)ds->sy / ds->n);
}

static double
Avg_Int(struct dataset_int *ds)
{

	return((double)ds->sy / ds->n);
}

static double
Var_Int(struct dataset_int *ds)
{
	return (ds->syy - (double)ds->sy * ds->sy / ds->n) / (ds->n - 1);
}

static double
Stddev_Int(struct dataset_int *ds)
{

	return sqrt(Var_Int(ds));
}

static void
Vitals(struct dataset *ds, long int flag)
{
 ....
}

static void
Vitals_Int(struct dataset_int *ds, long int flag)
{

	printf("%c %3d %13ld %13ld %13ld %13.8g %13.8g", symbol[flag],
	    ds->n, Min_Int(ds), Max_Int(ds), Median_Int(ds),
		Avg_Int(ds), Stddev_Int(ds));
	printf("\n");
}

static void
Relative_Int(struct dataset_int *ds, struct dataset_int *rs, int confidx)
{
	double spool, s, d, e, t;
	int i;

	i = ds->n + rs->n - 2;
	if (i > NSTUDENT)
		t = student[0][confidx];
	else
		t = student[i][confidx];
	spool = (ds->n - 1) * Var_Int(ds) + (rs->n - 1) * Var_Int(rs);
	spool /= ds->n + rs->n - 2;
	spool = sqrt(spool);
	s = spool * sqrt(1 / ds->n + 1 / rs->n);
	d = Avg_Int(ds) - Avg_Int(rs);
	e = t * s;

	if (fabs(d) > e) {

		printf("Difference at %.1f%% confidence\n", studentpct[confidx]);
		printf("	%g +/- %g\n", d, e);
		printf("	%g%% +/- %g%%\n", d * 100 / Avg_Int(rs), e * 100 / Avg_Int(rs));
		printf("	(Student's t, pooled s = %g)\n", spool);
	} else {
		printf("No difference proven at %.1f%% confidence\n",
		    studentpct[confidx]);
	}
}

static void
DimPlot_Int(struct dataset_int *ds)
{
	AdjPlot(Min_Int(ds));
	AdjPlot(Max_Int(ds));
	AdjPlot(Avg_Int(ds) - Stddev_Int(ds));
	AdjPlot(Avg_Int(ds) + Stddev_Int(ds));
}

static void
PlotSet_Int(struct dataset_int *ds, int val)
{
	struct plot *pl;
	int i, j, m, x;
	unsigned n;
	int bar;

	pl = &plot;
	if (pl->span == 0)
		return;

	if (pl->separate_bars)
		bar = val-1;
	else
		bar = 0;

	if (pl->bar == NULL) {
		pl->bar = malloc(sizeof(char *) * pl->num_datasets);
		memset(pl->bar, 0, sizeof(char*) * pl->num_datasets);
	}
	if (pl->bar[bar] == NULL) {
		pl->bar[bar] = malloc(pl->width);
		memset(pl->bar[bar], 0, pl->width);
	}

	m = 1;
	i = -1;
	j = 0;
	for (n = 0; n < ds->n; n++) {
		x = (ds->points[n] - pl->x0) / pl->dx;
		if (x == i) {
			j++;
			if (j > m)
				m = j;
		} else {
			j = 1;
			i = x;
		}
	}
	m += 1;
	if (m > pl->height) {
		pl->data = realloc(pl->data, pl->width * m);
		memset(pl->data + pl->height * pl->width, 0,
		    (m - pl->height) * pl->width);
	}
	pl->height = m;
	i = -1;
	for (n = 0; n < ds->n; n++) {
		x = (ds->points[n] - pl->x0) / pl->dx;
		if (x == i) {
			j++;
		} else {
			j = 1;
			i = x;
		}
		pl->data[j * pl->width + x] |= val;
	}
	if (!isnan(Stddev_Int(ds))) {
		x = ((Avg_Int(ds) - Stddev_Int(ds)) - pl->x0) / pl->dx;
		m = ((Avg_Int(ds) + Stddev_Int(ds)) - pl->x0) / pl->dx;
		pl->bar[bar][m] = '|';
		pl->bar[bar][x] = '|';
		for (i = x + 1; i < m; i++)
			if (pl->bar[bar][i] == 0)
				pl->bar[bar][i] = '_';
	}
	x = (Median_Int(ds) - pl->x0) / pl->dx;
	pl->bar[bar][x] = 'M';
	x = (Avg_Int(ds) - pl->x0) / pl->dx;
	pl->bar[bar][x] = 'A';
}

struct readset_context_int {
	struct dataset_int **multiset;
	int index;
	int fd; /* file descriptor */
	const char *n; /* filename */
	int column;
	const char *delim;
};

struct readsetworker_context_int {
	struct readset_context_int *file;
	size_t start, end; /* file */
	size_t points_start, points_end; /* s->points */
	struct dataset_int *s; /* parent */
	struct dataset_int *m; /* thread */
};

static void *
ReadSetWorker_Int(void *readsetworker_context_int)
{
	struct readsetworker_context_int *context = readsetworker_context_int;
	struct dataset_int *ds = context->m = NewDataSet_Int();
	ds->tail = ds->head = NewArrayList_Int();
	char buf[BUFSIZ], str[BUFSIZ + 25], *p, *t;
	long int d, *point;
	int line = 0;
	int i;
	int bytes_read;
	off_t cursor = context->start;
	size_t offset = 0;
	size_t ctx_size, buflen;

	for (;;) {
		ctx_size = context->end - cursor + 1;
		buflen = BUFSIZ <= ctx_size ? BUFSIZ : ctx_size;
		bytes_read = pread(context->file->fd, buf, buflen - 1, cursor);

		if (bytes_read <= 0) {
			break;
		}

		cursor += bytes_read;
		buf[bytes_read] = '\0';
		char *c = buf;
		char *str_start = c;

		for (; *c != '\0'; ++c) {
			if (*c == '\n') {
				line++;
				*c = '\0';
				strcpy(str + offset, str_start);
				offset = 0;
				str_start = c + 1;

				for (i = 1, t = p = str;
					*t != '#';
					i++) {
					t = p;
					p += strcspn(p, context->file->delim);
					if (*p != '\0')
						p++;
					if (i == context->file->column)
						break;
				}
				if (t == p || *t == '#')
					continue;

				d = strtol(t, &p, 10);
				if (strcspn(p, context->file->delim))
					err(2, "Invalid data on line %d in %s\n", line,
						context->file->n);
				if (*str != '\0')
					AddPoint_Int(ds, d);
			}
		}

		if (buf[bytes_read - 1] != '\0') {
			strcpy(str, str_start);
			offset = strlen(str);
		}
	}

	point = ds->points = malloc(ds->n * sizeof *ds->points);

	for (struct arraylist_int *al = ds->head; al != NULL; al = al->next) {
		memcpy(point, al->points, al->n * sizeof *point);
		point += al->n;
	}

	an_qsort_D(ds->points, ds->n);

	return NULL;
}

static void
Merge_Int(long int *points, size_t start, size_t mid, size_t end)
{
	size_t n, i;
	long int *x, *sorted;

	long int *first_half = points + start;
	long int *second_half = points + mid;
	long int *origin = first_half;
	long int *midpoint = second_half;
	long int *endpoint = points + end;

	n = end - start;
	sorted = malloc(n * sizeof *sorted);

	for (i = 0, x = sorted; i < n; ++i) {
		if (second_half >= endpoint) {
			*(x++) = *(first_half++);
		}
		else if (first_half >= midpoint) {
			*(x++) = *(second_half++);
		}
		else if (*first_half < *second_half) {
			*(x++) = *(first_half++);
		}
		else {
			*(x++) = *(second_half++);
		}
	}

	memcpy(origin, sorted, n * sizeof *origin);
}

static void *
ReadSet_Int(void *readset_context_int)
{
	clock_gettime(CLOCK_MONOTONIC, &start); //------------ time point start ------------//
	struct readset_context_int *context = readset_context_int;
	int f;
	size_t i, j, k, half_step, step;
	struct dataset_int *s;
	s = NewDataSet_Int();
	s->name = strdup(context->n);

	if (context->n == NULL) {
		f = STDIN_FILENO;
		context->n = "<stdin>";
	} else if (!strcmp(context->n, "-")) {
		f = STDIN_FILENO;
		context->n = "<stdin>";
	} else {
		f = open(context->n, O_RDONLY);
	}
	if (f == -1)
		err(1, "Cannot open %s", context->n);

	context->fd = f;

	struct stat stat;
	size_t byte_size, share, leftover, ctx_start, ctx_end;

	fstat(f, &stat);
	byte_size = stat.st_size;
	share = byte_size / READSET_THREAD_COUNT;
	leftover = byte_size % READSET_THREAD_COUNT;
	ctx_start = 0;
	ctx_end = share;

	struct readsetworker_context_int *workers[READSET_THREAD_COUNT];
	pthread_t threads[READSET_THREAD_COUNT];
	pthread_t *t = threads;
	char candidate;

	for (i = 0; i < READSET_THREAD_COUNT; ++i) {
		if (i == 0 && leftover) {
			ctx_end += leftover;
			leftover = 0;
		}

		while (pread(f, &candidate, 1, ctx_end - 1)) {
			if (candidate == '\n') {
				break;
			}

			ctx_end++;
		}

		struct readsetworker_context_int *worker_context = workers[i] = malloc(
			sizeof *worker_context
		);
		worker_context->file = context;
		worker_context->start = ctx_start;
		worker_context->end = ctx_end;
		worker_context->s = s;

		if (pthread_create(t++, NULL, ReadSetWorker_Int, worker_context) != 0) {
			err(1, "Failed to create a ReadSetWorker_Int thread");
		}

		ctx_start = ctx_end;
		ctx_end += share;

		if (ctx_end > byte_size) {
			ctx_end = byte_size;
		}
	}

	size_t points_start, points_end;
	points_start = points_end = 0;

	for (i = 0, t = threads; i < READSET_THREAD_COUNT; ++i) {
		if (pthread_join(*t++, NULL) != 0) {
			err(1, "Failed to join a ReadSetWorker_Int thread");
		}

		workers[i]->points_start = points_start;
		points_end = points_start + workers[i]->m->n;
		workers[i]->points_end = points_end;
		points_start = points_end;

		DataSetUnion_Int(s, workers[i]->m);
	}

	close(f);

	if (s->n < 3) {
		fprintf(stderr,
		    "Dataset %s must contain at least 3 data points\n", context->n);
		exit (2);
	}

	s->points = malloc(s->n * sizeof *s->points);
	long int *point = s->points;

	for (i = 0; i < READSET_THREAD_COUNT; ++i) {
		memcpy(point, workers[i]->m->points, workers[i]->m->n * sizeof *point);
		point += workers[i]->m->n;
	}

	share = s->n / READSET_THREAD_COUNT;
	leftover = s->n %  READSET_THREAD_COUNT;
	ctx_start = 0;
	ctx_end = share;

	for (
			j = 0, k = log2(READSET_THREAD_COUNT), half_step = 1, step = 2;
			j < k;
			j++
		) {
		for (i = 0; i < READSET_THREAD_COUNT; i += step) {
			Merge_Int(
				s->points,
				workers[i]->points_start,
				workers[i + half_step]->points_start,
				workers[i + step - 1]->points_end
			);
		}

		half_step = step;
		step *= 2;
	}

	context->multiset[context->index] = s;

	clock_gettime(CLOCK_MONOTONIC, &stop); //------------ time point stop ------------//
	ts[1] = elapsed_us(&start, &stop);
	return (s);
}

int
main(int argc, char **argv)
{

  ...
  struct dataset_int *ds_int[7];
  ...
  ....
  int flag_i = 0;

  ...
  while ((c = getopt(argc, argv, "C:c:d:snqiw:v")) != -1)
  ...
  ....
  case 'i':
	flag_i = 1;
	break;

  ...
  ......

  if (flag_i) {
			struct readset_context_int context_int;
			context_int.multiset = ds_int;
			context_int.index = 0;
			context_int.n = "-";
			context_int.column = column;



			ReadSet_Int((void *)&context_int);
		}
		else {
			struct readset_context context;
			context.multiset = ds;
			context.index = 0;
			context.n = "-";
			context.column = column;
			context.delim = delim;

			ReadSet((void *)&context);
		}

	....

	for (i = 0; i < nds; i++) {
		if (flag_i) {
			struct readset_context_int *context_int = malloc(sizeof *context_int);
			context_int->multiset = ds_int;
			context_int->index = i;
			context_int->n = argv[i];
			context_int->column = column;
			context_int->delim = delim;

			if (pthread_create(t++, NULL, ReadSet_Int, context_int) != 0) {
					err(1, "Failed to create a ReadSet_Int thread");
				}
			}
			else {
				struct readset_context *context = malloc(sizeof *context);
				context->multiset = ds;
				context->index = i;
				context->n = argv[i];
				context->column = column;
				context->delim = delim;

				if (pthread_create(t++, NULL, ReadSet, context) != 0) {
					err(1, "Failed to create a ReadSet thread");
				}
	for (i = 0, t = threads; i < nds; i++) {
		if (pthread_join(*t++, NULL) != 0) {
			if (flag_i) {
				err(1, "Failed to join a ReadSet_Int thread");
			}
			else {
				err(1, "Failed to join a ReadSet thread");
			}
		}
	}
	for (i = 0; i < nds; i++) {
		if(flag_i){
			printf("%c %s\n", symbol[i+1], ds_int[i]->name);
		}
		else{
			printf("%c %s\n", symbol[i+1], ds[i]->name);
		}
	}

	if (!flag_n && !flag_q) {
		SetupPlot(termwidth, flag_s, nds);
		for (i = 0; i < nds; i++) {
			if(flag_i){
				DimPlot_Int(ds_int[i]);
			}
			else {
				DimPlot(ds[i]);
			}
		}

		for (i = 0; i < nds; i++) {
			if (flag_i) {
				PlotSet_Int(ds_int[i], i + 1);
			}
			else {
				PlotSet(ds[i], i + 1);
			}
		}

		DumpPlot();
	}

	VitalsHead();

	if (flag_i) {
		Vitals_Int(ds_int[0], 1);
	}
	else {
		Vitals(ds[0], 1);
	}

	for (i = 1; i < nds; i++) {
		if (flag_i) {
			Vitals_Int(ds_int[i], i + 1);

			if(!flag_n)
				Relative_Int(ds_int[i], ds_int[0], ci);
		}
		else {
			Vitals(ds[i], i + 1);

			if (!flag_n)
				Relative(ds[i], ds[0], ci);
		}
	}

	......
	...

}
```



### Final Analysis

- Original
```
x game.txt
+ desktop.txt
    N           Min           Max        Median           Avg        Stddev
x 19992880             1 1.6039039e+09           868     166282.14      15806022
+ 15863880             1 1.5813409e+09             4     76146.461     4043837.6
Difference at 95.0% confidence
        -90135.7 +/- 7977.54
        -54.2065% +/- 4.79759%
        (Student's t, pooled s = 1.21051e+07)
real    0m12.140s
user    0m11.783s
sys     0m0.356s
```
- Final Result
```
```



- What improved from the base line?

- What were most beneficial optimizations?

One of the biggest drops in timing data ocurred after we implemented optimization 1.d. Instead of `strtod()` for char to double conversion, we are using `strtod_fast()` found in the `dtoa/` folder in this project, provided by <https://github.com/achan001/dtoa-fast/blob/master/license.txt>.
Another helpful optimization was milestone 1.b. Switching `qsort()` for `an_qsort()`, provided by <https://github.com/appnexus/acf/blob/master/LICENSE>.

The most beneficial optimization came from using threads to parallelize file reading and parsing. It significantly improved performance (see visualization).

Parallelizing input sorting only added slight improvement to overall performance.

- What were least effective?

The least effective was many of the micro-optimizations. There was a visible difference in timing data only when it came down to very large files.

- What challenges you encountered?

One challenge from the beginning was how to split the work up and collaborate on different schedules. In that regard, we all pulled through and supported one another.

Another challenge was making sure we all had the same data for testing so we were on the same page about whether our changes were true optimizations.

It could have helped if many of the faster functions (i.e. an_qsort and strtod_fast) had more documentation on the implementation in practice.

## Contributors

| Name          | Github                                                    |
| ------------- | --------------------------------------------------------- |
| Kangming Deng | <a href="https://github.com/Kamide">@Kamide</a>           |
| Leah Meza     | <a href="https://github.com/leahmezacs">@leahmezacs</a>   |
| Jonathan So   | <a href="https://github.com/Jonathan668">@Jonathan668</a> |
| Jiseon Yu     | <a href="https://github.com/JiseonYu">@JiseonYu</a>       |
