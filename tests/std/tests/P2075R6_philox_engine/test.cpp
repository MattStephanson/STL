// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <random>
#include <sstream>
#include <type_traits>

using namespace std;

void basic_philox_tests() {
    {
        static_assert(is_same_v<philox4x32::result_type, uint_fast32_t>);
        static_assert(philox4x32::word_count == 4);
        static_assert(philox4x32::word_size == 32);
        static_assert(philox4x32::round_count == 10);
        static_assert(philox4x32::min() == 0);
        static_assert(philox4x32::max() == UINT32_MAX);

        constexpr auto expected32 = 1955073260U;
        philox4x32 eng32;
        for (int i = 0; i < 9'999; ++i) {
            (void) eng32();
        }

        assert(eng32() == expected32);

        eng32.set_counter({0, 0, 0, 0});
        assert(eng32 == philox4x32{});
        eng32.discard(9'999);

        assert(eng32() == expected32);
    }


    {
        static_assert(is_same_v<philox4x64::result_type, uint_fast64_t>);
        static_assert(philox4x64::word_count == 4);
        static_assert(philox4x64::word_size == 64);
        static_assert(philox4x64::round_count == 10);
        static_assert(philox4x64::min() == 0);
        static_assert(philox4x64::max() == UINT64_MAX);

        constexpr auto expected64 = 3409172418970261260ULL;
        philox4x64 eng64;
        for (int i = 0; i < 9'999; ++i) {
            (void) eng64();
        }

        assert(eng64() == expected64);

        eng64.set_counter({0, 0, 0, 0});
        assert(eng64 == philox4x64{});
        eng64.discard(9'999);

        assert(eng64() == expected64);
    }
}

void philox_discard_tests() {
    std::array<philox4x32::result_type, 12> reference4x32;
    std::generate(reference4x32.begin(), reference4x32.end(), philox4x32{});

    {
        philox4x32 eng;
        eng.discard(3); // (Z,i): (0,3) -> (1,2)
        eng.discard(3); // (Z,i): (1,2) -> (2,1); generate a carry in
        assert(eng() == reference4x32[6]);
    }

    {
        // The lowest word of the counter overflows every 4*2^32 = values. Split this into two calls to `discard` in
        // order to create a carry out during the second call.
        philox4x32 eng;
        constexpr auto incr = 1ULL << 33;
        eng.discard(incr);
        eng.discard(incr);

        philox4x32 eng_ref;
        eng_ref.discard(incr + incr);
        assert(eng == eng_ref);
        assert(eng() == eng_ref());
    }

    {
        // When `discard` is called, the cached Y values only need to be computed if the Z counter increment
        // is positive AND the i index is not equal to `word_count`-1. Test that it works when exactly one of these
        // conditions is met.

        philox4x32 eng;

        // (Z,i): (0,3) -> (1,3); the cache will be generated on the next call to operator().
        eng.discard(4);
        assert(eng() == reference4x32[4]); // (Z,i): (1,3) -> (2,0)

        // (Z,i): (2,0) -> (2,1); the current cache is still valid.
        eng.discard(1);
        assert(eng() == reference4x32[6]); // (Z,i): (2,1) -> (2,2)

        // (Z,i): (2,2) -> (3,2); the cache is invalid and will NOT be generated when operator() is called.
        eng.discard(4);
        assert(eng() == reference4x32[11]);
    }
}

void philox_streaming() {
    // Counter words are little-endian for streaming.
    constexpr auto expected_str = "97534301 0 200523338 199766893 565984983 194338537 2";

    philox4x32 expected_eng(97534301);
    // Counter words are big-endian for `set_counter`.
    expected_eng.set_counter({194338537, 565984983, 199766893, 200523337});
    expected_eng.discard(3); // carry into the counter, now 200523338

    ostringstream oss;
    oss << expected_eng;
    assert(oss.str() == expected_str);

    philox4x32 eng;
    istringstream{expected_str} >> eng;
    assert(eng == expected_eng);
    assert(eng() == expected_eng());
}

int main() {
    basic_philox_tests();
    philox_discard_tests();
    philox_streaming();
}
