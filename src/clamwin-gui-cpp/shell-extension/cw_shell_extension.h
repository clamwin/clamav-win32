//-----------------------------------------------------------------------------
// Name:        cw_shell_extension.h
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

//
// The class ID of this Shell extension class.
//
// class id:  65713842-C410-4f44-8383-BFE01A398C90
//
//

#ifndef CW_SHELL_EXTENSION_H
#define CW_SHELL_EXTENSION_H

// {65713842-C410-4f44-8383-BFE01A398C90}
DEFINE_GUID(CLSID_ShellExtension,
				0x65713842, 0xc410, 0x4f44, 0x83, 0x83, 0xbf, 0xe0, 0x1a, 0x39, 0x8c, 0x90);

class CWShellExtensionClassFactory : public IClassFactory
{
protected:
	ULONG	m_cRef;					// Object reference count

public:
	CWShellExtensionClassFactory();
	~CWShellExtensionClassFactory();

	//IUnknown members
	STDMETHODIMP			QueryInterface(REFIID, LPVOID FAR *);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	//IClassFactory members
	STDMETHODIMP		CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
	STDMETHODIMP		LockServer(BOOL);

};
using CWShellExtensionClassFactoryPtr = CWShellExtensionClassFactory*;

// this is the actual OLE Shell context menu handler
class CWShellExtension : public IContextMenu,
IShellExtInit
{
protected:
	ULONG			m_cRef;					// Object reference count
	LPDATAOBJECT	m_pDataObj;

public:
	CWShellExtension();
	~CWShellExtension();

	//IUnknown members
	STDMETHODIMP			QueryInterface(REFIID, LPVOID FAR *);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	//IShell members
	STDMETHODIMP			QueryContextMenu(HMENU hMenu, UINT indexMenu,  UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
	STDMETHODIMP			InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi);
    STDMETHODIMP            GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax);
	//IShellExtInit methods
	STDMETHODIMP		    Initialize(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID);
private:
	BOOL runScan(HWND hwnd);
	PTCHAR m_szPath;
};
using CWShellExtensionPtr = CWShellExtension*;

#endif // CW_SHELL_EXTENSION_H
