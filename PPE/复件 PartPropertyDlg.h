#pragma once


// CPartPropertyDlg �Ի���

class CPartPropertyDlg : public CDialog
{
	DECLARE_DYNCREATE(CPartPropertyDlg)
public:
	CPartPropertyDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CPartPropertyDlg();

// �Ի�������
	enum { IDD = IDD_PART_PROPERTY_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
};
