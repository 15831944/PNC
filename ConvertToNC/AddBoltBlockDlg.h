#pragma once
#include "resource.h"

// CAddBoltBlock �Ի���

class CAddBoltBlockDlg : public CDialog
{
	DECLARE_DYNAMIC(CAddBoltBlockDlg)

public:
	CAddBoltBlockDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CAddBoltBlockDlg();

// �Ի�������
	enum { IDD = IDD_ADD_BOLT_BLOCK_DLG };
	CString m_sBlockName;
	CString m_sBoltD;
	CString m_sHoleD;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
};
