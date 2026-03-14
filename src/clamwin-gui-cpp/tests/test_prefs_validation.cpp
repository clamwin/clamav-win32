#include "doctest.h"
#include "cw_prefs_validation.h"

TEST_SUITE("prefs_validation")
{
    TEST_CASE("limits accepts valid bounds")
    {
        CWLimitsValidationResult r = CW_ValidateLimitsValues(100, true, 150, 500, 50);
        CHECK(r.ok);
        CHECK(r.field == CW_LIMITS_FIELD_NONE);
    }

    TEST_CASE("limits rejects max file size out of range")
    {
        CWLimitsValidationResult low = CW_ValidateLimitsValues(0, false, 150, 500, 50);
        CHECK_FALSE(low.ok);
        CHECK(low.field == CW_LIMITS_FIELD_MAX_FILE_SIZE);

        CWLimitsValidationResult high = CW_ValidateLimitsValues(4097, false, 150, 500, 50);
        CHECK_FALSE(high.ok);
        CHECK(high.field == CW_LIMITS_FIELD_MAX_FILE_SIZE);
    }

    TEST_CASE("limits ignores archive-only bounds when archive scanning disabled")
    {
        CWLimitsValidationResult r = CW_ValidateLimitsValues(100, false, 0, 0, 0);
        CHECK(r.ok);
    }

    TEST_CASE("limits enforces archive size range when enabled")
    {
        CWLimitsValidationResult low = CW_ValidateLimitsValues(100, true, 0, 500, 50);
        CHECK_FALSE(low.ok);
        CHECK(low.field == CW_LIMITS_FIELD_MAX_SCAN_SIZE);

        CWLimitsValidationResult high = CW_ValidateLimitsValues(100, true, 4097, 500, 50);
        CHECK_FALSE(high.ok);
        CHECK(high.field == CW_LIMITS_FIELD_MAX_SCAN_SIZE);
    }

    TEST_CASE("limits enforces max files range when enabled")
    {
        CWLimitsValidationResult low = CW_ValidateLimitsValues(100, true, 150, 0, 50);
        CHECK_FALSE(low.ok);
        CHECK(low.field == CW_LIMITS_FIELD_MAX_FILES);

        CWLimitsValidationResult high = CW_ValidateLimitsValues(100, true, 150, 1073741825, 50);
        CHECK_FALSE(high.ok);
        CHECK(high.field == CW_LIMITS_FIELD_MAX_FILES);
    }

    TEST_CASE("limits enforces max depth range when enabled")
    {
        CWLimitsValidationResult low = CW_ValidateLimitsValues(100, true, 150, 500, 0);
        CHECK_FALSE(low.ok);
        CHECK(low.field == CW_LIMITS_FIELD_MAX_DEPTH);

        CWLimitsValidationResult high = CW_ValidateLimitsValues(100, true, 150, 500, 1000);
        CHECK_FALSE(high.ok);
        CHECK(high.field == CW_LIMITS_FIELD_MAX_DEPTH);
    }
}
