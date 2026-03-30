#include <stdio.h>
#include <string.h>

#include "xdebug_hash.h"

static int failures = 0;
static int dtor_calls = 0;

static void count_dtor(void *ptr)
{
	(void) ptr;
	dtor_calls++;
}

static void assert_true(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "%s\n", message);
		failures = 1;
	}
}

int main(void)
{
	xdebug_hash *h;
	void        *out = NULL;

	h = xdebug_hash_alloc(2, count_dtor);
	assert_true(h != NULL, "hash allocation failed");

	assert_true(xdebug_hash_add(h, "alpha", 5, "a"), "failed to add alpha");
	assert_true(xdebug_hash_add(h, "beta", 4, "b"), "failed to add beta");
	assert_true(xdebug_hash_index_add(h, 42, "n"), "failed to add numeric key");
	assert_true(h->size == 3, "unexpected size before resize");

	assert_true(xdebug_hash_resize(h, 8), "resize to 8 slots failed");
	assert_true(h->slots == 8, "slot count was not updated");
	assert_true(h->size == 3, "size changed after resize");
	assert_true(dtor_calls == 0, "resize should not trigger destructors");

	assert_true(
		xdebug_hash_find(h, "alpha", 5, &out) && strcmp((char *) out, "a") == 0,
		"alpha lookup failed after resize"
	);
	assert_true(
		xdebug_hash_find(h, "beta", 4, &out) && strcmp((char *) out, "b") == 0,
		"beta lookup failed after resize"
	);
	assert_true(
		xdebug_hash_index_find(h, 42, &out) && strcmp((char *) out, "n") == 0,
		"numeric lookup failed after resize"
	);

	assert_true(xdebug_hash_resize(h, 8), "same-size resize should succeed");
	assert_true(!xdebug_hash_resize(h, 0), "resize to 0 slots should fail");
	assert_true(h->slots == 8, "invalid resize changed slot count");

	assert_true(xdebug_hash_add(h, "alpha", 5, "a2"), "failed to update alpha");
	assert_true(h->size == 3, "updating an entry changed the size");
	assert_true(dtor_calls == 1, "updating should destroy exactly one old payload");
	assert_true(
		xdebug_hash_find(h, "alpha", 5, &out) && strcmp((char *) out, "a2") == 0,
		"updated alpha lookup failed"
	);

	xdebug_hash_destroy(h);
	assert_true(dtor_calls == 4, "destroy should process the remaining three payloads");

	if (failures) {
		return 1;
	}

	printf("ok\n");
	return 0;
}
