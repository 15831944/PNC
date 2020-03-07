
// PNCDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "PNC.h"
#include "PNCDlg.h"
#include "afxdialogex.h"
#include "InputDlg.h"
#include "Buffer.h"
#include "XhLicAgent.h"
#include "CADExecutePath.h"
#include "AboutDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CPNCDlg �Ի���
CPNCDlg::CPNCDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPNCDlg::IDD, pParent)
	, m_sPath(_T(""))
{
#ifdef __UBOM_ONLY_
	m_hIcon = AfxGetApp()->LoadIcon(IDI_UBOM);
#else
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
#endif
	m_nRightMargin=0;
	m_bChkEnableDocWnd = FALSE;
}

void CPNCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_xPathCmbBox);
	DDX_CBString(pDX, IDC_COMBO1, m_sPath);
	DDX_Check(pDX, IDC_CHK_ENABLE_DOCK_WND, m_bChkEnableDocWnd);
}

BEGIN_MESSAGE_MAP(CPNCDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_QUERYDRAGICON()
	ON_CBN_SELCHANGE(IDC_COMBO1, &CPNCDlg::OnCbnSelchangeCombo1)
	ON_COMMAND(ID_BTN_PPE,OnBtnRunPPE)
	ON_BN_CLICKED(IDC_BTN_ABOUT, &CPNCDlg::OnBnClickedBtnAbout)
END_MESSAGE_MAP()

// CPNCDlg ��Ϣ�������
BOOL CPNCDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	m_xPathCmbBox.InsertString(0,CString("<�༭...>"));
	m_xPathCmbBox.InsertString(1,CString("<���...>"));
	m_xPathCmbBox.InsertString(2,CString("<�Զ�����...>"));
	//ͨ��ע����ҳ�CAD������·��
	InitCadPathList(m_xCadPathArr);
	char cadPath[MAX_PATH] = {0};
	char sSubKey[MAX_PATH] = "Software\\Xerofox\\PNC\\Settings";
#ifdef __UBOM_ONLY_
	strcpy(sSubKey,"Software\\Xerofox\\UBOM\\Settings");
#endif
	GetRegKey(HKEY_CURRENT_USER, sSubKey, "CAD_PATH", cadPath);
	int iCurIndex = 3, iActivedCad = -1, iFirstValidCad = -1;
	for (int i = 0; i < m_xCadPathArr.GetSize(); i++)
	{
		if (m_xCadPathArr[i].sCadPath.GetLength() > 0)
		{
			m_xPathCmbBox.InsertString(iCurIndex, CString(m_xCadPathArr[i].sCadPath));
			if (m_xCadPathArr[i].bUsed||m_xCadPathArr[i].sCadPath.EqualNoCase(cadPath))
				iActivedCad = iCurIndex;
			if (iFirstValidCad == -1)
				iFirstValidCad = iCurIndex;
			iCurIndex++;
		}
		if(iActivedCad>=0)
			m_xPathCmbBox.SetCurSel(iActivedCad);
		else if (iFirstValidCad >= 0)
			m_xPathCmbBox.SetCurSel(iFirstValidCad);
		else
			m_xPathCmbBox.SetCurSel(2);
	}
	//
	RECT rect1,clientRect;
	GetClientRect(&clientRect);
	m_xPathCmbBox.GetWindowRect(&rect1);
	ScreenToClient(&rect1);
	m_nRightMargin=clientRect.right-rect1.right;
#ifdef __UBOM_ONLY_
	GetDlgItem(ID_BTN_PPE)->ShowWindow(SW_HIDE);
	GetDlgItem(IDOK)->SetWindowText("����UBOM����");
	//��ť����
	RECT ppeBtnRc,pncRc;
	GetDlgItem(ID_BTN_PPE)->GetWindowRect(&ppeBtnRc);
	ScreenToClient(&ppeBtnRc);
	GetDlgItem(IDOK)->GetWindowRect(&pncRc);
	ScreenToClient(&pncRc);
	int width = pncRc.right - pncRc.left;
	pncRc.left = ppeBtnRc.left;
	pncRc.right = pncRc.left + width;
	GetDlgItem(IDOK)->MoveWindow(&pncRc);
	//
	SetWindowText("UBOM");
	RefreshCheckShowState();
#endif
	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CPNCDlg::RefreshCheckShowState()
{
#ifdef __UBOM_ONLY_
	int iCurSel = m_xPathCmbBox.GetCurSel();
	CString sCadPath,sCadName;
	if (iCurSel > 0)
	{
		m_xPathCmbBox.GetLBText(iCurSel, sCadPath);
		sCadName.Format("%sacad.exe", sCadPath);
	}
	CString sProductVersion = GetProductVersion(sCadName);
	if (sProductVersion.Find("16.") == 0)
		GetDlgItem(IDC_CHK_ENABLE_DOCK_WND)->ShowWindow(SW_SHOW);
	else
		GetDlgItem(IDC_CHK_ENABLE_DOCK_WND)->ShowWindow(SW_HIDE);
#else
	GetDlgItem(IDC_CHK_ENABLE_DOCK_WND)->ShowWindow(SW_HIDE);
#endif
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CPNCDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CPNCDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
//
void CPNCDlg::OnCbnSelchangeCombo1()
{
	char cadPath[MAX_PATH],sSubKey[MAX_PATH];
	int nIndex=m_xPathCmbBox.GetCurSel();
	CString path_str;
	if(nIndex==0)
	{
		CInputDlg dlg;
		if(dlg.DoModal()==IDOK)
			path_str=dlg.m_sInputValue;
	}
	else if(nIndex==1)
	{
		CFileDialog dlg(TRUE,"exe","acad.exe",
			OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_ALLOWMULTISELECT,
			"��ִ���ļ�(*.exe)|*.exe|�����ļ�(*.*)|*.*||");
		if(dlg.DoModal()==IDOK)
			path_str=dlg.GetPathName();
	}
	else if(nIndex==2)
	{
#ifdef __UBOM_ONLY_
		strcpy(sSubKey,"Software\\Xerofox\\UBOM\\Settings");
#else
		strcpy(sSubKey, "Software\\Xerofox\\PNC\\Settings");
#endif
		SetRegKey(HKEY_CURRENT_USER,sSubKey,"CAD_PATH","");
	}
	else
		m_xPathCmbBox.GetLBText(nIndex,path_str);
	nIndex=m_xPathCmbBox.FindString(0,path_str);
	if(nIndex<0 && path_str.GetLength()>0)
	{
		int nCount=m_xPathCmbBox.GetCount();
		m_xPathCmbBox.InsertString(nCount,path_str);
	}
	UpdateData();
	m_sPath=path_str;
	UpdateData(FALSE);
	//����CAD·����ע�����
	if(GetCadPath(cadPath))
	{
#ifdef __UBOM_ONLY_
		strcpy(sSubKey, "Software\\Xerofox\\UBOM\\Settings");
#else
		strcpy(sSubKey, "Software\\Xerofox\\PNC\\Settings");
#endif
		SetRegKey(HKEY_CURRENT_USER,sSubKey,"CAD_PATH",cadPath);
	}
	RefreshCheckShowState();
}
void CPNCDlg::OnSize(UINT nType, int cx, int cy)
{
	RECT rect;
	CWnd* pWnd=GetDlgItem(IDC_COMBO1);
	if(pWnd->GetSafeHwnd()!=NULL)
	{
		pWnd->GetWindowRect(&rect);
		ScreenToClient(&rect);
		rect.right=cx-m_nRightMargin;
		pWnd->MoveWindow(&rect);
	}
	/*pWnd=GetDlgItem(IDOK);
	if(pWnd->GetSafeHwnd()!=NULL)
	{
		pWnd->GetWindowRect(&rect);
		ScreenToClient(&rect);
		int nLen=rect.right-rect.left;
		rect.left=(long)((cx-nLen)*0.5);
		rect.right=(long)((cx+nLen)*0.5);
		pWnd->MoveWindow(&rect);
	}*/
	CDialog::OnSize(nType, cx, cy);
}
//
void CPNCDlg::OnOK()
{
	UpdateData();
	//����CAD�����Զ�����PNC.arx
	char cadPath[MAX_PATH]="",cadName[MAX_PATH]="";
	GetCadPath(cadPath);
#ifdef __UBOM_ONLY_
	if (StartCadAndLoadArx("UBOM", APP_PATH, cadPath, m_sRxFile, m_hWnd, m_bChkEnableDocWnd))
#else
	if (StartCadAndLoadArx("PNC", APP_PATH, cadPath, m_sRxFile, m_hWnd))
#endif
	{
		//����CAD·����ע�����
		char sSubKey[MAX_PATH]="Software\\Xerofox\\PNC\\Settings";
#ifdef __UBOM_ONLY_
		strcpy(sSubKey, "Software\\Xerofox\\UBOM\\Settings");
#endif
		SetRegKey(HKEY_CURRENT_USER, sSubKey, "CAD_PATH", cadPath);
		return OnCancel();
	}
}
void CPNCDlg::OnCancel()
{
	if(m_sRxFile.GetLength()>1)
		::DeleteFile(m_sRxFile);
	return CDialog::OnCancel();
}
BOOL WriteToClient(HANDLE hPipeWrite)
{
	if( hPipeWrite == INVALID_HANDLE_VALUE )
		return FALSE;
	CBuffer buffer(10000);	//10Kb
	DWORD version[2]={0,20170615};
	BYTE* pByteVer=(BYTE*)version;
	pByteVer[0]=1;
	pByteVer[1]=2;
	pByteVer[2]=0;
	pByteVer[3]=0;
	BYTE cProductType=PRODUCT_PNC;
	//1��д����ܹ�֤����Ϣ
	char APP_PATH[MAX_PATH];
	GetAppPath(APP_PATH);
	buffer.WriteByte(cProductType);
	buffer.WriteDword(version[0]);
	buffer.WriteDword(version[1]);
	buffer.WriteString(APP_PATH,MAX_PATH);
	//2��д��PPE����ģʽ��Ϣ
	buffer.WriteByte(0);
	buffer.WriteInteger(0);
	//3���������ܵ���д������
	return buffer.WriteToPipe(hPipeWrite,1024);
}
void CPNCDlg::OnBtnRunPPE()
{
	//1��������һ���ܵ�: ���ڷ���������ͻ��˷�������
	SECURITY_ATTRIBUTES attrib;
	attrib.nLength = sizeof( SECURITY_ATTRIBUTES );
	attrib.bInheritHandle= true;
	attrib.lpSecurityDescriptor = NULL;
	HANDLE hPipeClientRead=NULL, hPipeSrcWrite=NULL;
	if( !CreatePipe( &hPipeClientRead, &hPipeSrcWrite, &attrib, 0 ) )
		return;
	HANDLE hPipeSrcWriteDup=NULL;
	if( !DuplicateHandle( GetCurrentProcess(), hPipeSrcWrite, GetCurrentProcess(), &hPipeSrcWriteDup, 0, false, DUPLICATE_SAME_ACCESS ) )
		return;
	CloseHandle( hPipeSrcWrite );
	//2�������ڶ����ܵ������ڿͻ�����������˷�������
	HANDLE hPipeClientWrite=NULL, hPipeSrcRead=NULL;
	if( !CreatePipe( &hPipeSrcRead, &hPipeClientWrite, &attrib, 0) )
		return;
	HANDLE hPipeSrcReadDup=NULL;
	if( !DuplicateHandle( GetCurrentProcess(), hPipeSrcRead, GetCurrentProcess(), &hPipeSrcReadDup, 0, false, DUPLICATE_SAME_ACCESS ) )
		return;
	CloseHandle( hPipeSrcRead );
	//3�������ӽ���,����������
	TCHAR cmd_str[MAX_PATH];
	GetAppPath(cmd_str);
	strcat(cmd_str,"PPE.exe -child");

	STARTUPINFO startInfo;
	memset( &startInfo, 0 , sizeof( STARTUPINFO ) );
	startInfo.cb= sizeof( STARTUPINFO );
	startInfo.dwFlags |= STARTF_USESTDHANDLES;
	startInfo.hStdError= 0;;
	startInfo.hStdInput= hPipeClientRead;
	startInfo.hStdOutput= hPipeClientWrite;
	PROCESS_INFORMATION processInfo;
	memset( &processInfo, 0, sizeof(PROCESS_INFORMATION) );
	BOOL bRet=CreateProcess( NULL,cmd_str,NULL,NULL, TRUE,CREATE_NEW_CONSOLE, NULL, NULL,&startInfo,&processInfo);
	if(bRet && WriteToClient(hPipeSrcWriteDup))
	{
		WaitForInputIdle(processInfo.hProcess,INFINITE);
		//ShowWindow(SW_HIDE);
		//Sleep(10000);
		//return OnCancel();
	}
	else
		MessageBox(CXhChar100("�������չ�����ʧ��,%s",cmd_str));	
}
BOOL CPNCDlg::GetCadPath(char* cad_path)
{
	if(m_sPath.GetLength()<=0)
		return FALSE;
	char drive[4],dir[MAX_PATH];
	_splitpath(m_sPath,drive,dir,NULL,NULL);
	strcpy(cad_path,drive);
	strcat(cad_path,dir);
	return TRUE;
}


void CPNCDlg::OnBnClickedBtnAbout()
{
	CAboutDlg dlg;
	dlg.DoModal();
}
