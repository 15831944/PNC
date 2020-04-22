// BPS.cpp : ���� DLL �ĳ�ʼ�����̡�
//

#include "stdafx.h"
#include <afxwin.h>
#include <afxdllx.h>
#include "XhLicAgent.h"
#include "XhLdsLm.h"
#include "LicFuncDef.h"
#include "BPSMenu.h"
#include "BPSModel.h"
#include "CadToolFunc.h"
#include "SelectJgCardDlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

void RegisterServerComponents ()
{	
#ifdef _ARX_2007
	// ������ȡ�Ǹ�
	acedRegCmds->addCommand(L"BPS-MENU",           // Group name
		L"BP",             // Global function name
		L"BP",             // Local function name
		ACRX_CMD_MODAL,   // Type
		&BatchPrintPartProcessCards);            // Function pointer
	// �ϴ��Ǹֹ��տ�
	acedRegCmds->addCommand(L"BPS-MENU",           // Group name
		L"RP",             // Global function name
		L"RP",             // Local function name
		ACRX_CMD_MODAL,   // Type
		&RetrievedProcessCards);            // Function pointer
	// �ϴ��Ǹֹ��տ�
	acedRegCmds->addCommand(L"BPS-MENU",           // Group name
		L"AP",             // Global function name
		L"AP",             // Local function name
		ACRX_CMD_MODAL,   // Type
		&AppendOrUpdateProcessCards);            // Function pointer
	//�Ż���������
	acedRegCmds->addCommand(L"BPS-MENU",           // Group name
		L"OptimalSortSet",        // Global function name
		L"OptimalSortSet",        // Local function name
		ACRX_CMD_MODAL,   // Type
		&OptimalSortSet);            // Function pointer
#else
	// ������ȡ�Ǹ�
	acedRegCmds->addCommand( "BPS-MENU",           // Group name
		"BP",        // Global function name
		"BP",        // Local function name
		ACRX_CMD_MODAL,   // Type
		&BatchPrintPartProcessCards);            // Function pointer
	//�Ż���������
	acedRegCmds->addCommand( "BPS-MENU",           // Group name
		"OptimalSortSet",        // Global function name
		"OptimalSortSet",        // Local function name
		ACRX_CMD_MODAL,   // Type
		&OptimalSortSet);            // Function pointer
#endif
}
void InitApplication()
{
	//���м��ܴ���
	DWORD version[2]={0,20150318};
	BYTE* pByteVer=(BYTE*)version;
	pByteVer[0]=4;
	pByteVer[1]=1;
	pByteVer[2]=0;
	pByteVer[3]=5;
	char lic_file[MAX_PATH];
	if(GetLicFile2(lic_file)==FALSE)
	{
		AfxMessageBox("֤���ļ�·����ȡʧ��!");
		return;
	}
	ULONG retCode=ImportLicFile(lic_file,PRODUCT_TMA,version);
	if(retCode!=0)
	{
		CXhChar500 sInfo;
		if(retCode==-2)
			sInfo.Copy("�״�ʹ�ã���δָ����֤���ļ���");
		else if(retCode==-1)
			sInfo.Copy("��������ʼ��ʧ��");
		else if(retCode==1)
			sInfo.Copy("1#�޷���֤���ļ�");
		else if(retCode==2)
			sInfo.Copy("2#֤���ļ��⵽�ƻ�");
		else if(retCode==3)
			sInfo.Copy("3#֤������ܹ���ƥ��");
		else if(retCode==4)
			sInfo.Copy("4#��Ȩ֤��ļ��ܰ汾����");
		else if(retCode==5)
			sInfo.Copy("5#֤������ܹ���Ʒ��Ȩ�汾��ƥ��");
		else if(retCode==6)
			sInfo.Copy("6#�����汾ʹ����Ȩ��Χ");
		else if(retCode==7)
			sInfo.Copy("7#������Ѱ汾������Ȩ��Χ");
		else if(retCode==8)
			sInfo.Copy("8#֤������뵱ǰ���ܹ���һ��");
		sInfo.Append("֤��·��:");
		sInfo.Append(lic_file);
		AfxMessageBox(sInfo);
		ExitCurrentDogSession();
		exit(0);
	}
	if(!VerifyValidFunction(TMA_LICFUNC::FUNC_IDENTITY_UBOM))
	{
		AfxMessageBox("���ȱ�ٺϷ�ʹ����Ȩ!");
		exit(0);
	}
	HWND hWnd = adsw_acadMainWnd();
	::SetWindowText(hWnd,"BPS");
	RegisterServerComponents();
}
void UnloadApplication()
{
	ExitCurrentDogSession();	//�˳����ܹ�
	char sGroupName[100]="BPS-MENU";
#ifdef _ARX_2007
	acedRegCmds->removeGroup((ACHAR*)_bstr_t(sGroupName));
#else
	acedRegCmds->removeGroup(sGroupName);
#endif
}

static AFX_EXTENSION_MODULE BPSDLL = { NULL, NULL };
extern "C" int APIENTRY
	DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	// ���ʹ�� lpReserved���뽫���Ƴ�
	UNREFERENCED_PARAMETER(lpReserved);
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		TRACE0("BPS.DLL ���ڳ�ʼ��!\n");

		// ��չ DLL һ���Գ�ʼ��
		if (!AfxInitExtensionModule(BPSDLL, hInstance))
			return 0;

		new CDynLinkLibrary(BPSDLL);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		TRACE0("BPS.DLL ������ֹ!\n");

		// �ڵ�����������֮ǰ��ֹ�ÿ�
		AfxTermExtensionModule(BPSDLL);
	}
	return 1;   // ȷ��
}
extern "C" 
AcRx::AppRetCode acrxEntryPoint(AcRx::AppMsgCode msg, void* pkt)
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