/*
 * ClamWin Free Antivirus — Preferences Validation Helpers
 */

#pragma once

struct CWLimitsValidationResult
{
    bool ok;
    int field;
    const char* message;
};

enum CWLimitsValidationField
{
    CW_LIMITS_FIELD_NONE = 0,
    CW_LIMITS_FIELD_MAX_FILE_SIZE,
    CW_LIMITS_FIELD_MAX_SCAN_SIZE,
    CW_LIMITS_FIELD_MAX_FILES,
    CW_LIMITS_FIELD_MAX_DEPTH
};

CWLimitsValidationResult CW_ValidateLimitsValues(int maxFileSizeMb,
                                                 bool scanArchives,
                                                 int maxScanSizeMb,
                                                 int maxFiles,
                                                 int maxDepth);
