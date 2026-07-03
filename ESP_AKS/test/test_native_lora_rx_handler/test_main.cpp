#include <unity.h>

extern void test_heartbeat_byte_is_classified_as_heartbeat(void);
extern void test_former_command_bytes_are_classified_as_unknown(void);
extern void test_random_bytes_are_classified_as_unknown(void);
extern void test_unknown_byte_always_increments_count(void);
extern void test_unknown_byte_warns_when_interval_elapsed_since_zero(void);
extern void test_unknown_byte_within_interval_does_not_warn_again(void);
extern void test_unknown_byte_warns_again_after_interval(void);

void setUp(void) {}
void tearDown(void) {}

int main(int /*argc*/, char ** /*argv*/) {
    UNITY_BEGIN();

    RUN_TEST(test_heartbeat_byte_is_classified_as_heartbeat);
    RUN_TEST(test_former_command_bytes_are_classified_as_unknown);
    RUN_TEST(test_random_bytes_are_classified_as_unknown);
    RUN_TEST(test_unknown_byte_always_increments_count);
    RUN_TEST(test_unknown_byte_warns_when_interval_elapsed_since_zero);
    RUN_TEST(test_unknown_byte_within_interval_does_not_warn_again);
    RUN_TEST(test_unknown_byte_warns_again_after_interval);

    return UNITY_END();
}
