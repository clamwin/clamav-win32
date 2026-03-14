/*
 * ClamWin Free Antivirus — WinMain entry point (C++ class-based)
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#include "cw_application.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)nCmdShow;

    CWApplication app;
    return app.run(hInstance, lpCmdLine);
}
