#ifndef AGW_TEST_H
#define AGW_TEST_H

#include <cstdio>
#include <cstdlib>
#include <string>

namespace agw_test {
inline int &failCount()
{
    static int n = 0;
    return n;
}

inline void report(bool ok, const char *expr, const char *file, int line)
{
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n  at %s:%d\n", expr, file, line);
        ++failCount();
    }
}

inline void reportEq(const std::string &a, const std::string &b, const char *expr, const char *file, int line)
{
    if (a != b) {
        std::fprintf(stderr, "FAIL: %s\n  at %s:%d\n  lhs=[%s]\n  rhs=[%s]\n",
                     expr, file, line, a.c_str(), b.c_str());
        ++failCount();
    }
}
}

#define CHECK(expr) ::agw_test::report((expr), #expr, __FILE__, __LINE__)
#define CHECK_EQ(a, b) ::agw_test::reportEq((a), (b), #a " == " #b, __FILE__, __LINE__)

#define AGW_TEST_MAIN_RETURN() \
    (::agw_test::failCount() == 0 ? (std::printf("OK\n"), 0) : (std::fprintf(stderr, "%d check(s) failed\n", ::agw_test::failCount()), 1))

#endif
