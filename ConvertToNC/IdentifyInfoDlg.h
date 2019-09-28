#pragma once
#include "Resource.h"

// CIdentifyInfoDlg �Ի���

class CIdentifyInfoDlg : public CDialog
{
	DECLARE_DYNAMIC(CIdentifyInfoDlg)

public:
	CIdentifyInfoDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CIdentifyInfoDlg();

// �Ի�������
	enum { IDD = IDD_IDENTIFY_INFO_DLG };
	BOOL	m_arrIsCanUse[4];
	CString m_sPnKey;
	CString m_sThickKey;
	CString m_sMatKey;
	CString m_sNumKey;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnChkPartNo();
	afx_msg void OnChkThick();
	afx_msg void OnChkMaterial();
	afx_msg void OnChkNum();
};
