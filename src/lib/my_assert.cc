#include "lib/my_assert.hh"
#include "lib/strings.hh"

void 
assert_msg(const char *expr, const char *filename, int line, const char *function) {
    erroutf("%s:%d: %s: Assertion '(%s)' failed.\n", filename, line, function, expr);    
}