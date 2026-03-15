//-----------------------------------------------------------------------------
// Name:        cw_shell_extension_impl.cpp
// Product:     ClamWin Antivirus
//
// Author:      alch [alch at users dot sourceforge dot net]
//
// Created:     2004/19/03
// Copyright:   Copyright alch (c) 2004
// Licence:
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

//-----------------------------------------------------------------------------

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <shellapi.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include "cw_shell_extension.h"
#include "cw_shell_extension_command.h"
#define ResultFromShort(i)  ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, (USHORT)(i)))

//
//  FUNCTION: CWShellExtension::Initialize(LPCITEMIDLIST, LPDATAOBJECT, HKEY)
//
//  PURPOSE: Called by the shell when initializing a context menu or property
//           sheet extension.
//
//  PARAMETERS:
//    pIDFolder - Specifies the parent folder
//    pDataObj  - Spefifies the set of items selected in that folder.
//    hRegKey   - Specifies the type of the focused item in the selection.
//
//  RETURN VALUE:
//
//    NOERROR in all cases.
//
//  COMMENTS:   Note that at the time this function is called, we don't know
//              (or care) what type of shell extension is being initialized.
//              It could be a context menu or a property sheet.
//

STDMETHODIMP CWShellExtension::Initialize(LPCITEMIDLIST pIDFolder,
                                   LPDATAOBJECT pDataObj,
                                   HKEY hRegKey)
{
	HRESULT hres = E_FAIL;
	STGMEDIUM medium;
	FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	TCHAR szPath[MAX_PATH];
	INT numFiles;
	size_t len;


	// Initialize can be called more than once
	if (m_pDataObj)
		m_pDataObj->Release();

	// duplicate the object pointer and registry handle
	if (pDataObj)
	{
		m_pDataObj = pDataObj;
		pDataObj->AddRef();
	}
    else
    {
        return E_INVALIDARG;
    }

	// use the given IDataObject to get a list of filenames (CF_HDROP)
	hres = pDataObj->GetData(&fmte, &medium);

	if(FAILED(hres))
		return E_FAIL;

	// find out how many files the user selected
	// not more than 250 though, otherwise explorer crashes
	numFiles = DragQueryFile((HDROP)medium.hGlobal, (UINT)-1, NULL, 0);
    if(numFiles > 250)
    {
        ::ReleaseStgMedium(&medium);
		return E_FAIL;
    }

	// free old path (just in case)
	if(m_szPath)
	{
        delete [] m_szPath;
 	    m_szPath = NULL;
    }
	// allocate memory for our combined path
	// add length of [ --path=""]
	INT cbPath = (MAX_PATH + 10) * numFiles;
	m_szPath = new TCHAR[cbPath];
	m_szPath[0] = _T('\0');
	for(int i = 0; i < numFiles; i++)
	{
        DragQueryFile((HDROP)medium.hGlobal, i, szPath, _countof(szPath));
		// check the long path validity for a unicode build
#if defined UNICODE || defined _UNICODE
		{
			CHAR atemp[MAX_PATH];
			BOOL invalid;
			if(!WideCharToMultiByte(CP_OEMCP, WC_NO_BEST_FIT_CHARS, szPath, -1,
      		  atemp, sizeof(atemp), NULL, &invalid) || invalid)
    		{
    			WCHAR wtemp[MAX_PATH];
    			// unicode name is not directly tranleatable to ascii, use the shortname instead
           		if(GetShortPathNameW(szPath, wtemp, _countof(wtemp)) > 0)
   	  			wcscpy(szPath, wtemp);
    		}
		}
#endif
		// convert \ to / so cygwin doesn't go crazy (particularly over UNC names)
//		_tcsreplace(szPath, _T('\\'), _T('/'));
		len = _tcslen(szPath);
		// remove last slash from the scanning path
        if(szPath[len-1] == _T('\\'))
            szPath[len-1] = _T('\0');
        {
            size_t used = _tcslen(m_szPath);
            size_t cap = (size_t)cbPath;
            if (used < cap)
            {
                _sntprintf(m_szPath + used, cap - used, _T(" --path=\"%s\""), szPath);
                m_szPath[cbPath - 1] = _T('\0');
            }
        }
	}
	::ReleaseStgMedium(&medium);

	return NOERROR;
}


//
//  FUNCTION: CWShellExtension::QueryContextMenu(HMENU, UINT, UINT, UINT, UINT)
//
//  PURPOSE: Called by the shell just before the context menu is displayed.
//           This is where you add your specific menu items.
//
//  PARAMETERS:
//    hMenu      - Handle to the context menu
//    indexMenu  - Index of where to begin inserting menu items
//    idCmdFirst - Lowest value for new menu ID's
//    idCmtLast  - Highest value for new menu ID's
//    uFlags     - Specifies the context of the menu event
//
//  RETURN VALUE:
//
//
//  COMMENTS:
//
// The menu text
STDMETHODIMP CWShellExtension::QueryContextMenu(HMENU hMenu,UINT indexMenu,UINT idCmdFirst,UINT idCmdLast,UINT uFlags)
{

	UINT			idCmd = idCmdFirst;
	HRESULT		hr = E_INVALIDARG;

	// Seperator
	::InsertMenu(hMenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL);
	::InsertMenu(hMenu, indexMenu++, MF_STRING|MF_BYPOSITION, idCmd++, _T("Scan with ClamWin Free Antivirus"));
	// Seperator
	::InsertMenu(hMenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL);

	return ResultFromShort(idCmd-idCmdFirst);	//Must return number of menu
	//items we added.
}

BOOL CWShellExtension::runScan(HWND hwnd)
{
    size_t len;
    if(!m_szPath || !_tcslen(m_szPath))
    {
        MessageBox(hwnd, _T("Error: Unable to retrieve Path."), _T("ClamWin Free Antivirus"), MB_OK | MB_ICONERROR);
        return FALSE;
    }

    DWORD dwType, cbData;
    TCHAR szClamWinPath[MAX_PATH] = _T("");
    TCHAR szParams[MAX_PATH*2] = _T("");
    TCHAR szPathExpanded[MAX_PATH], szParamsExpanded[MAX_PATH*2];
    // get path to ClamWin
    // Try registry first
    HKEY hKey;
    const TCHAR* shellExtRegPath = _T("Software\\Classes\\CLSID\\{65713842-C410-4f44-8383-BFE01A398C90}\\InProcServer32");
    // try in hkey_current_user
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\ClamWin"), 0, KEY_READ, &hKey))
    {
        cbData = sizeof(szClamWinPath);
        RegQueryValueEx(hKey, _T("Path"), NULL, &dwType, (PBYTE)szClamWinPath, &cbData);
        RegCloseKey(hKey);
    }
    // try in hkey_local_machine if failed
    if (!_tcslen(szClamWinPath) &&
    		(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\ClamWin"), 0, KEY_READ, &hKey)))

    {
        cbData = sizeof(szClamWinPath);
        RegQueryValueEx(hKey, _T("Path"), NULL, &dwType, (PBYTE)szClamWinPath, &cbData);
        RegCloseKey(hKey);
    }

    if(!_tcslen(szClamWinPath))
    {
        // Derive ClamWin folder from shell-extension registration path.
        TCHAR szShellExtPath[MAX_PATH] = _T("");
        if ((ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, shellExtRegPath, 0, KEY_READ, &hKey)) ||
            (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, shellExtRegPath, 0, KEY_READ, &hKey)))
        {
            cbData = sizeof(szShellExtPath);
            if (ERROR_SUCCESS == RegQueryValueEx(hKey, NULL, NULL, &dwType, (PBYTE)szShellExtPath, &cbData) && _tcslen(szShellExtPath))
            {
                _tcsncpy(szClamWinPath, szShellExtPath, MAX_PATH - 1);
                szClamWinPath[MAX_PATH - 1] = _T('\0');

                TCHAR *sep = _tcsrchr(szClamWinPath, _T('\\'));
                if (!sep)
                    sep = _tcsrchr(szClamWinPath, _T('/'));

                if (sep)
                    *sep = _T('\0');
            }
            RegCloseKey(hKey);
        }
    }
    len = _tcslen(szClamWinPath);
    if(!len)
    {
        MessageBox(hwnd, _T("Error: Unable to retrieve path to ClamWin. Please reinstall ClamWin Free Antivirus"), _T("ClamWin Free Antivirus"), MB_OK | MB_ICONERROR);
        return FALSE;
    }
    // Expand Env
    ExpandEnvironmentStrings(szClamWinPath, szPathExpanded, _countof(szPathExpanded));
    len = _tcslen(szPathExpanded);
    // remove trailing slash
    if(szPathExpanded[len-1] == _T('\\'))
        szPathExpanded[len-1] = _T('\0');
#if defined(UNICODE) || defined(_UNICODE)
    std::wstring scanPathArgs = m_szPath;
    std::wstring command = CWBuildShellScannerCommand(szPathExpanded, scanPathArgs, L"");
#else
    std::string scanPathArgs = m_szPath;
    std::string command = CWBuildShellScannerCommand(szPathExpanded, scanPathArgs, "");
#endif

    // read  optional params from registry
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, shellExtRegPath, 0, KEY_READ, &hKey) ||
        ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, shellExtRegPath, 0, KEY_READ, &hKey))
    {
        cbData = sizeof(szParams);
        RegQueryValueEx(hKey, _T("params"), NULL, &dwType, (PBYTE)szParams, &cbData);
        RegCloseKey(hKey);
        // append params if exist
        if (szParams[0] != _T('\0'))
        {
            // Expand Env
            ExpandEnvironmentStrings(szParams, szParamsExpanded, _countof(szParamsExpanded));
            command = CWBuildShellScannerCommand(szPathExpanded, scanPathArgs, szParamsExpanded);
        }
    }

#if defined(UNICODE) || defined(_UNICODE)
    std::vector<wchar_t> commandBuf(command.begin(), command.end());
#else
    std::vector<char> commandBuf(command.begin(), command.end());
#endif
    commandBuf.push_back('\0');

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);

    ZeroMemory( &pi, sizeof(pi) );

    // Start the child process.
    if( !CreateProcess( NULL, // No module name (use command line).
        commandBuf.data(),            // Command line.
        NULL,             // Process handle not inheritable.
        NULL,             // Thread handle not inheritable.
        FALSE,            // Set handle inheritance to FALSE.
        0,                // No creation flags.
        NULL,             // Use parent's environment block.
        NULL,             // Use parent's starting directory.
        &si,              // Pointer to STARTUPINFO structure.
        &pi )             // Pointer to PROCESS_INFORMATION structure.
    )
    {
        TCHAR szMsg[MAX_PATH*2];
        _sntprintf(szMsg, _countof(szMsg), _T("Error: Unable to execute command %s."), command.c_str());
        MessageBox(hwnd, szMsg, _T("ClamWin Free Antivirus"), MB_OK | MB_ICONERROR);
        return FALSE;
    }

    // Close process and thread handles.
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );

    return TRUE;
}
//
//  FUNCTION: CWShellExtension::InvokeCommand(LPCMINVOKECOMMANDINFO)
//
//  PURPOSE: Called by the shell after the user has selected on of the
//           menu items that was added in QueryContextMenu().
//
//  PARAMETERS:
//    lpcmi - Pointer to an CMINVOKECOMMANDINFO structure
//
//  RETURN VALUE: HRESULT code signifying success or failure;NOERROR if no error
//
//
//  COMMENTS:
//
STDMETHODIMP CWShellExtension::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
	HRESULT			hr = E_INVALIDARG;

	//If HIWORD(lpcmi->lpVerb) then we have been called programmatically
	//and lpVerb is a command that should be invoked.  Otherwise, the shell
	//has called us, and LOWORD(lpcmi->lpVerb) is the menu ID the user has
	//selected.  Actually, it's (menu ID - idCmdFirst) from QueryContextMenu().
	if (!HIWORD(lpcmi->lpVerb))
	{
		UINT idCmd = LOWORD(lpcmi->lpVerb);
		switch (idCmd)
		{
        case 0:
            runScan(lpcmi->hwnd);
			break;
		}
		hr = NOERROR;
	}
	return hr;
}


//
//  FUNCTION: CWShellExtension::GetCommandString(UINT idCmd,UINT uFlags,UINT FAR *reserved,LPSTR pszName,UINT cchMax)
//
//  PURPOSE: Called by the shell to retreive the cononical command name
//			 or help text for a menu item added by a context menu extension handler
//
//  PARAMETERS:
//	  idCmd - Menu item ID offset
//    uFlags - Value specifying the type of information to retreive
//    *reserved - Pointer to reserved value
//    pszName - Pointer to buffer to receive name string or help text
//    cchMax - Buffer size
//
//  RETURN VALUE: HRESULT code signifying success or failure;NOERROR if no error
//
//
//  COMMENTS:
//
STDMETHODIMP CWShellExtension::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{

	switch (idCmd)
	{
	case 0:
		if (GCS_HELPTEXTW == uType)
			wcsncpy((LPWSTR)pszName, L"ClamWin Free Antivirus", cchMax);
		else if (GCS_HELPTEXTA == uType)
			strncpy(pszName, "ClamWin Free Antivirus", cchMax);
		break;
   }
	return NOERROR;
}
