#include "texture.h"

#include "unity.h"

static void
test_str_from_texture_kind(
	void
) {
	StringView str = str_from_texture_kind(TextureKindDiffuse);
	TEST_ASSERT(sv_eq(str, sv("diffuse")));
}

static void
test_texture_kind_from_str(
	void
) {
	TEST_ASSERT(TextureKindDiffuse == texture_kind_from_str(&sv("diffuse")));
}

static void
test_sv_eq(
	void
) {
	TEST_ASSERT( sv_eq(sv("lady"), sv("lady")));
	TEST_ASSERT(!sv_eq(sv("lady"), sv("gaga")));
	TEST_ASSERT(!sv_eq(sv("lady"), sv("ladi")));
	TEST_ASSERT(!sv_eq(sv("lady"), sv("lad" )));
}

static void
test_sv(
	void
) {
	TEST_ASSERT_EQUAL_size_t(sv(""     ).len, 0);
	TEST_ASSERT_EQUAL_size_t(sv("alpha").len, 5);

	char chars[4] = {0};
	TEST_ASSERT_EQUAL_PTR(sv(chars).ptr, chars);
	TEST_ASSERT_EQUAL_size_t(sv(chars).len, 3);
}

static void
test_sv_print(
	void
) {
	StringView print_me = sv("print me!");
	char buf[32];
	snprintf(buf, sizeof(buf), "printed: " PRISV "\n", SV(print_me));
	TEST_ASSERT_EQUAL_STRING("printed: print me!\n", buf);
}

static void
test_bv(
	void
) {
	TEST_ASSERT_EQUAL_size_t(bv(""     ).size, 1);
	TEST_ASSERT_EQUAL_size_t(bv("alpha").size, 6);

	byte bytes[4] = {0};
	TEST_ASSERT_EQUAL_PTR(bv(bytes).data, bytes);
	TEST_ASSERT_EQUAL_size_t(bv(bytes).size, 4);
}

int
main(
	void
) {
	UNITY_BEGIN();
	RUN_TEST(test_str_from_texture_kind);
	RUN_TEST(test_texture_kind_from_str);
	RUN_TEST(test_sv_eq);
	RUN_TEST(test_sv);
	RUN_TEST(test_sv_print);
	RUN_TEST(test_bv);
	return UNITY_END();
}

void setUp(void) { NOOP; }
void tearDown(void) { NOOP; }
