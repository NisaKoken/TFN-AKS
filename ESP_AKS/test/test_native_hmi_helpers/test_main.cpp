#include <unity.h>

// Faz 5 — HMI helpers (saf metin + change-cache UART yazıcılar)

// State text
extern void test_state_text_init(void);
extern void test_state_text_idle(void);
extern void test_state_text_ready(void);
extern void test_state_text_drive(void);
extern void test_state_text_emergency_stop(void);
extern void test_state_text_fault(void);
extern void test_state_text_unknown_falls_back(void);

// Error format
extern void test_error_format_zero(void);
extern void test_error_format_full_byte(void);
extern void test_error_format_specific_value(void);
extern void test_error_format_uppercase_hex(void);
extern void test_error_format_zero_buffer_size_no_crash(void);
extern void test_error_format_truncates_to_small_buffer(void);

// Validity / contactor
extern void test_validity_valid_no_timeout(void);
extern void test_validity_invalid_no_timeout(void);
extern void test_validity_timeout_overrides_invalid(void);
extern void test_validity_timeout_overrides_valid(void);
extern void test_contactor_closed(void);
extern void test_contactor_open(void);

// Change cache
extern void test_numeric_same_value_no_force_skips_write(void);
extern void test_numeric_changed_value_writes_command(void);
extern void test_numeric_force_writes_even_when_unchanged(void);
extern void test_numeric_negative_value_formatted(void);
extern void test_text_same_value_no_force_skips_write(void);
extern void test_text_changed_value_writes_command(void);
extern void test_text_force_writes_even_when_unchanged(void);
extern void test_text_terminated_with_end_bytes(void);
extern void test_sendEndBytes_writes_three_ff(void);

// "Veri yok" gösterimi (UNVERIFIED SOC/sıcaklık sentinelleri)
extern void test_battery_unverified_source_returns_no_data(void);
extern void test_temp_unverified_source_returns_no_data(void);
extern void test_production_source_verified_flags_are_false_pending_hw_verify(void);
extern void test_battery_invalid_bms_returns_no_data(void);
extern void test_temp_invalid_bms_returns_no_data(void);
extern void test_battery_verified_valid_converts_hundredths_to_percent(void);
extern void test_battery_verified_valid_clamps_above_100(void);
extern void test_temp_verified_valid_passes_through(void);

// §8.2.a.iv akım (packi) — gating + Prompt 1 sözleşmesi B (txt "--"/"±d.d")
extern void test_current_unverified_source_returns_no_data(void);
extern void test_current_invalid_bms_returns_no_data(void);
extern void test_current_verified_valid_passes_through(void);
extern void test_battery_text_sentinel_is_dashes(void);
extern void test_battery_text_value(void);
extern void test_temp_text_sentinel_is_dashes(void);
extern void test_temp_text_value(void);
extern void test_current_text_sentinel_is_dashes(void);
extern void test_current_text_positive_one_decimal(void);
extern void test_current_text_negative_one_decimal(void);
extern void test_current_text_sub_amp_and_zero(void);

// RX sınıflandırıcısı — touch vs Nextion sistem yanıtı karışması (§9.4.a.vii)
extern void test_rx_touch_start_cmd1(void);
extern void test_rx_touch_all_four_commands(void);
extern void test_rx_touch_checksum_mismatch_no_command(void);
extern void test_rx_0x88_is_reset_not_touch(void);
extern void test_rx_startup_sequence_is_reset(void);
extern void test_rx_startup_with_extra_leading_zeros(void);
extern void test_rx_bare_ff_run_no_reset(void);
extern void test_rx_two_zeros_then_ff_no_reset(void);
extern void test_rx_broken_startup_pattern_no_reset(void);
extern void test_rx_reset_then_touch_both_seen(void);
extern void test_rx_startup_then_touch_both_seen(void);
extern void test_rx_stray_ff_before_touch_does_not_corrupt(void);
extern void test_rx_touch_bytes_never_trigger_reset(void);

void setUp(void) {}
void tearDown(void) {}

int main(int /*argc*/, char ** /*argv*/) {
    UNITY_BEGIN();

    RUN_TEST(test_state_text_init);
    RUN_TEST(test_state_text_idle);
    RUN_TEST(test_state_text_ready);
    RUN_TEST(test_state_text_drive);
    RUN_TEST(test_state_text_emergency_stop);
    RUN_TEST(test_state_text_fault);
    RUN_TEST(test_state_text_unknown_falls_back);

    RUN_TEST(test_error_format_zero);
    RUN_TEST(test_error_format_full_byte);
    RUN_TEST(test_error_format_specific_value);
    RUN_TEST(test_error_format_uppercase_hex);
    RUN_TEST(test_error_format_zero_buffer_size_no_crash);
    RUN_TEST(test_error_format_truncates_to_small_buffer);

    RUN_TEST(test_validity_valid_no_timeout);
    RUN_TEST(test_validity_invalid_no_timeout);
    RUN_TEST(test_validity_timeout_overrides_invalid);
    RUN_TEST(test_validity_timeout_overrides_valid);
    RUN_TEST(test_contactor_closed);
    RUN_TEST(test_contactor_open);

    RUN_TEST(test_numeric_same_value_no_force_skips_write);
    RUN_TEST(test_numeric_changed_value_writes_command);
    RUN_TEST(test_numeric_force_writes_even_when_unchanged);
    RUN_TEST(test_numeric_negative_value_formatted);
    RUN_TEST(test_text_same_value_no_force_skips_write);
    RUN_TEST(test_text_changed_value_writes_command);
    RUN_TEST(test_text_force_writes_even_when_unchanged);
    RUN_TEST(test_text_terminated_with_end_bytes);
    RUN_TEST(test_sendEndBytes_writes_three_ff);

    RUN_TEST(test_battery_unverified_source_returns_no_data);
    RUN_TEST(test_temp_unverified_source_returns_no_data);
    RUN_TEST(test_production_source_verified_flags_are_false_pending_hw_verify);
    RUN_TEST(test_battery_invalid_bms_returns_no_data);
    RUN_TEST(test_temp_invalid_bms_returns_no_data);
    RUN_TEST(test_battery_verified_valid_converts_hundredths_to_percent);
    RUN_TEST(test_battery_verified_valid_clamps_above_100);
    RUN_TEST(test_temp_verified_valid_passes_through);

    // §8.2.a.iv akım gating + sentinel txt formatlama
    RUN_TEST(test_current_unverified_source_returns_no_data);
    RUN_TEST(test_current_invalid_bms_returns_no_data);
    RUN_TEST(test_current_verified_valid_passes_through);
    RUN_TEST(test_battery_text_sentinel_is_dashes);
    RUN_TEST(test_battery_text_value);
    RUN_TEST(test_temp_text_sentinel_is_dashes);
    RUN_TEST(test_temp_text_value);
    RUN_TEST(test_current_text_sentinel_is_dashes);
    RUN_TEST(test_current_text_positive_one_decimal);
    RUN_TEST(test_current_text_negative_one_decimal);
    RUN_TEST(test_current_text_sub_amp_and_zero);

    // RX sınıflandırıcısı — touch vs Nextion sistem yanıtı karışması
    RUN_TEST(test_rx_touch_start_cmd1);
    RUN_TEST(test_rx_touch_all_four_commands);
    RUN_TEST(test_rx_touch_checksum_mismatch_no_command);
    RUN_TEST(test_rx_0x88_is_reset_not_touch);
    RUN_TEST(test_rx_startup_sequence_is_reset);
    RUN_TEST(test_rx_startup_with_extra_leading_zeros);
    RUN_TEST(test_rx_bare_ff_run_no_reset);
    RUN_TEST(test_rx_two_zeros_then_ff_no_reset);
    RUN_TEST(test_rx_broken_startup_pattern_no_reset);
    RUN_TEST(test_rx_reset_then_touch_both_seen);
    RUN_TEST(test_rx_startup_then_touch_both_seen);
    RUN_TEST(test_rx_stray_ff_before_touch_does_not_corrupt);
    RUN_TEST(test_rx_touch_bytes_never_trigger_reset);

    return UNITY_END();
}
