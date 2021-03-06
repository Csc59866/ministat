/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 *
 */
#include <sys/ioctl.h>

#include "an_qsort.inc"
#include "an_qsort_int.inc"
#include "dtoa/strtod-lite.c"
#include <err.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "queue.h"

#define ARRAYLIST_SIZE 100000
#define READSET_THREAD_COUNT 4

#define NSTUDENT 100
#define NCONF 6
double const studentpct[] = { 80, 90, 95, 98, 99, 99.5 };
double student [NSTUDENT + 1][NCONF] = {
/* inf */	{	1.282,	1.645,	1.960,	2.326,	2.576,	3.090  },
/* 1. */	{	3.078,	6.314,	12.706,	31.821,	63.657,	318.313  },
/* 2. */	{	1.886,	2.920,	4.303,	6.965,	9.925,	22.327  },
/* 3. */	{	1.638,	2.353,	3.182,	4.541,	5.841,	10.215  },
/* 4. */	{	1.533,	2.132,	2.776,	3.747,	4.604,	7.173  },
/* 5. */	{	1.476,	2.015,	2.571,	3.365,	4.032,	5.893  },
/* 6. */	{	1.440,	1.943,	2.447,	3.143,	3.707,	5.208  },
/* 7. */	{	1.415,	1.895,	2.365,	2.998,	3.499,	4.782  },
/* 8. */	{	1.397,	1.860,	2.306,	2.896,	3.355,	4.499  },
/* 9. */	{	1.383,	1.833,	2.262,	2.821,	3.250,	4.296  },
/* 10. */	{	1.372,	1.812,	2.228,	2.764,	3.169,	4.143  },
/* 11. */	{	1.363,	1.796,	2.201,	2.718,	3.106,	4.024  },
/* 12. */	{	1.356,	1.782,	2.179,	2.681,	3.055,	3.929  },
/* 13. */	{	1.350,	1.771,	2.160,	2.650,	3.012,	3.852  },
/* 14. */	{	1.345,	1.761,	2.145,	2.624,	2.977,	3.787  },
/* 15. */	{	1.341,	1.753,	2.131,	2.602,	2.947,	3.733  },
/* 16. */	{	1.337,	1.746,	2.120,	2.583,	2.921,	3.686  },
/* 17. */	{	1.333,	1.740,	2.110,	2.567,	2.898,	3.646  },
/* 18. */	{	1.330,	1.734,	2.101,	2.552,	2.878,	3.610  },
/* 19. */	{	1.328,	1.729,	2.093,	2.539,	2.861,	3.579  },
/* 20. */	{	1.325,	1.725,	2.086,	2.528,	2.845,	3.552  },
/* 21. */	{	1.323,	1.721,	2.080,	2.518,	2.831,	3.527  },
/* 22. */	{	1.321,	1.717,	2.074,	2.508,	2.819,	3.505  },
/* 23. */	{	1.319,	1.714,	2.069,	2.500,	2.807,	3.485  },
/* 24. */	{	1.318,	1.711,	2.064,	2.492,	2.797,	3.467  },
/* 25. */	{	1.316,	1.708,	2.060,	2.485,	2.787,	3.450  },
/* 26. */	{	1.315,	1.706,	2.056,	2.479,	2.779,	3.435  },
/* 27. */	{	1.314,	1.703,	2.052,	2.473,	2.771,	3.421  },
/* 28. */	{	1.313,	1.701,	2.048,	2.467,	2.763,	3.408  },
/* 29. */	{	1.311,	1.699,	2.045,	2.462,	2.756,	3.396  },
/* 30. */	{	1.310,	1.697,	2.042,	2.457,	2.750,	3.385  },
/* 31. */	{	1.309,	1.696,	2.040,	2.453,	2.744,	3.375  },
/* 32. */	{	1.309,	1.694,	2.037,	2.449,	2.738,	3.365  },
/* 33. */	{	1.308,	1.692,	2.035,	2.445,	2.733,	3.356  },
/* 34. */	{	1.307,	1.691,	2.032,	2.441,	2.728,	3.348  },
/* 35. */	{	1.306,	1.690,	2.030,	2.438,	2.724,	3.340  },
/* 36. */	{	1.306,	1.688,	2.028,	2.434,	2.719,	3.333  },
/* 37. */	{	1.305,	1.687,	2.026,	2.431,	2.715,	3.326  },
/* 38. */	{	1.304,	1.686,	2.024,	2.429,	2.712,	3.319  },
/* 39. */	{	1.304,	1.685,	2.023,	2.426,	2.708,	3.313  },
/* 40. */	{	1.303,	1.684,	2.021,	2.423,	2.704,	3.307  },
/* 41. */	{	1.303,	1.683,	2.020,	2.421,	2.701,	3.301  },
/* 42. */	{	1.302,	1.682,	2.018,	2.418,	2.698,	3.296  },
/* 43. */	{	1.302,	1.681,	2.017,	2.416,	2.695,	3.291  },
/* 44. */	{	1.301,	1.680,	2.015,	2.414,	2.692,	3.286  },
/* 45. */	{	1.301,	1.679,	2.014,	2.412,	2.690,	3.281  },
/* 46. */	{	1.300,	1.679,	2.013,	2.410,	2.687,	3.277  },
/* 47. */	{	1.300,	1.678,	2.012,	2.408,	2.685,	3.273  },
/* 48. */	{	1.299,	1.677,	2.011,	2.407,	2.682,	3.269  },
/* 49. */	{	1.299,	1.677,	2.010,	2.405,	2.680,	3.265  },
/* 50. */	{	1.299,	1.676,	2.009,	2.403,	2.678,	3.261  },
/* 51. */	{	1.298,	1.675,	2.008,	2.402,	2.676,	3.258  },
/* 52. */	{	1.298,	1.675,	2.007,	2.400,	2.674,	3.255  },
/* 53. */	{	1.298,	1.674,	2.006,	2.399,	2.672,	3.251  },
/* 54. */	{	1.297,	1.674,	2.005,	2.397,	2.670,	3.248  },
/* 55. */	{	1.297,	1.673,	2.004,	2.396,	2.668,	3.245  },
/* 56. */	{	1.297,	1.673,	2.003,	2.395,	2.667,	3.242  },
/* 57. */	{	1.297,	1.672,	2.002,	2.394,	2.665,	3.239  },
/* 58. */	{	1.296,	1.672,	2.002,	2.392,	2.663,	3.237  },
/* 59. */	{	1.296,	1.671,	2.001,	2.391,	2.662,	3.234  },
/* 60. */	{	1.296,	1.671,	2.000,	2.390,	2.660,	3.232  },
/* 61. */	{	1.296,	1.670,	2.000,	2.389,	2.659,	3.229  },
/* 62. */	{	1.295,	1.670,	1.999,	2.388,	2.657,	3.227  },
/* 63. */	{	1.295,	1.669,	1.998,	2.387,	2.656,	3.225  },
/* 64. */	{	1.295,	1.669,	1.998,	2.386,	2.655,	3.223  },
/* 65. */	{	1.295,	1.669,	1.997,	2.385,	2.654,	3.220  },
/* 66. */	{	1.295,	1.668,	1.997,	2.384,	2.652,	3.218  },
/* 67. */	{	1.294,	1.668,	1.996,	2.383,	2.651,	3.216  },
/* 68. */	{	1.294,	1.668,	1.995,	2.382,	2.650,	3.214  },
/* 69. */	{	1.294,	1.667,	1.995,	2.382,	2.649,	3.213  },
/* 70. */	{	1.294,	1.667,	1.994,	2.381,	2.648,	3.211  },
/* 71. */	{	1.294,	1.667,	1.994,	2.380,	2.647,	3.209  },
/* 72. */	{	1.293,	1.666,	1.993,	2.379,	2.646,	3.207  },
/* 73. */	{	1.293,	1.666,	1.993,	2.379,	2.645,	3.206  },
/* 74. */	{	1.293,	1.666,	1.993,	2.378,	2.644,	3.204  },
/* 75. */	{	1.293,	1.665,	1.992,	2.377,	2.643,	3.202  },
/* 76. */	{	1.293,	1.665,	1.992,	2.376,	2.642,	3.201  },
/* 77. */	{	1.293,	1.665,	1.991,	2.376,	2.641,	3.199  },
/* 78. */	{	1.292,	1.665,	1.991,	2.375,	2.640,	3.198  },
/* 79. */	{	1.292,	1.664,	1.990,	2.374,	2.640,	3.197  },
/* 80. */	{	1.292,	1.664,	1.990,	2.374,	2.639,	3.195  },
/* 81. */	{	1.292,	1.664,	1.990,	2.373,	2.638,	3.194  },
/* 82. */	{	1.292,	1.664,	1.989,	2.373,	2.637,	3.193  },
/* 83. */	{	1.292,	1.663,	1.989,	2.372,	2.636,	3.191  },
/* 84. */	{	1.292,	1.663,	1.989,	2.372,	2.636,	3.190  },
/* 85. */	{	1.292,	1.663,	1.988,	2.371,	2.635,	3.189  },
/* 86. */	{	1.291,	1.663,	1.988,	2.370,	2.634,	3.188  },
/* 87. */	{	1.291,	1.663,	1.988,	2.370,	2.634,	3.187  },
/* 88. */	{	1.291,	1.662,	1.987,	2.369,	2.633,	3.185  },
/* 89. */	{	1.291,	1.662,	1.987,	2.369,	2.632,	3.184  },
/* 90. */	{	1.291,	1.662,	1.987,	2.368,	2.632,	3.183  },
/* 91. */	{	1.291,	1.662,	1.986,	2.368,	2.631,	3.182  },
/* 92. */	{	1.291,	1.662,	1.986,	2.368,	2.630,	3.181  },
/* 93. */	{	1.291,	1.661,	1.986,	2.367,	2.630,	3.180  },
/* 94. */	{	1.291,	1.661,	1.986,	2.367,	2.629,	3.179  },
/* 95. */	{	1.291,	1.661,	1.985,	2.366,	2.629,	3.178  },
/* 96. */	{	1.290,	1.661,	1.985,	2.366,	2.628,	3.177  },
/* 97. */	{	1.290,	1.661,	1.985,	2.365,	2.627,	3.176  },
/* 98. */	{	1.290,	1.661,	1.984,	2.365,	2.627,	3.175  },
/* 99. */	{	1.290,	1.660,	1.984,	2.365,	2.626,	3.175  },
/* 100. */	{	1.290,	1.660,	1.984,	2.364,	2.626,	3.174  }
};

#define	MAX_DS	8
#define MAX_TS  2
static char symbol[MAX_DS] = { ' ', 'x', '+', '*', '%', '#', '@', 'O' };
static unsigned long long ts[2] = {0,0};
struct timespec start, stop;

static unsigned long long
elapsed_us(struct timespec *a, struct timespec *b)
{
	unsigned long long a_p = (a->tv_sec * 1000000ULL) + a->tv_nsec / 1000;
	unsigned long long b_p = (b->tv_sec * 1000000ULL) + b->tv_nsec / 1000;
	return b_p - a_p;
}

static void
TimePrint(void)
{
	unsigned i;
	printf("Timing Performance     AddPoint 	ReadSet		... 	\n");
	printf("This Week:                   ");
	for(i=0; i<MAX_TS; i++){
		printf("%llu           ", ts[i]);
	}
	printf("\n");
}

struct arraylist {
	double *points;
	unsigned n;
	struct arraylist *next;
};

struct arraylist_int {
	long int *points;
	unsigned n;
	struct arraylist_int *next;
};


struct arraylist *
NewArrayList(void)
{
	struct arraylist *al = calloc(1, sizeof *al);
	al->points = calloc(ARRAYLIST_SIZE, sizeof *al->points);
	return al;
}

struct arraylist_int *
NewArrayList_Int(void)
{
	struct arraylist_int *al = calloc(1, sizeof *al);
	al->points = calloc(ARRAYLIST_SIZE, sizeof *al->points);
	return al;
}

struct dataset {
	char *name;
	double *points;
	double sy, syy;
	unsigned n;
	struct arraylist *head, *tail;
};

struct dataset_int {
	char *name;
	long int *points;
	long long int sy, syy;
	unsigned n;
	struct arraylist_int *head, *tail;
};

static void
AddPoint(struct dataset *ds, double a)
{
	clock_gettime(CLOCK_MONOTONIC, &start); //------------ time point start ------------//
	if (ds->tail->n >= ARRAYLIST_SIZE) {
		ds->tail = ds->tail->next = NewArrayList();
	}
	ds->tail->points[ds->tail->n++] = a;
	ds->sy += a;
	ds->syy += a * a;
	ds->n += 1;
	clock_gettime(CLOCK_MONOTONIC, &stop); //------------ time point stop ------------//
	ts[0] = elapsed_us(&start, &stop);
}

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
DataSetUnion(struct dataset *destination, struct dataset *source)
{
	destination->sy += source->sy;
	destination->syy += source->syy;
	destination->n += source->n;
}

static void
DataSetUnion_Int(struct dataset_int *destination, struct dataset_int *source)
{
	destination->sy += source->sy;
	destination->syy += source->syy;
	destination->n += source->n;
}

static struct dataset *
NewDataSet(void)
{
	struct dataset *ds;

	ds = calloc(1, sizeof *ds);
	return(ds);
}

static struct dataset_int *
NewDataSet_Int(void)
{
	struct dataset_int *ds;

	ds = calloc(1, sizeof *ds);
	return(ds);
}

static double
Min(struct dataset *ds)
{

	return (ds->points[0]);
}

static long int
Min_Int(struct dataset_int *ds)
{

	return (ds->points[0]);
}


static double
Max(struct dataset *ds)
{

	return (ds->points[ds->n - 1]);
}

static long int
Max_Int(struct dataset_int *ds)
{

	return (ds->points[ds->n - 1]);
}

static double
Avg(struct dataset *ds)
{

	return(ds->sy / ds->n);
}

static double
Avg_Int(struct dataset_int *ds)
{

	return((double)ds->sy / ds->n);
}

static double
Median(struct dataset *ds)
{

	return (ds->points[ds->n / 2]);
}

static long int
Median_Int(struct dataset_int *ds)
{

	return (ds->points[ds->n / 2]);
}

static double
Var(struct dataset *ds)
{

	return (ds->syy - ds->sy * ds->sy / ds->n) / (ds->n - 1.0);
}

static double
Var_Int(struct dataset_int *ds)
{
	return (ds->syy - (double)ds->sy * ds->sy / ds->n) / (ds->n - 1);
}

static double
Stddev(struct dataset *ds)
{

	return sqrt(Var(ds));
}

static double
Stddev_Int(struct dataset_int *ds)
{

	return sqrt(Var_Int(ds));
}

static void
VitalsHead(void)
{

	printf("    N           Min           Max        Median           Avg        Stddev\n");
}

static void
Vitals(struct dataset *ds, long int flag)
{

	printf("%c %3d %13.8g %13.8g %13.8g %13.8g %13.8g", symbol[flag],
	    ds->n, Min(ds), Max(ds), Median(ds), Avg(ds), Stddev(ds));
	printf("\n");
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
Relative(struct dataset *ds, struct dataset *rs, int confidx)
{
	double spool, s, d, e, t;
	int i;

	i = ds->n + rs->n - 2;
	if (i > NSTUDENT)
		t = student[0][confidx];
	else
		t = student[i][confidx];
	spool = (ds->n - 1) * Var(ds) + (rs->n - 1) * Var(rs);
	spool /= ds->n + rs->n - 2;
	spool = sqrt(spool);
	s = spool * sqrt(1.0 / ds->n + 1.0 / rs->n);
	d = Avg(ds) - Avg(rs);
	e = t * s;

	if (fabs(d) > e) {

		printf("Difference at %.1f%% confidence\n", studentpct[confidx]);
		printf("	%g +/- %g\n", d, e);
		printf("	%g%% +/- %g%%\n", d * 100 / Avg(rs), e * 100 / Avg(rs));
		printf("	(Student's t, pooled s = %g)\n", spool);
	} else {
		printf("No difference proven at %.1f%% confidence\n",
		    studentpct[confidx]);
	}
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

struct plot {
	double		min;
	double		max;
	double		span;
	int		width;

	double		x0, dx;
	int		height;
	char		*data;
	char		**bar;
	int		separate_bars;
	int		num_datasets;
};

static struct plot plot;

static void
SetupPlot(int width, int separate, int num_datasets)
{
	struct plot *pl;

	pl = &plot;
	pl->width = width;
	pl->height = 0;
	pl->data = NULL;
	pl->bar = NULL;
	pl->separate_bars = separate;
	pl->num_datasets = num_datasets;
	pl->min = 999e99;
	pl->max = -999e99;
}

static void
AdjPlot(double a)
{
	struct plot *pl;

	pl = &plot;
	if (a < pl->min)
		pl->min = a;
	if (a > pl->max)
		pl->max = a;
	pl->span = pl->max - pl->min;
	pl->dx = pl->span / (pl->width - 1.0);
	pl->x0 = pl->min - .5 * pl->dx;
}

static void
DimPlot(struct dataset *ds)
{
	AdjPlot(Min(ds));
	AdjPlot(Max(ds));
	AdjPlot(Avg(ds) - Stddev(ds));
	AdjPlot(Avg(ds) + Stddev(ds));
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
PlotSet(struct dataset *ds, int val)
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
	if (!isnan(Stddev(ds))) {
		x = ((Avg(ds) - Stddev(ds)) - pl->x0) / pl->dx;
		m = ((Avg(ds) + Stddev(ds)) - pl->x0) / pl->dx;
		pl->bar[bar][m] = '|';
		pl->bar[bar][x] = '|';
		for (i = x + 1; i < m; i++)
			if (pl->bar[bar][i] == 0)
				pl->bar[bar][i] = '_';
	}
	x = (Median(ds) - pl->x0) / pl->dx;
	pl->bar[bar][x] = 'M';
	x = (Avg(ds) - pl->x0) / pl->dx;
	pl->bar[bar][x] = 'A';
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

static void
DumpPlot(void)
{
	struct plot *pl;
	int i, j, k;

	pl = &plot;
	if (pl->span == 0) {
		printf("[no plot, span is zero width]\n");
		return;
	}

	putchar('+');
	for (i = 0; i < pl->width; i++)
		putchar('-');
	putchar('+');
	putchar('\n');
	for (i = 1; i < pl->height; i++) {
		putchar('|');
		for (j = 0; j < pl->width; j++) {
			k = pl->data[(pl->height - i) * pl->width + j];
			if (k >= 0 && k < MAX_DS)
				putchar(symbol[k]);
			else
				printf("[%02x]", k);
		}
		putchar('|');
		putchar('\n');
	}
	for (i = 0; i < pl->num_datasets; i++) {
		if (pl->bar[i] == NULL)
			continue;
		putchar('|');
		for (j = 0; j < pl->width; j++) {
			k = pl->bar[i][j];
			if (k == 0)
				k = ' ';
			putchar(k);
		}
		putchar('|');
		putchar('\n');
	}
	putchar('+');
	for (i = 0; i < pl->width; i++)
		putchar('-');
	putchar('+');
	putchar('\n');
}

struct readset_context {
	struct dataset **multiset;
	int index;
	int fd; /* file descriptor */
	const char *n; /* filename */
	int column;
	const char *delim;
};

struct readsetworker_context {
	struct readset_context *file;
	size_t start, end; /* file */
	size_t points_start, points_end; /* s->points */
	struct dataset *s; /* parent */
	struct dataset *m; /* thread */
};

static void *
ReadSetWorker(void *readsetworker_context)
{
	struct readsetworker_context *context = readsetworker_context;
	struct dataset *ds = context->m = NewDataSet();
	ds->tail = ds->head = NewArrayList();
	char buf[BUFSIZ], str[BUFSIZ + 25], *p, *t;
	double d, *point;
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

				d = strtod_fast(t, &p);
				if (strcspn(p, context->file->delim))
					err(2, "Invalid data on line %d in %s\n", line,
						context->file->n);
				if (*str != '\0')
					AddPoint(ds, d);
			}
		}

		if (buf[bytes_read - 1] != '\0') {
			strcpy(str, str_start);
			offset = strlen(str);
		}
	}

	point = ds->points = malloc(ds->n * sizeof *ds->points);

	for (struct arraylist *al = ds->head; al != NULL; al = al->next) {
		memcpy(point, al->points, al->n * sizeof *point);
		point += al->n;
	}

	an_qsort_C(ds->points, ds->n);

	return NULL;
}

static void
Merge(double *points, size_t start, size_t mid, size_t end)
{
	size_t n, i;
	double *x, *sorted;

	double *first_half = points + start;
	double *second_half = points + mid;
	double *origin = first_half;
	double *midpoint = second_half;
	double *endpoint = points + end;

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
ReadSet(void *readset_context)
{
	clock_gettime(CLOCK_MONOTONIC, &start); //------------ time point start ------------//
	struct readset_context *context = readset_context;
	int f;
	size_t i, j, k, half_step, step;
	struct dataset *s;
	s = NewDataSet();
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

	struct readsetworker_context *workers[READSET_THREAD_COUNT];
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

		struct readsetworker_context *worker_context = workers[i] = malloc(
			sizeof *worker_context
		);
		worker_context->file = context;
		worker_context->start = ctx_start;
		worker_context->end = ctx_end;
		worker_context->s = s;

		if (pthread_create(t++, NULL, ReadSetWorker, worker_context) != 0) {
			err(1, "Failed to create a ReadSetWorker thread");
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
			err(1, "Failed to join a ReadSetWorker thread");
		}

		workers[i]->points_start = points_start;
		points_end = points_start + workers[i]->m->n;
		workers[i]->points_end = points_end;
		points_start = points_end;

		DataSetUnion(s, workers[i]->m);
	}

	close(f);

	if (s->n < 3) {
		fprintf(stderr,
		    "Dataset %s must contain at least 3 data points\n", context->n);
		exit (2);
	}

	s->points = malloc(s->n * sizeof *s->points);
	double *point = s->points;

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
			Merge(
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

static void
usage(char const *whine)
{
	int i;

	fprintf(stderr, "%s\n", whine);
	fprintf(stderr,
	    "Usage: ministat [-C column] [-c confidence] [-d delimiter(s)] [-ns] [-w width] [file [file ...]]\n");
	fprintf(stderr, "\tconfidence = {");
	for (i = 0; i < NCONF; i++) {
		fprintf(stderr, "%s%g%%",
		    i ? ", " : "",
		    studentpct[i]);
	}
	fprintf(stderr, "}\n");
	fprintf(stderr, "\t-C : column number to extract (starts and defaults to 1)\n");
	fprintf(stderr, "\t-d : delimiter(s) string, default to \" \\t\"\n");
	fprintf(stderr, "\t-n : print summary statistics only, no graph/test\n");
	fprintf(stderr, "\t-q : print summary statistics and test only, no graph\n");
	fprintf(stderr, "\t-s : print avg/median/stddev bars on separate lines\n");
	fprintf(stderr, "\t-w : width of graph/test output (default 74 or terminal width)\n");
	exit (2);
}

int
main(int argc, char **argv)
{
	struct dataset *ds[7];
	struct dataset_int *ds_int[7];
	int nds;
	double a;
	const char *delim = " \t";
	char *p;
	int c, i, ci;
	int column = 1;
	int flag_s = 0;
	int flag_n = 0;
	int flag_q = 0;
	int flag_v = 0;
	int flag_i = 0;
	int termwidth = 74;

	if (isatty(STDOUT_FILENO)) {
		struct winsize wsz;

		if ((p = getenv("COLUMNS")) != NULL && *p != '\0')
			termwidth = atoi(p);
		else if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &wsz) != -1 &&
			 wsz.ws_col > 0)
			termwidth = wsz.ws_col - 2;
	}

	ci = -1;
	while ((c = getopt(argc, argv, "C:c:d:snqiw:v")) != -1)
		switch (c) {
		case 'C':
			column = strtol(optarg, &p, 10);
			if (p != NULL && *p != '\0')
				usage("Invalid column number.");
			if (column <= 0)
				usage("Column number should be positive.");
			break;
		case 'c':
			a = strtod_fast(optarg, &p);
			if (p != NULL && *p != '\0')
				usage("Not a floating point number");
			for (i = 0; i < NCONF; i++)
				if (a == studentpct[i])
					ci = i;
			if (ci == -1)
				usage("No support for confidence level");
			break;
		case 'd':
			if (*optarg == '\0')
				usage("Can't use empty delimiter string");
			delim = optarg;
			break;
		case 'n':
			flag_n = 1;
			break;
		case 'q':
			flag_q = 1;
			break;
		case 's':
			flag_s = 1;
			break;
		case 'i':
			flag_i = 1;
			break;
		case 'w':
			termwidth = strtol(optarg, &p, 10);
			if (p != NULL && *p != '\0')
				usage("Invalid width, not a number.");
			if (termwidth < 0)
				usage("Unable to move beyond left margin.");
			break;
		case 'v':
            flag_v = 1;
            break;
		default:
			usage("Unknown option");
			break;
		}
	if (ci == -1)
		ci = 2;
	argc -= optind;
	argv += optind;

	if (argc == 0) {
		nds = 1;

		if (flag_i) {
			struct readset_context_int context_int;
			context_int.multiset = ds_int;
			context_int.index = 0;
			context_int.n = "-";
			context_int.column = column;
			context_int.delim = delim;

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
	} else {
		if (argc > (MAX_DS - 1))
			usage("Too many datasets.");

		nds = argc;

		pthread_t threads[nds];
		pthread_t *t = threads;

		for (i = 0; i < nds; i++) {
			if (flag_i) {
				struct readset_context_int *context_int = malloc(
					sizeof *context_int
				);
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
			}
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
	}

	for (i = 0; i < nds; i++) {
		if (flag_i) {
			printf("%c %s\n", symbol[i+1], ds_int[i]->name);
		}
		else {
			printf("%c %s\n", symbol[i+1], ds[i]->name);
		}
	}

	if (!flag_n && !flag_q) {
		SetupPlot(termwidth, flag_s, nds);
		for (i = 0; i < nds; i++) {
			if (flag_i) {
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

			if (!flag_n)
				Relative_Int(ds_int[i], ds_int[0], ci);
		}
		else {
			Vitals(ds[i], i + 1);

			if (!flag_n)
				Relative(ds[i], ds[0], ci);
		}

	}
	if(flag_v) {
		TimePrint();
	}
	exit(0);
}
