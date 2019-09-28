
// PNCDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"
#include "CADExecutePath.h"

// CPNCDlg �Ի���
class CPNCDlg : public CDialogEx
{
// ����
public:
	CPNCDlg(CWnd* pParent = NULL);	// ��׼���캯��	

// �Ի�������
	enum { IDD = IDD_PNC_DIALOG };
	int m_nRightMargin;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��
	virtual BOOL OnInitDialog();
	BOOL GetCadPath(char* cad_path);
	// ʵ��
public:
	HICON m_hIcon;
	CComboBox m_xPathCmbBox;
	CString m_sPath;
	ARRAY_LIST<CAD_PATH> m_xCadPathArr;
	CString m_sRxFile;
	// ���ɵ���Ϣӳ�亯��
	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnOK();
	afx_msg void OnCancel();
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnBtnRunPPE();
	afx_msg void OnBnClickedBtnAbout();
};
