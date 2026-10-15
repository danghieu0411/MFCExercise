
// CMCWeek4.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CCMCWeek4App:
// See CMCWeek4.cpp for the implementation of this class
//

class CCMCWeek4App : public CWinApp
{
public:
	CCMCWeek4App();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CCMCWeek4App theApp;
