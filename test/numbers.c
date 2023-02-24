#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include "priv/numbers.h"

#define RUN_TEST(name) ({\
		test_##name();\
		printf("%s passed\n", #name);\
	})

void test_ceil_log2() {
	for (uint64_t i = 1; i <= 64; i++) {
		const uint64_t actual = nt_ceil_log2(i);
		const uint64_t expected = floor(log2(i)) + 1;
		assert(actual == expected);
	}
}

void test_multiply_mod() {
	{
		const uint64_t mod = 2;
		const uint64_t mod_bits = nt_ceil_log2(mod);
		const uint64_t barrett_factor = nt_compute_barrett_factor(
				(uint64_t) 1 << (mod_bits + nt_alpha - 64),
			mod, mod_bits);
		
		assert(nt_multiply_mod(0, 0, mod, barrett_factor) == 0);
		assert(nt_multiply_mod(0, 0, mod, barrett_factor) == 0);
		assert(nt_multiply_mod(1, 0, mod, barrett_factor) == 0);
		assert(nt_multiply_mod(1, 1, mod, barrett_factor) == 1);
	}

	{
		const uint64_t mod = 10;
		const uint64_t mod_bits = nt_ceil_log2(mod);
		const uint64_t barrett_factor = nt_compute_barrett_factor(
				(uint64_t) 1 << (mod_bits + nt_alpha - 64),
			mod, mod_bits);
		
		assert(nt_multiply_mod(0, 0, mod, barrett_factor) == 0);
		assert(nt_multiply_mod(7, 7, mod, barrett_factor) == 9);
		assert(nt_multiply_mod(6, 7, mod, barrett_factor) == 2);
		assert(nt_multiply_mod(7, 6, mod, barrett_factor) == 2);
	}

	{
		const uint64_t mod = 2305843009211596801;
		const uint64_t mod_bits = nt_ceil_log2(mod);
		const uint64_t barrett_factor = nt_compute_barrett_factor(
				(uint64_t) 1 << (mod_bits + nt_alpha - 64),
			mod, mod_bits);

		assert(nt_multiply_mod(0, 0, mod, barrett_factor) == 0);
		assert(nt_multiply_mod(1152921504605798400, 1152921504605798401,
					mod, barrett_factor) == 576460752302899200);
	}
}

void test_power_mod() {
	{
		const uint64_t modulus = 5;
		assert(nt_power_mod(1, 0, modulus) == 1);
		assert(nt_power_mod(1, 0xFFFFFFFFFFFFFFFF, modulus) == 1);
		assert(nt_power_mod(2, 0xFFFFFFFFFFFFFFFF, modulus) == 3);
	}

	{
		const uint64_t modulus = 0x1000000000000000;
		assert(nt_power_mod(2, 60, modulus) == 0);
		assert(nt_power_mod(2, 59, modulus) == 0x800000000000000);
	}

	{
		const uint64_t modulus = 131313131313;
		assert(nt_power_mod(2424242424, 16, modulus) == 39418477653);
	}
}

int main() {
	RUN_TEST(ceil_log2);
	RUN_TEST(multiply_mod);
	RUN_TEST(power_mod);
}
