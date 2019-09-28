#pragma once


// CHoleIncrementSetDlg �Ի���

class CHoleIncrementSetDlg : public CDialog
{
	DECLARE_DYNAMIC(CHoleIncrementSetDlg)

public:
	CHoleIncrementSetDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CHoleIncrementSetDlg();

// �Ի�������
	enum { IDD = IDD_HOLE_INCREMENT_DLG };
	BOOL	m_arrIsCanUse[5];
	double  m_fM12Increment;
	double  m_fM16Increment;
	double  m_fM20Increment;
	double  m_fM24Increment;
	double  m_fSpcIncrement;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnChkM12();
	afx_msg void OnChkM16();
	afx_msg void OnChkM20();
	afx_msg void OnChkM24();
	afx_msg void OnChkMSH();
};
