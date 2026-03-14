/*
 * ClamWin Free Antivirus — CWDialog base class
 *
 * Virtual dispatch for Win32 modal dialogs built from a memory
 * template (no .rc file required for simple dialogs).
 *
 * For complex dialogs, subclasses can override runModal() to use
 * DialogBoxIndirectParam with a richer template.
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#pragma once
#include "cwdefs.h"
#include <windows.h>

class CWDialog
{
public:
    CWDialog();
    virtual ~CWDialog();

    HWND hwnd() const { return m_hwnd; }

    /* Run a modal dialog. Returns the DialogBox result (usually
     * IDOK or IDCANCEL), or -1 on error. */
    INT_PTR runModal(HWND parent, int w = 400, int h = 300);

    /* Control mnemonic visibility for owner-drawn controls in this dialog. */
    static void setDialogMnemonicCues(HWND hwnd, bool show);
    static bool getDialogMnemonicCues(HWND hwnd);

protected:
    HWND m_hwnd;

    /* Close the dialog with a result code. */
    void endDialog(INT_PTR result);

    /* Move focus to the next/previous tab-stop item in the dialog. */
    bool focusNextTabStop(HWND from, bool previous = false);

    /* Move focus to the first tab-stop item in the dialog. */
    bool focusFirstTabStop();

    /* ── Virtual message handlers ─────────────────────────── */

    /* Called from WM_INITDIALOG. Return TRUE to set focus. */
    virtual bool onInit()         { return true; }

    /* Called from WM_COMMAND. Return TRUE if handled. */
    virtual bool onCommand(int id, HWND src) { (void)id; (void)src; return false; }

    /* Called from WM_CLOSE. Default: EndDialog(IDCANCEL). */
    virtual void onClose()        { endDialog(IDCANCEL); }

    /* Override for full control. */
    virtual INT_PTR handleMessage(UINT msg, WPARAM wp, LPARAM lp);

    /* Optional override: determine if a control should use default colors. */
    virtual bool useDefaultControlColors(HWND ctrl) const { (void)ctrl; return false; }

private:
    /* No copy */
    CWDialog(const CWDialog&);
    CWDialog& operator=(const CWDialog&);

    static INT_PTR CALLBACK staticDlgProc(HWND hwnd, UINT msg,
                                           WPARAM wp, LPARAM lp);
};
