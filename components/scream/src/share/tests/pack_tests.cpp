#include "catch2/catch.hpp"

#include "share/scream_config.hpp"
#include "share/scream_types.hpp"
#include "share/scream_pack.hpp"

namespace {

template <int PACKN>
struct TestMask {
  typedef scream::pack::Mask<PACKN> Mask;
  typedef scream::pack::Pack<int, PACKN> Pack;

  static int sum_true (const Mask& m) {
    int sum1 = 0, sum2 = 0, sum3 = 0;
    scream_masked_loop(m) ++sum1;
    scream_masked_loop_no_force_vec(m) ++sum2;
    scream_masked_loop_no_vec(m) ++sum3;
    REQUIRE(sum1 == sum2);
    REQUIRE(sum2 == sum3);
    return sum1;
  }

  static void run () {
    {
      Mask m(false);
      REQUIRE( ! m.any());
    }
    {
      Mask m(true);
      REQUIRE(m.any());
      for (int i = 0; i < Mask::n; ++i) REQUIRE(m[i]);
      REQUIRE(sum_true(m) == Mask::n);
    }
    for (int i = 0; i < Mask::n; ++i) {
      Mask m(false);
      m.set(i, true);
      REQUIRE(sum_true(m) == 1);
    }
    {
      Pack a, b;
      for (int i = 0; i < Mask::n; ++i)
        a[i] = i + (i % 2);
      for (int i = 0; i < Mask::n; ++i)
        b[i] = i;
      const auto m1 = a > b;
      scream_masked_loop_no_vec(m1) REQUIRE(m1[s] == (s % 2 == 1));
      const auto m2 = b < a;
      scream_masked_loop_no_vec(m2) REQUIRE(m2[s] == (s % 2 == 1));
      REQUIRE(sum_true(m1 && m2) == Mask::n / 2);
      REQUIRE(sum_true(m1 || m2) == Mask::n / 2);
      REQUIRE(sum_true(m1 && ! m2) == 0);
      REQUIRE(sum_true(m1 || ! m2) == Mask::n);
    }
  }
};

TEST_CASE("Mask", "scream::pack") {
  TestMask<1>::run();
  TestMask<2>::run();
  TestMask<3>::run();
  TestMask<4>::run();
  TestMask<8>::run();
  TestMask<16>::run();
  TestMask<32>::run();
}

template <typename Scalar, int PACKN>
struct TestPack {
  typedef scream::pack::Mask<PACKN> Mask;
  typedef scream::pack::Pack<Scalar, PACKN> Pack;
  typedef typename Pack::scalar scalar;

  static const double tol;

  static void test_conversion () {
    
  }

  static void test_unary_min_max () {
    Mask m(true);
    Pack p;
    for (int i = 0; i < p.n; ++i) {
      for (int j = 0; j < p.n; ++j)
        p[j] = j;
      p[i] = -1;
      REQUIRE(min(p) == -1);
      REQUIRE(min(m, p.n, p) == -1);
      p[i] = p.n;
      REQUIRE(max(p) == p.n);
      REQUIRE(max(m, -1, p) == p.n);
    }
  }

  static void test_index () {
    
  }

  static void test_range () {
    
  }

  static void setup (Pack& a, Pack& b, scalar& c,
                     const bool limit = false) {
    using scream::util::min;
    for (int i = 0; i < Pack::n; ++i) {
      const auto sign = (2*(i % 2) - 1);
      a[i] =  i + 3.5;
      if (limit) a[i] = min<scalar>(3, a[i]);
      a[i] *= sign;
    }
    for (int i = 0; i < Pack::n; ++i) {
      const auto sign = (2*(i % 2) - 1);
      b[i] = i - 11;
      if (limit) b[i] = min<scalar>(3, b[i]);
      if (b[i] == 0) b[i] = 2;
      b[i] *= sign;
    }
    c = 42;
  }

  static void compare (const Pack& a, const Pack& b) {
    REQUIRE(max(abs(a - b)) <= tol);
  }

  static void compare (const Mask& a, const Mask& b) {
    vector_novec for (int i = 0; i < Mask::n; ++i)
      REQUIRE(a[i] == b[i]);
  }

#define test_pack_gen_assign_op_all(op) do {        \
    Pack a, b;                                      \
    scalar c;                                       \
    setup(a, b, c);                                 \
    const auto a0(a);                               \
    auto ac(a0);                                    \
    a op b;                                         \
    vector_novec for (int i = 0; i < Pack::n; ++i)  \
      ac[i] op b[i];                                \
    compare(ac, a);                                 \
    a = a0;                                         \
    ac = a0;                                        \
    a op c;                                         \
    vector_novec for (int i = 0; i < Pack::n; ++i)  \
      ac[i] op c;                                   \
    compare(ac, a);                                 \
  } while (0)

#define test_pack_gen_bin_op_all(op) do {           \
    Pack a, b, d, dc;                               \
    scalar c;                                       \
    setup(a, b, c);                                 \
    d = a op b;                                     \
    vector_novec for (int i = 0; i < Pack::n; ++i)  \
      dc[i] = a[i] op b[i];                         \
    compare(dc, d);                                 \
    d = a op c;                                     \
    vector_novec for (int i = 0; i < Pack::n; ++i)  \
      dc[i] = a[i] op c;                            \
    compare(dc, d);                                 \
    d = c op b;                                     \
    vector_novec for (int i = 0; i < Pack::n; ++i)  \
      dc[i] = c op b[i];                            \
    compare(dc, d);                                 \
  } while (0)

#define test_pack_gen_bin_fn_all(op, impl) do {     \
    Pack a, b, d, dc;                               \
    scalar c;                                       \
    setup(a, b, c);                                 \
    d = op(a, b);                                   \
    vector_novec for (int i = 0; i < Pack::n; ++i)  \
      dc[i] = impl(a[i], b[i]);                     \
    compare(dc, d);                                 \
    d = op(a, c);                                   \
    vector_novec for (int i = 0; i < Pack::n; ++i)  \
      dc[i] = impl(a[i], c);                        \
    compare(dc, d);                                 \
    d = op(c, b);                                   \
    vector_novec for (int i = 0; i < Pack::n; ++i)  \
      dc[i] = impl(c, b[i]);                        \
    compare(dc, d);                                 \
  } while (0)

#define test_pack_gen_unary_fn(op, impl) do {       \
    Pack a, b, ac;                                  \
    scalar c;                                       \
    setup(a, b, c, true);                           \
    a = op(abs(b));                                 \
    vector_novec for (int i = 0; i < Pack::n; ++i)  \
      ac[i] = impl(std::abs(b[i]));                 \
    compare(ac, a);                                 \
  } while (0)

#define test_pack_gen_unary_stdfn(op) test_pack_gen_unary_fn(op, std::op)

#define test_mask_gen_bin_op_all(op) do {           \
    Pack a, b;                                      \
    scalar c;                                       \
    Mask m, mc;                                     \
    setup(a, b, c);                                 \
    m = a op b;                                     \
    vector_novec for (int i = 0; i < Pack::n; ++i)  \
      mc.set(i, a[i] op b[i]);                      \
    compare(mc, m);                                 \
    m = a op c;                                     \
    vector_novec for (int i = 0; i < Pack::n; ++i)  \
      mc.set(i, a[i] op c);                         \
    compare(mc, m);                                 \
    m = c op b;                                     \
    vector_novec for (int i = 0; i < Pack::n; ++i)  \
      mc.set(i, c op b[i]);                         \
    compare(mc, m);                                 \
  } while (0)

  static void run () {
    test_pack_gen_assign_op_all(=);
    test_pack_gen_assign_op_all(+=);
    test_pack_gen_assign_op_all(-=);
    test_pack_gen_assign_op_all(*=);
    test_pack_gen_assign_op_all(/=);

    test_pack_gen_bin_op_all(+);
    test_pack_gen_bin_op_all(-);
    test_pack_gen_bin_op_all(*);
    test_pack_gen_bin_op_all(/);

    test_pack_gen_bin_fn_all(min, scream::util::min);
    test_pack_gen_bin_fn_all(max, scream::util::max);

    test_pack_gen_unary_stdfn(abs);
    test_pack_gen_unary_stdfn(exp);
    test_pack_gen_unary_stdfn(log);
    test_pack_gen_unary_stdfn(log10);
    test_pack_gen_unary_stdfn(tgamma);

    test_mask_gen_bin_op_all(==);
    test_mask_gen_bin_op_all(>=);
    test_mask_gen_bin_op_all(<=);
    test_mask_gen_bin_op_all(>);
    test_mask_gen_bin_op_all(<);

    test_conversion();
    test_unary_min_max();
    test_index();
    test_range();
  }
};

template <typename Scalar, int PACKN>
const double TestPack<Scalar,PACKN>::tol =
  std::numeric_limits<Scalar>::epsilon();

TEST_CASE("Pack", "scream::pack") {
  TestPack<int,1>::run();
  TestPack<int,2>::run();
  TestPack<int,3>::run();
  TestPack<int,4>::run();
  TestPack<int,8>::run();
  TestPack<int,16>::run();
  TestPack<int,32>::run();
  TestPack<long,1>::run();
  TestPack<long,2>::run();
  TestPack<long,3>::run();
  TestPack<long,4>::run();
  TestPack<long,8>::run();
  TestPack<long,16>::run();
  TestPack<long,32>::run();
  TestPack<float,1>::run();
  TestPack<float,2>::run();
  TestPack<float,3>::run();
  TestPack<float,4>::run();
  TestPack<float,8>::run();
  TestPack<float,16>::run();
  TestPack<float,32>::run();
  TestPack<double,1>::run();
  TestPack<double,2>::run();
  TestPack<double,3>::run();
  TestPack<double,4>::run();
  TestPack<double,8>::run();
  TestPack<double,16>::run();
  TestPack<double,32>::run();
}

} // empty namespace
