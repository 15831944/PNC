// AddBoltBlock.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "AddBoltBlockDlg.h"


// CAddBoltBlock �Ի���

IMPLEMENT_DYNAMIC(CAddBoltBlockDlg, CDialog)

CAddBoltBlockDlg::CAddBoltBlockDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAddBoltBlockDlg::IDD, pParent)
{

}

CAddBoltBlockDlg::~CAddBoltBlockDlg()
{
}

void CAddBoltBlockDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_E_BLOCK_NAME, m_sBlockName);
	DDX_Text(pDX, IDC_E_BOLT_D, m_sBoltD);
	DDX_Text(pDX, IDC_E_HOLE_D, m_sHoleD);
}


BEGIN_MESSAGE_MAP(CAddBoltBlockDlg, CDialog)
END_MESSAGE_MAP()


// CAddBoltBlock ��Ϣ�������
