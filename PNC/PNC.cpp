
// PNC.cpp : ����Ӧ�ó��������Ϊ��
//

#include "stdafx.h"
#include "PNC.h"
#include "PNCDlg.h"
#include "direct.h"
#include "XhLmd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CPNCApp

BEGIN_MESSAGE_MAP(CPNCApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CPNCApp ����

CPNCApp::CPNCApp()
{
	// ֧����������������
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: �ڴ˴���ӹ�����룬
	// ��������Ҫ�ĳ�ʼ�������� InitInstance ��
}


// Ψһ��һ�� CPNCApp ����

CPNCApp theApp;


// CPNCApp ��ʼ��
char APP_PATH[MAX_PATH];
void GetAppPath(char* startPath)
{
	char drive[4];
	char dir[MAX_PATH];
	char fname[MAX_PATH];
	char ext[MAX_PATH];

	_splitpath(__argv[0],drive,dir,fname,ext);
	strcpy(startPath,drive);
	strcat(startPath,dir);
	_chdir(startPath);
}
int ImportLicFile(char* licfile,BYTE cProductType,DWORD version[2])
{
	FILE* fp=fopen(licfile,"rb");
	if(fp==NULL)
		return 1;	//�޷���֤���ļ�
	fseek(fp,0,SEEK_END);
	UINT uBufLen=ftell(fp);	//��ȡ��֤�ļ�����
	CHAR_ARRAY buffer_s(uBufLen);
	fseek(fp,0,SEEK_SET);
	fread(buffer_s,uBufLen,1,fp);
	fclose(fp);
	return ImportLicBuffer(buffer_s,uBufLen,cProductType,version);
}
char* SearchChar(char* srcStr,char ch,bool reverseOrder=false)
{
	if(!reverseOrder)
		return strchr(srcStr,ch);
	else
	{
		int len=strlen(srcStr);
		for(int i=len-1;i>=0;i--)
		{
			if(srcStr[i]==ch)
				return &srcStr[i];
		}
	}
	return NULL;
}
bool DetectSpecifiedHaspKeyFile(const char* default_file)
{
	FILE* fp=fopen(default_file,"rt");
	if(fp==NULL)
		return false;
	bool detected=false;
	CXhChar200 line_txt;//[MAX_PATH];
	CXhChar200 scope_xmlstr;
	scope_xmlstr.Append(
		"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"
		"<haspscope>");
	while(!feof(fp))
	{
		if(fgets(line_txt,line_txt.GetLengthMax(),fp)==NULL)
			break;
		line_txt.Replace("��","=");
		char* final=SearchChar(line_txt,';');
		if(final!=NULL)
			*final=0;
		char *skey=strtok(line_txt," =,");
		//��������
		if(_stricmp(skey,"Key")==0)
		{
			if(skey=strtok(NULL,"=,"))
			{
				scope_xmlstr.Append("<hasp id=\"");
				scope_xmlstr.Append(skey);
				scope_xmlstr.Append("\" />");
				detected=true;
			}
		}
	}
	fclose(fp);
	scope_xmlstr.Append("</haspscope>");
	if(detected)
		SetHaspLoginScope(scope_xmlstr);
	return detected;
}
BOOL CPNCApp::InitInstance()
{
	// ���һ�������� Windows XP �ϵ�Ӧ�ó����嵥ָ��Ҫ
	// ʹ�� ComCtl32.dll �汾 6 ����߰汾�����ÿ��ӻ���ʽ��
	//����Ҫ InitCommonControlsEx()�����򣬽��޷��������ڡ�
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// ��������Ϊ��������Ҫ��Ӧ�ó�����ʹ�õ�
	// �����ؼ��ࡣ
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	AfxEnableControlContainer();

	// ���� shell ���������Է��Ի������
	// �κ� shell ����ͼ�ؼ��� shell �б���ͼ�ؼ���
	CShellManager *pShellManager = new CShellManager;

	// ��׼��ʼ��
	// ���δʹ����Щ���ܲ�ϣ����С
	// ���տ�ִ���ļ��Ĵ�С����Ӧ�Ƴ�����
	// ����Ҫ���ض���ʼ������
	// �������ڴ洢���õ�ע�����
	// TODO: Ӧ�ʵ��޸ĸ��ַ�����
	// �����޸�Ϊ��˾����֯��
#ifdef __UBOM_ONLY_
	free((void*)m_pszAppName);
	m_pszAppName = _tcsdup(_T("UBOM"));
#endif

	SetRegistryKey(_T("Xerofox"));
	GetAppPath(APP_PATH);
	WriteProfileString("SETUP","APP_PATH",APP_PATH);
	strcpy(execute_path,APP_PATH);
	char lic_file[MAX_PATH],sys_file[MAX_PATH];
	sprintf(sys_file,"%ssys",APP_PATH);
	WriteProfileString("SETUP","SYS_PATH",sys_file);
	sprintf(lic_file,"%sPNC.lic",APP_PATH);
	WriteProfileString("Settings","lic_file",lic_file);

	DWORD version[2];
	BYTE* byteVer=(BYTE*)version;
	WORD wModule=0;
	version[1]=20170615;	//�汾��������
	byteVer[0]=1;
	byteVer[1]=2;
	byteVer[2]=0;
	byteVer[3]=0;

	m_version[0]=version[0];
	m_version[1]=version[1];
	char default_file[MAX_PATH];
	strcpy(default_file,lic_file);
	//�����Ƿ����ָ���������ŵ��ļ� wht-2017.06.07
	char* separator=SearchChar(default_file,'.',true);
	strcpy(separator,".key");
	DetectSpecifiedHaspKeyFile(default_file);
	DWORD retCode=0;
	bool passed_liccheck=true;
	CXhChar50 errormsgstr("δָ֤���ȡ����");
	for(int i=0;true;i++){
		retCode=ImportLicFile(lic_file,PRODUCT_PNC,version);
		if(retCode==0)
		{
			passed_liccheck=false;
			if(GetLicVersion()<1000005)
#ifdef AFX_TARG_ENU_ENGLISH
				errormsgstr.Copy("The certificate file has been out of date, the current software's version must use the new certificate file��");
#else 
				errormsgstr.Copy("��֤���ļ��ѹ�ʱ����ǰ����汾����ʹ����֤�飡");
#endif
			else if(!VerifyValidFunction(LICFUNC::FUNC_IDENTITY_BASIC))
#ifdef AFX_TARG_ENU_ENGLISH
				errormsgstr.Copy("Software Lacks of legitimate use authorized!");
#else 
				errormsgstr.Copy("���ȱ�ٺϷ�ʹ����Ȩ!");
#endif
			else
			{
				passed_liccheck=true;
				WriteProfileString("Settings","lic_file",lic_file);
			}
			if(!passed_liccheck)
			{
#ifndef _LEGACY_LICENSE
				ExitCurrentDogSession();
#elif defined(_NET_DOG)
				ExitNetLicEnv(m_wDogModule);
#endif
			}
			else
				break;
		}
		else
		{
#ifdef AFX_TARG_ENU_ENGLISH
			if(retCode==-1)
				errormsgstr.Copy("0# Hard dog failed to initialize!");
			else if(retCode==1)
				errormsgstr.Copy("1# Unable to open the license file!");
			else if(retCode==2)
				errormsgstr.Copy("2# License file was damaged!");
			else if(retCode==3||retCode==4)
				errormsgstr.Copy("3# License file not found or does'nt match the hard lock!");
			else if(retCode==5)
				errormsgstr.Copy("5# License file doesn't match the authorized products in hard lock!");
			else if(retCode==6)
				errormsgstr.Copy("6# Beyond the scope of authorized version !");
			else if(retCode==7)
				errormsgstr.Copy("7# Beyond the scope of free authorize version !");
			else if(retCode==8)
				errormsgstr.Copy("8# The serial number of license file does'nt match the hard lock!");
#else 
			if(retCode==-1)
				errormsgstr.Copy("0#���ܹ���ʼ��ʧ��!");
			else if(retCode==1)
				errormsgstr.Copy("1#�޷���֤���ļ�");
			else if(retCode==2)
				errormsgstr.Copy("2#֤���ļ��⵽�ƻ�");
			else if(retCode==3||retCode==4)
				errormsgstr.Copy("3#ִ��Ŀ¼�²�����֤���ļ���֤���ļ�����ܹ���ƥ��");
			else if(retCode==5)
				errormsgstr.Copy("5#֤������ܹ���Ʒ��Ȩ�汾��ƥ��");
			else if(retCode==6)
				errormsgstr.Copy("6#�����汾ʹ����Ȩ��Χ");
			else if(retCode==7)
				errormsgstr.Copy("7#������Ѱ汾������Ȩ��Χ");
			else if(retCode==8)
				errormsgstr.Copy("8#֤������뵱ǰ���ܹ���һ��");
			else if(retCode==9)
				errormsgstr.Copy("9#��Ȩ���ڣ���������Ȩ");
			else if(retCode==10)
				errormsgstr.Copy("10#����ȱ����Ӧִ��Ȩ�ޣ����Թ���ԱȨ�����г���");
#endif
#ifndef _LEGACY_LICENSE
			ExitCurrentDogSession();
#elif defined(_NET_DOG)
			ExitNetLicEnv(m_wDogModule);
#endif
			passed_liccheck=false;
			//return FALSE;
		}
		if(!passed_liccheck)
		{
			DWORD dwRet=ProcessLicenseAuthorize(PRODUCT_PNC,m_version,theApp.execute_path,false,errormsgstr);
			if(dwRet==0)
				return FALSE;
			if(passed_liccheck=(dwRet==1))
				break;	//�ڲ��ѳɹ�����֤���ļ�
		}
	}
	if(!passed_liccheck)
	{
#ifndef _LEGACY_LICENSE
		ExitCurrentDogSession();
#elif defined(_NET_DOG)
		ExitNetLicEnv(m_wDogModule);
#endif
		return FALSE;
	}
	if(!VerifyValidFunction(LICFUNC::FUNC_IDENTITY_BASIC))
	{
		AfxMessageBox("֤����Ȩ������!");
		return FALSE;
	}
	//
	CPNCDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: �ڴ˷��ô����ʱ��
		//  ��ȷ�������رնԻ���Ĵ���
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: �ڴ˷��ô����ʱ��
		//  ��ȡ�������رնԻ���Ĵ���
	}

	// ɾ�����洴���� shell ��������
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}
	// ���ڶԻ����ѹرգ����Խ����� FALSE �Ա��˳�Ӧ�ó���
	//  ����������Ӧ�ó������Ϣ�á�
	return FALSE;
}

int CPNCApp::ExitInstance() 
{
#ifndef _LEGACY_LICENSE
	ExitCurrentDogSession();
#elif defined(_NET_DOG)
	ExitNetLicEnv(m_wDogModule);
#endif
	return CWinApp::ExitInstance();
}
