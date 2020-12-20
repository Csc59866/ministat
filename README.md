# ministat
A small tool to do the statistics legwork on benchmarks etc. The goal is to optimize ministat to take advantage modern CPU features and compiler optimizations.

## Table of Contents
- [Implement micro-optimzations](#1.-implement-micro-optimizations)
- [Validate performance improvements and Visualizations](#2.-validate-performance-improvements-and-visualizations)
- [Switch to a multi-threaded architecture](#3.-switch-to-a-multi-threaded-architecture)
- [Implement parallel sorting and quicksort ](#4.-implement-parallel-sorting-and-quicksort)
- [Final Analysis](#5.-final-analysis)
- [Contributors](#contributors)

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

- [x] b. Use *an_qsort* to implement a final merge sort.
<p>: Implemented <i>#include "an_qsort.inc"</i></p>

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
<p>: Instead of implementing a new option '-v', we put <i>clock_gettime()</i> so that we can collect timing information that we're interested in optimizing.</p>

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
<p>: Implemented <i>#include "dtoa/strtod-lite.c"</i>. <i>strtod_fast()</i> converts string to double, use FE_TONEAREST mode</p>

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

*After*:

```
	d = strtod_fast(t, &p);
	if (strcspn(p, context->file->delim))
		err(2, "Invalid data on line %d in %s\n", line, context->file->n);
	if (*str != '\0')
		AddPoint(ms, d);
```

- [x] e. Implement a raw I/O interface using read, write, open and close.
<p>: Provide real timing data demonstrating the new parsing is better. Benchmark against multiple block sizes.</p>

*After*:

```
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
<p>: Provide real timing data demonstrating the new parsing is better. We replaced strtok with <i>strsep</i> and then we replaced strsep with <i>strcspn</i> again.(strtok() -> strsep() -> strcspn()) </p>

*Before*:

```
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

```
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

```
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

### 2. Validate performance improvements and Visualizations
- Original ministat vs. with-optimizations 1.a - 1.f
<p><strong> Linear Scale </strong></p>
<img src="/images/linear_scale_1.png" width="70%" />

<p><strong> Logarithmic Scale </strong></p>
<img src="/images/logarithmic_scale_1.png" width="70%" /> 

- Original ministat vs. with parallel file reading & parsing (4 threads) vs. with parallel file sorting
<p><strong> Linear Scale </strong></p>
<img src="/images/linear_scale_4_threads.png" width="70%" />

<p><strong> Logarithmic Scale </strong></p>
<img src="/images/logarithmic_scale_4_threads.png" width="70%" /> 

- Timing data of original ministat vs. parallelized ministat
<p><strong> Linear Scale </strong></p>
<img src="/images/parallelized_linear.png" width="70%" />

<p><strong> Logarithmic Scale </strong></p>
<img src="/images/parallelized_log.png" width="70%" /> 


### 3. Switch to a multi-threaded architecture

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

Reastatic void *
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

Reastatic void *
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
- Parallelize appending of miniset points in *ReadSet* instend of *ReadSetWorker*.

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

### 4. Implement parallel sorting and quicksort
- We used *dataset* instead of using miniset. 

*After*:

```
struct dataset {
	char *name;
	double *points;
	duboe sy, syy;
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


### 5. Final Analysis

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

- What were least effective?

- What challenges you encountered?





## Contributors

| Name          | Github        | 
| ------------- | ------------- | 
| Kangming Deng | <a href="https://github.com/Kamide">@Kamide</a>  | 
| Leah Meza | <a href="https://github.com/leahmezacs">@leahmezacs</a>  |
| Jonathan So | <a href="https://github.com/Jonathan668">@Jonathan668</a>  |	
| Jiseon Yu | <a href="https://github.com/JiseonYu">@JiseonYu</a> | 
