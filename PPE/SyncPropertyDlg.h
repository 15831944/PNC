#pragma once


// CSyncPropertyDlg �Ի���
#include "XhListCtrl.h"
#include "ProcessPart.h"
class CSyncPropertyDlg : public CDialog
{
	DECLARE_DYNAMIC(CSyncPropertyDlg)

public:
	CSyncPropertyDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CSyncPropertyDlg();
	void RefeshSyncFlag();
// �Ի�������
	enum { IDD = IDD_SYNC_PROPERTY_DLG };
	CProcessPart* m_pProcessPart;
	CXhListCtrl m_xPropertyList;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnNMRClickListSyncProp(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSetSyncItem();
	afx_msg void OnRevokeSyncItem();
};
