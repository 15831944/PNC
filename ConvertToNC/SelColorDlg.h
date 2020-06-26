#pragma once

#include "ColorSelectComboBox.h"
// CSelColorDlg �Ի���

class CSelColorDlg : public CDialog
{
	DECLARE_DYNAMIC(CSelColorDlg)

public:
	CSelColorDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CSelColorDlg();

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SEL_COLOR_DLG };
#endif
	CColorSelectComboBox m_cmbEdgeClr;
	CColorSelectComboBox m_cmbM12Clr;
	CColorSelectComboBox m_cmbM16Clr;
	CColorSelectComboBox m_cmbM20Clr;
	CColorSelectComboBox m_cmbM24Clr;
	CColorSelectComboBox m_cmbOtherClr;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	DECLARE_MESSAGE_MAP()
};
