//-----------------------------------------------------------------------------
// Name:        cw_shell_extension.cpp
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
// Initialize GUIDs (should be done only and at-least once per DLL/EXE)
//
#include <windows.h>
#include <shlobj.h>
#define INITGUID
#include <initguid.h>
#include "cw_shell_extension.h"


//
// Global variables
//
UINT      s_dllRefCount = 0;
HINSTANCE s_moduleHandle = NULL;

extern "C" int APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		s_moduleHandle = hInstance;

	return TRUE;   // ok
}

//---------------------------------------------------------------------------
// DllCanUnloadNow
//---------------------------------------------------------------------------
STDAPI  DllCanUnloadNow(void)
{
	return (s_dllRefCount == 0 ? S_OK : S_FALSE);
}

//---------------------------------------------------------------------------
// DllGetClassObject
//---------------------------------------------------------------------------
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvOut)
{
	*ppvOut = NULL;

	if (IsEqualIID(rclsid, CLSID_ShellExtension))
	{
		CWShellExtensionClassFactory *pcf = new CWShellExtensionClassFactory;

		return pcf->QueryInterface(riid, ppvOut);
	}

	return CLASS_E_CLASSNOTAVAILABLE;
}

//---------------------------------------------------------------------------
// CWShellExtensionClassFactory::CWShellExtensionClassFactory
//---------------------------------------------------------------------------

CWShellExtensionClassFactory::CWShellExtensionClassFactory()
{
	m_cRef = 0L;

	s_dllRefCount++;
}

//---------------------------------------------------------------------------
// CWShellExtensionClassFactory::~CWShellExtensionClassFactory
//---------------------------------------------------------------------------

CWShellExtensionClassFactory::~CWShellExtensionClassFactory()
{
	s_dllRefCount--;
}

//---------------------------------------------------------------------------
// CWShellExtensionClassFactory::QueryInterface
//---------------------------------------------------------------------------

STDMETHODIMP CWShellExtensionClassFactory::QueryInterface(REFIID riid,
                                                   LPVOID FAR *ppv)
{
	*ppv = NULL;

	// Any interface on this object is the object pointer

	if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
	{
		*ppv = (LPCLASSFACTORY)this;

		AddRef();

		return NOERROR;
	}

	return E_NOINTERFACE;
}

//---------------------------------------------------------------------------
// CWShellExtensionClassFactory::AddRef
//---------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CWShellExtensionClassFactory::AddRef()
{
	return ++m_cRef;
}

//---------------------------------------------------------------------------
// CWShellExtensionClassFactory::Release
//---------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CWShellExtensionClassFactory::Release()
{
	if (--m_cRef)
		return m_cRef;

	delete this;

	return 0L;
}

//---------------------------------------------------------------------------
// CWShellExtensionClassFactory::CreateInstance
//---------------------------------------------------------------------------

STDMETHODIMP CWShellExtensionClassFactory::CreateInstance(LPUNKNOWN pUnkOuter,
																	REFIID riid,
																	LPVOID *ppvObj)
{
	*ppvObj = NULL;

	// Shell extensions typically don't support aggregation (inheritance)

	if (pUnkOuter)
		return CLASS_E_NOAGGREGATION;

	// Create the main shell extension object.  The shell will then call
	// QueryInterface with IID_IShellExtInit--this is how shell extensions are
	// initialized.

	CWShellExtensionPtr pShellExt = new CWShellExtension();

	if (NULL == pShellExt)
		return E_OUTOFMEMORY;

	return pShellExt->QueryInterface(riid, ppvObj);
}

//---------------------------------------------------------------------------
// CWShellExtensionClassFactory::LockServer
//---------------------------------------------------------------------------

STDMETHODIMP CWShellExtensionClassFactory::LockServer(BOOL fLock)
{
	return NOERROR;
}

// *********************** CWShellExtension *************************

//---------------------------------------------------------------------------
// CWShellExtension::CWShellExtension
//---------------------------------------------------------------------------

CWShellExtension::CWShellExtension()
{
	m_cRef = 0L;
	m_pDataObj = NULL;
	m_szPath = NULL;

	s_dllRefCount++;
}

//---------------------------------------------------------------------------
// CWShellExtension::~CWShellExtension
//---------------------------------------------------------------------------

CWShellExtension::~CWShellExtension()
{
    if (m_szPath)
        delete [] m_szPath;
	if (m_pDataObj)
		m_pDataObj->Release();

	s_dllRefCount--;
}

//---------------------------------------------------------------------------
// CWShellExtension::QueryInterface
//---------------------------------------------------------------------------

STDMETHODIMP CWShellExtension::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
	*ppv = NULL;

	if (IsEqualIID(riid, IID_IShellExtInit) || IsEqualIID(riid, IID_IUnknown))
		*ppv = (LPSHELLEXTINIT)this;
	else if (IsEqualIID(riid, IID_IContextMenu))
		*ppv = (LPCONTEXTMENU)this;
	if (*ppv)
	{
		AddRef();
		return NOERROR;
	}
	return E_NOINTERFACE;
}

//---------------------------------------------------------------------------
// CWShellExtension::AddRef
//---------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CWShellExtension::AddRef()
{
	return ++m_cRef;
}

//---------------------------------------------------------------------------
// CWShellExtension::Release
//---------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CWShellExtension::Release()
{
	if (--m_cRef)
		return m_cRef;

	delete this;

	return 0L;
}
