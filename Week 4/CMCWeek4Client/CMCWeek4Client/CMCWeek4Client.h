
// CMCWeek4Client.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CCMCWeek4ClientApp:
// See CMCWeek4Client.cpp for the implementation of this class
//

class CCMCWeek4ClientApp : public CWinApp
{
public:
	CCMCWeek4ClientApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CCMCWeek4ClientApp theApp;
