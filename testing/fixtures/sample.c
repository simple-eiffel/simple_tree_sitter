/* A small C file for the simple_tree_sitter tests: café ∀ 😀. */
#include <stdio.h>

#define MAX_ITEMS 16

typedef struct {
	int count;
	const char *name;
} item_t;

static int total(const int *values, int n)
{
	int i, sum = 0;
	for (i = 0; i < n; i++) {
		if (values[i] > 0) {
			sum += values[i];
		}
	}
	return sum;
}

int main(void)
{
	int values[MAX_ITEMS] = { 1, 2, 3 };
	item_t item = { 3, "naïve 😀" };
	printf("%s %d\n", item.name, total(values, item.count));
	return 0;
}
