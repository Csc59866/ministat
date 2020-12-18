# ministat
A small tool to do the statistics legwork on benchmarks etc. The goal is to optimize ministat to take advantage modern CPU features and compiler optimizations.

## Milestones 

### 1. Implement micro-optimizations
- Implement a new data structure for inserting new data points:
<p>Initially, change the algorithm to just use realloc without using calloc or memcpy.</p>

Before:
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

After:
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


### 2. Validate performance improvements

### 3. Switch to a multi-threaded architecture

### 4. Implement integer mode

### 5. Validate performance improvements 


## Contributors

| Name          | Github        | 
| ------------- | ------------- | 
| Kangming Deng | <a href="https://github.com/Kamide">@Kamide/a>  | 
| Leah Meza | <a href="https://github.com/leahmezacs">@leahmezacs</a>  |
| Jonathan So | <a href="https://github.com/Jonathan668">@Jonathan668</a>  |	
| Jiseon Yu | <a href="https://github.com/JiseonYu">@JiseonYu</a> | 
