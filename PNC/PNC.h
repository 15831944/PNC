
// PNC.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������
#include "XhCharString.h"
#include "ArrayList.h"
#include "XhLicAgent.h"
#include "LicFuncDef.h"
//#include "XhLdsLm.h"

// CPNCApp:
// �йش����ʵ�֣������ PNC.cpp
//
void GetAppPath(char* StartPath);
int ImportLicFile(char* licfile,BYTE cProductType,DWORD version[2]);
class CPNCApp : public CWinApp
{
public:
	DWORD m_version[2];
	char execute_path[MAX_PATH];//��ȡִ���ļ�������Ŀ¼
	CPNCApp();

// ��д
public:
	virtual BOOL InitInstance();
	virtual BOOL ExitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CPNCApp theApp;
extern char APP_PATH[MAX_PATH];