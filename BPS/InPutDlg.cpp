// InputDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "InputDlg.h"

// CInputDlg �Ի���

IMPLEMENT_DYNAMIC(CInputDlg, CDialog)

	CInputDlg::CInputDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CInputDlg::IDD, pParent)
	, m_sInputValue(_T(""))
{

}

CInputDlg::~CInputDlg()
{
}

void CInputDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_E_INPUT, m_sInputValue);
}

BEGIN_MESSAGE_MAP(CInputDlg, CDialog)
	ON_EN_CHANGE(IDC_E_INPUT, &CInputDlg::OnEnChangeEInput)
	ON_BN_CLICKED(IDC_BTN_OK, &CInputDlg::OnBtnClickedOk)
END_MESSAGE_MAP()


// CInputDlg ��Ϣ�������
void CInputDlg::OnEnChangeEInput()
{
	UpdateData();
}

void CInputDlg::OnBtnClickedOk()
{
	CDialog::OnOK();
}
