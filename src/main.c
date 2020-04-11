#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

// musl
float sinf_musl(float);
float cosf_musl(float);
float tanf_musl(float);

// newlib
float sinf_newlib(float);
float cosf_newlib(float);
float tanf_newlib(float);

#define THREADS    8

#define MAKE_PATTERN(exp, test, accept) \
	{ #exp "," #test, exp, test, accept, }

typedef double (*func_expect)(double);
typedef float (*func_test)(float);

union u32f {
	uint32_t x;
	float f;
};

struct test_pattern {
	const char *name;
	func_expect f_expect;
	func_test f_test;
	int accept_diff;
};

struct workitem {
	struct test_pattern *pat;
	uint32_t st;
	uint32_t ed;
};

struct test_pattern tests[] = {
	MAKE_PATTERN(sin, sinf, 1),
	MAKE_PATTERN(sin, sinf_musl, 1),
	MAKE_PATTERN(sin, sinf_newlib, 1),
	MAKE_PATTERN(cos, cosf, 1),
	MAKE_PATTERN(cos, cosf_musl, 1),
	MAKE_PATTERN(cos, cosf_newlib, 1),
	MAKE_PATTERN(tan, tanf, 3),
	MAKE_PATTERN(tan, tanf_musl, 3),
	MAKE_PATTERN(tan, tanf_newlib, 3),
	{0},
};

const int g_debug = 0;

int32_t abs(int32_t a)
{
	if (a < 0)
		return -a;
	return a;
}

int test_range(uint32_t st, uint32_t ed, struct test_pattern *pat)
{
	union u32f x, expect, result;
	int ret = 0;

	for (uint32_t i = st; i < ed; i++) {
		//to float
		x.x = i;

		expect.f = pat->f_expect(x.f);
		result.f = pat->f_test(x.f);
		if (expect.x == result.x)
			continue;

		if (abs(expect.x - result.x) > pat->accept_diff) {
			printf("%10s: NG  : x:%08x f:%f d:%d, exp:%08x %f, res:%08x %f\n",
				pat->name, x.x, x.f, pat->accept_diff,
				expect.x, expect.f, result.x, result.f);
			ret = 1;
		} else if (g_debug) {
			printf("%10s: PASS: x:%08x f:%f d:%d, exp:%08x %f, res:%08x %f\n",
				pat->name, x.x, x.f, pat->accept_diff,
				expect.x, expect.f, result.x, result.f);
		}
	}

	return ret;
}

void *tmain(void *arg)
{
	struct timeval tv_st, tv_ed, tv_ela;
	struct workitem *item = arg;
	int res;

	gettimeofday(&tv_st, NULL);
	res = test_range(item->st, item->ed, item->pat);
	gettimeofday(&tv_ed, NULL);
	timersub(&tv_ed, &tv_st, &tv_ela);

	printf("%16s: %08x-%08x: diff<=%d, %s, %d.%06d[s]\n",
		item->pat->name, item->st, item->ed,
		item->pat->accept_diff,
		(res == 0) ? "OK!!" : "NG!!",
		(int)tv_ela.tv_sec, (int)tv_ela.tv_usec);

	return NULL;
}

int test_one(int test_no)
{
	struct workitem items[THREADS];
	pthread_t pth[THREADS];
	uint64_t st = 0x00000000, ed = 0xffffffff;
	uint32_t t = 0, inc;
	int res;

	inc = (uint32_t)((ed - st + 1) / THREADS);
	if (inc == 0) {
		fprintf(stderr, "illegal range: %08x\n", inc);
		return 0;
	}

	for (int i = 0; i < THREADS; i++, t += inc) {
		items[i].pat = &tests[test_no];
		items[i].st = t;
		items[i].ed = t + inc - 1;
		res = pthread_create(&pth[i], NULL, tmain, &items[i]);
		if (res != 0) {
			perror("pthread_create");
			return -1;
		}
	}

	for (int i = 0; i < THREADS; i++) {
		pthread_join(pth[i], NULL);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	for (int i = 0; tests[i].f_expect != NULL; i++) {
		test_one(i);
	}

	return 0;
}
