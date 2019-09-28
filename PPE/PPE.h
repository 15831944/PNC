// PPE.h : main header file for the PPE application
//

#if !defined(AFX_PPE_H__643D2645_3F1A_4977_B732_71AC083A7B83__INCLUDED_)
#define AFX_PPE_H__643D2645_3F1A_4977_B732_71AC083A7B83__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "afxwinappex.h"

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#ifndef __PNC_
	#ifndef __PROCESS_PLATE_
		#define __PROCESS_PLATE_
	#endif
	#ifndef __PROCESS_ANGLE_
		#define __PROCESS_ANGLE_
	#endif
#endif

#include "resource.h"       // main symbols
#include "PPEDoc.h"
#include "PPEView.h"

/////////////////////////////////////////////////////////////////////////////
// CPPEApp:
// See PPE.cpp for the implementation of this class
//

void GetSysPath(char* StartPath);
class CPPEApp : public CWinAppEx
{
	WORD m_wDogModule;
public:
	CPPEApp();
	struct STARTER{
		bool m_bChildProcess;		//�ӽ���ģʽ
		/*	PPE����ģʽ��
		  0.������Ϊ�ӽ�������;
		  1.�������޷���;
		  2.����������ģʽ;
		  3.�๹���޷���;
		  4.�๹������ģʽ;
		  5.���������αȶ�ģʽ;
		  6.ָ���ļ�·��ģʽ
		  7.�๹������ģʽ������������������Ϣģʽ����(����TMA������Ϣ����)
		*/
		BYTE mode;
		BYTE m_cProductType;	//��λΪ1��ʾ�����������ģʽ����֤�顡wjh-2016.12.07
		bool IsMultiPartsMode(){return (mode==0||mode==3||mode==4||mode==5||mode==6||mode==7);}
		bool IsDuplexMode(){return m_bChildProcess&&(mode==2||mode==4||mode==7);}
		bool IsCompareMode(){return m_bChildProcess&&mode==5;}
		bool IsOfferFilePathMode(){return m_bChildProcess && mode==6;}
		bool IsSupportPlateEditing();
		bool IsIncPartPattern(){return m_bChildProcess&&mode==7;}
	};
	STARTER starter;
	CBuffer m_xPPEModelBuffer;	//������Ϣ����
	CPPEDoc* GetDocument();
	CPPEView* GetView();
	void InitPPEModel();
	void GetProductVersion(CString &product_version);
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPPEApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CPPEApp)
	UINT  m_nMainMenuID;
	UINT  m_nAppLook;
	BOOL  m_bHiColorIcons;

	virtual void PreLoadState();
	virtual void LoadCustomState();
	virtual void SaveCustomState();
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
extern CPPEApp theApp;
extern char APP_PATH[MAX_PATH];
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PPE_H__643D2645_3F1A_4977_B732_71AC083A7B83__INCLUDED_)
