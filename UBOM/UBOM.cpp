// (C) Copyright 2002-2007 by Autodesk, Inc. 
//
// Permission to use, copy, modify, and distribute this software in
// object code form for any purpose and without fee is hereby granted, 
// provided that the above copyright notice appears in all copies and 
// that both that copyright notice and the limited warranty and
// restricted rights notice below appear in all supporting 
// documentation.
//
// AUTODESK PROVIDES THIS PROGRAM "AS IS" AND WITH ALL FAULTS. 
// AUTODESK SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTY OF
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR USE.  AUTODESK, INC. 
// DOES NOT WARRANT THAT THE OPERATION OF THE PROGRAM WILL BE
// UNINTERRUPTED OR ERROR FREE.
//
// Use, duplication, or disclosure by the U.S. Government is subject to 
// restrictions set forth in FAR 52.227-19 (Commercial Computer
// Software - Restricted Rights) and DFAR 252.227-7013(c)(1)(ii)
// (Rights in Technical Data and Computer Software), as applicable.
//

//-----------------------------------------------------------------------------
//- ConvertToNC.cpp : Initialization functions
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "resource.h"
#include <afxdllx.h>
#include "MenuFunc.h"
#include "XhLdsLm.h"
#include "CadToolfunc.h"
#include "LicFuncDef.h"
#include "RevisionDlg.h"
#include "MsgBox.h"

/////////////////////////////////////////////////////////////////////////////
// ObjectARX EntryPoint

void RegisterServerComponents ()
{	
#ifdef _ARX_2007
	// ������ȡ����Ϣ
	acedRegCmds->addCommand(L"REVISION-MENU",   // Group name
		L"RPP",					// Global function name
		L"RPP",					// Local function name
		ACRX_CMD_MODAL,							// Type
		&RevisionPartProcess);					// Function pointer
#else
	// ������ȡ����Ϣ
	acedRegCmds->addCommand("REVISION-MENU",   // Group name
		"RPP",					// Global function name
		"RPP",					// Local function name
		ACRX_CMD_MODAL,							// Type
		&RevisionPartProcess);					// Function pointer
#endif
}
WORD m_wDogModule=0;
#if !defined(__AFTER_ARX_2007)
extern "C" void ads_queueexpr(ACHAR *);
#endif
void InitApplication()
{
	//���м��ܴ���
	DWORD version[2]={0,20150318};
	BYTE* pByteVer=(BYTE*)version;
	pByteVer[0]=4;
	pByteVer[1]=1;
	pByteVer[2]=0;
	pByteVer[3]=5;
	char lic_file[MAX_PATH],lic_file2[MAX_PATH];
	if(GetLicFile(lic_file)==FALSE&&GetLicFile2(lic_file2)==FALSE)
	{
		AfxMessageBox("֤���ļ�·����ȡʧ��!");
		return;
	}
	CXhChar500 cur_lic_file(lic_file2);
	ULONG retCode=ImportLicFile(lic_file2,PRODUCT_TMA,version);
	if(retCode!=0)
	{
		retCode=ImportLicFile(lic_file,PRODUCT_TMA,version);
		cur_lic_file.Copy(lic_file);
	}
	if(retCode!=0)
	{
		CXhChar500 sError;
		if(retCode==-2)
			sError.Copy("�״�ʹ�ã���δָ����֤���ļ���");
		else if(retCode==-1)
			sError.Copy("��������ʼ��ʧ��!");
		else if(retCode==1)
			sError.Copy("1#�޷���֤���ļ�!");
		else if(retCode==2)
			sError.Copy("2#֤���ļ��⵽�ƻ�!");
		else if(retCode==3)
			sError.Copy("3#֤������ܹ���ƥ��!");
		else if(retCode==4)
			sError.Copy("4#��Ȩ֤��ļ��ܰ汾����!");
		else if(retCode==5)
			sError.Copy("5#֤������ܹ���Ʒ��Ȩ�汾��ƥ��!");
		else if(retCode==6)
			sError.Copy("6#�����汾ʹ����Ȩ��Χ!");
		else if (retCode == 7)
			sError.Copy("7#������Ѱ汾������Ȩ��Χ");
		else if (retCode == 8)
			sError.Copy("8#֤������뵱ǰ���ܹ���һ��");
		else if (retCode == 9)
			sError.Copy("9#��Ȩ���ڣ���������Ȩ");
		else if (retCode == 10)
			sError.Copy("10#����ȱ����Ӧִ��Ȩ�ޣ����Թ���ԱȨ�����г���");
		else if (retCode == 11)
			sError.Copy("11#��Ȩ�쳣����ʹ�ù���ԱȨ����������֤��");
		else
			sError.Printf("δ֪���󣬴������%d#", retCode);
		sError.Append(CXhChar500(".֤��·����%s", lic_file));
		strcpy(BTN_ID_YES_TEXT, "���ļ���");
		strcpy(BTN_ID_NO_TEXT, "�˳�CAD");
		strcpy(BTN_ID_CANCEL_TEXT, "�ر�");
		int nRetCode = MsgBox(adsw_acadMainWnd(), sError, "������ʾ", MB_YESNOCANCEL | MB_ICONERROR);
		if (nRetCode == IDYES)
		{
			CXhChar500 sPath(lic_file);
			sPath.Replace("UBOM.lic", "");	//�Ƴ�UBOM.lic
			sPath.Replace("TMA.lic", "");	//�Ƴ�TMA.lic
			ShellExecute(NULL, "open", NULL, NULL, sPath, SW_SHOW);
			exit(0);
		}
		else if (nRetCode == IDNO)
			exit(0);
		else
		{	//ж��UBOM�����˳�cad
			//kLoadDwgMsg�в��ܵ���sendStringToExecute��acedCommand,acDocManager->curDocument()==NULL,�޷�ִ�� wht 18-12-25
#ifdef _ARX_2007
			ads_queueexpr(L"(command\"arx\" \"u\" \"UBOM.arx\")");
#else
			ads_queueexpr("(command\"arx\" \"u\" \"UBOM.arx\")");
#endif
			return;
		}
		ExitCurrentDogSession();
		exit(0);
	}
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_UBOM))
	{
		AfxMessageBox("���ȱ�ٺϷ�ʹ����Ȩ!");
		exit(0);
	}
	RegisterServerComponents();
	//�����Ի���
	g_pRevisionDlg = new CRevisionDlg(); 
}
void UnloadApplication()
{
	delete g_pRevisionDlg;
	//
#ifndef _LEGACY_LICENSE
	ExitCurrentDogSession();
#elif defined(_NET_DOG)
	ExitNetLicEnv(m_wDogModule);
#endif
	char sGroupName[100]="NC-MENU";
#ifdef _ARX_2007
	acedRegCmds->removeGroup((ACHAR*)_bstr_t(sGroupName));
#else
	acedRegCmds->removeGroup(sGroupName);
#endif
}

//-----------------------------------------------------------------------------
//- Define the sole extension module object.
AC_IMPLEMENT_EXTENSION_MODULE(UbomDLL)
//- Please do not remove the 3 following lines. These are here to make .NET MFC Wizards
//- running properly. The object will not compile but is require by .NET to recognize
//- this project as being an MFC project
#ifdef NEVER
AFX_EXTENSION_MODULE UbomDLL ={ NULL, NULL } ;
#endif

//- Now you can use the CAcModuleResourceOverride class in
//- your application to switch to the correct resource instance.
//- Please see the ObjectARX Documentation for more details

//-----------------------------------------------------------------------------
//- DLL Entry Point
extern "C"
BOOL WINAPI DllMain (HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/) {
	//- Remove this if you use lpReserved
	//UNREFERENCED_PARAMETER(lpReserved) ;

	if ( dwReason == DLL_PROCESS_ATTACH ) 
	{
        //_hdllInstance =hInstance ;
		UbomDLL.AttachInstance (hInstance) ;
		InitAcUiDLL () ;
	}
	else if ( dwReason == DLL_PROCESS_DETACH )
	{
		UbomDLL.DetachInstance () ;
	}
	return (TRUE) ;
}

extern "C" AcRx::AppRetCode acrxEntryPoint(AcRx::AppMsgCode msg, void* pkt)
{
	switch (msg) {
	case AcRx::kInitAppMsg:
		// Comment out the following line if your
		// application should be locked into memory
		acrxDynamicLinker->unlockApplication(pkt);
		acrxDynamicLinker->registerAppMDIAware(pkt);
		InitApplication();
		break;
	case AcRx::kUnloadAppMsg:
		UnloadApplication();
		break;
	case AcRx::kLoadDwgMsg:
		InitDrawingEnv();
		break;
	}
	return AcRx::kRetOK;
}

