/*
 * Copyright (c) 2014 DeNA Co., Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "picotest.h"

struct test_t {
    int num_tests;
    int failed;
};
struct test_t main_tests, *cur_tests = &main_tests;
static int test_level = 0;

static void indent(void)
{
    int i;
    for (i = 0; i != test_level; ++i)
        printf("    ");
}

__attribute__((format (printf, 1, 2)))
void note(const char *fmt, ...)
{
    va_list arg;

    indent();
    printf("# ");

    va_start(arg, fmt);
    vprintf(fmt, arg);
    va_end(arg);

    printf("\n");
}

__attribute__((format (printf, 2, 3)))
void _ok(int cond, const char *fmt, ...)
{
    va_list arg;

    if (! cond)
        cur_tests->failed = 1;
    indent();

    printf("%s %d - ", cond ? "ok" : "not ok", ++cur_tests->num_tests);
    va_start(arg, fmt);
    vprintf(fmt, arg);
    va_end(arg);

    printf("\n");
}

int done_testing(void)
{
    indent();
    printf("1..%d\n", cur_tests->num_tests);
    return cur_tests->failed;
}

void subtest(const char *name, void (*cb)(void))
{
    struct test_t test = {}, *parent_tests;

    parent_tests = cur_tests;
    cur_tests = &test;
    ++test_level;

    note("Subtest: %s", name);

    cb();

    done_testing();

    --test_level;
    cur_tests = parent_tests;
    if (test.failed)
        cur_tests->failed = 1;
    _ok(! test.failed, "%s", name);
}
