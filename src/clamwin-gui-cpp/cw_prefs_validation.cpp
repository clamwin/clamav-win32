/*
 * ClamWin Free Antivirus — Preferences Validation Helpers
 */

#include "cw_prefs_validation.h"

CWLimitsValidationResult CW_ValidateLimitsValues(int maxFileSizeMb,
                                                 bool scanArchives,
                                                 int maxScanSizeMb,
                                                 int maxFiles,
                                                 int maxDepth)
{
    if (maxFileSizeMb < 1 || maxFileSizeMb > 4096)
    {
        CWLimitsValidationResult r = {false, CW_LIMITS_FIELD_MAX_FILE_SIZE,
                                      "Max file size must be between 1 and 4096 MB."};
        return r;
    }

    if (scanArchives)
    {
        if (maxScanSizeMb < 1 || maxScanSizeMb > 4096)
        {
            CWLimitsValidationResult r = {false, CW_LIMITS_FIELD_MAX_SCAN_SIZE,
                                          "Max archive size must be between 1 and 4096 MB."};
            return r;
        }

        if (maxFiles < 1 || maxFiles > 1073741824)
        {
            CWLimitsValidationResult r = {false, CW_LIMITS_FIELD_MAX_FILES,
                                          "Max extracted files must be between 1 and 1073741824."};
            return r;
        }

        if (maxDepth < 1 || maxDepth > 999)
        {
            CWLimitsValidationResult r = {false, CW_LIMITS_FIELD_MAX_DEPTH,
                                          "Max sub-archives must be between 1 and 999."};
            return r;
        }
    }

    CWLimitsValidationResult ok = {true, CW_LIMITS_FIELD_NONE, ""};
    return ok;
}
