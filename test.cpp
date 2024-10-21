#define CRITERION_MAIN
#include <criterion/criterion.h>

Test(SampleTest, BasicAssertions) { cr_assert_eq(1, 1, "1 should be equal to 1"); }

int main(int argc, char *argv[]) { return criterion_test(argc, argv); }


#include <string.h>
#include <criterion/criterion.h>

Test(sample, test) {
    cr_expect(strlen("Test") == 4, "Expected \"Test\" to have a length of 4.");
    cr_expect(strlen("Hello") == 4, "This will always fail, why did I add this?");
    cr_assert(strlen("") == 0);
}

