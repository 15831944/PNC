#pragma once
#ifndef __IN_PUT_DLG_
#define __IN_PUT_DLG_

// CInputDlg �Ի���
#include "resource.h"

class CInputDlg : public CDialog
{
	DECLARE_DYNAMIC(CInputDlg)

public:
	CInputDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CInputDlg();
	// �Ի�������
	enum { IDD = IDD_INPUT_DLG };
	CString m_sInputValue;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnEnChangeEInput();
	afx_msg void OnBtnClickedOk();
};

#endif