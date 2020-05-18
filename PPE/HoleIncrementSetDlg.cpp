// HoleIncrementSetDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "PPE.h"
#include "HoleIncrementSetDlg.h"
#include "afxdialogex.h"
#include "SysPara.h"


// CHoleIncrementSetDlg �Ի���

IMPLEMENT_DYNAMIC(CHoleIncrementSetDlg, CDialog)

CHoleIncrementSetDlg::CHoleIncrementSetDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CHoleIncrementSetDlg::IDD, pParent)
{
	m_arrIsCanUse[0]=FALSE;
	m_arrIsCanUse[1]=FALSE;
	m_arrIsCanUse[2]=FALSE;
	m_arrIsCanUse[3]=FALSE;
	m_arrIsCanUse[4]=FALSE;
}

CHoleIncrementSetDlg::~CHoleIncrementSetDlg()
{
}

void CHoleIncrementSetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHK_M12, m_arrIsCanUse[0]);
	DDX_Check(pDX, IDC_CHK_M16, m_arrIsCanUse[1]);
	DDX_Check(pDX, IDC_CHK_M20, m_arrIsCanUse[2]);
	DDX_Check(pDX, IDC_CHK_M24, m_arrIsCanUse[3]);
	DDX_Check(pDX, IDC_CHK_CUT_SH, m_arrIsCanUse[4]);
	DDX_Check(pDX, IDC_CHK_PRO_SH, m_arrIsCanUse[5]);
	DDX_Text(pDX, IDC_E_M12, m_fM12Increment);
	DDX_Text(pDX, IDC_E_M16, m_fM16Increment);
	DDX_Text(pDX, IDC_E_M20, m_fM20Increment);
	DDX_Text(pDX, IDC_E_M24, m_fM24Increment);
	DDX_Text(pDX, IDC_E_CUT_SH, m_fCutIncrement);
	DDX_Text(pDX, IDC_E_PRO_SH, m_fProIncrement);
}


BEGIN_MESSAGE_MAP(CHoleIncrementSetDlg, CDialog)
	ON_BN_CLICKED(IDC_CHK_M12, OnChkM12)
	ON_BN_CLICKED(IDC_CHK_M16, OnChkM16)
	ON_BN_CLICKED(IDC_CHK_M20, OnChkM20)
	ON_BN_CLICKED(IDC_CHK_M24, OnChkM24)
	ON_BN_CLICKED(IDC_CHK_CUT_SH, OnChkCutSH)
	ON_BN_CLICKED(IDC_CHK_PRO_SH, OnChkProSH)
END_MESSAGE_MAP()


// CHoleIncrementSetDlg ��Ϣ�������
BOOL CHoleIncrementSetDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	double fDatum=g_sysPara.holeIncrement.m_fDatum;
	//M12
	m_fM12Increment=g_sysPara.holeIncrement.m_fM12;
	if(fabs(m_fM12Increment-fDatum)>EPS)
		m_arrIsCanUse[0]=TRUE;
	((CEdit*)GetDlgItem(IDC_E_M12))->SetReadOnly(!m_arrIsCanUse[0]);
	//M16
	m_fM16Increment=g_sysPara.holeIncrement.m_fM16;
	if(fabs(m_fM16Increment-fDatum)>EPS)
		m_arrIsCanUse[1]=TRUE;
	((CEdit*)GetDlgItem(IDC_E_M16))->SetReadOnly(!m_arrIsCanUse[1]);
	//M20
	m_fM20Increment=g_sysPara.holeIncrement.m_fM20;
	if(fabs(m_fM20Increment-fDatum)>EPS)
		m_arrIsCanUse[2]=TRUE;
	((CEdit*)GetDlgItem(IDC_E_M20))->SetReadOnly(!m_arrIsCanUse[2]);
	//M24
	m_fM24Increment=g_sysPara.holeIncrement.m_fM24;
	if(fabs(m_fM24Increment-fDatum)>EPS)
		m_arrIsCanUse[3]=TRUE;
	((CEdit*)GetDlgItem(IDC_E_M24))->SetReadOnly(!m_arrIsCanUse[3]);
	//SH
	m_fCutIncrement=g_sysPara.holeIncrement.m_fCutSH;
	if(fabs(m_fCutIncrement-fDatum)>EPS)
		m_arrIsCanUse[4]=TRUE;
	((CEdit*)GetDlgItem(IDC_E_CUT_SH))->SetReadOnly(!m_arrIsCanUse[4]);
	//SH
	m_fProIncrement = g_sysPara.holeIncrement.m_fProSH;
	if (fabs(m_fProIncrement - fDatum) > EPS)
		m_arrIsCanUse[5] = TRUE;
	((CEdit*)GetDlgItem(IDC_E_PRO_SH))->SetReadOnly(!m_arrIsCanUse[5]);
	//
	UpdateData(FALSE);
	return TRUE;
}
void CHoleIncrementSetDlg::OnChkM12()
{
	m_arrIsCanUse[0]=!m_arrIsCanUse[0];
	((CEdit*)GetDlgItem(IDC_E_M12))->SetReadOnly(!m_arrIsCanUse[0]);
}
void CHoleIncrementSetDlg::OnChkM16()
{
	m_arrIsCanUse[1]=!m_arrIsCanUse[1];
	((CEdit*)GetDlgItem(IDC_E_M16))->SetReadOnly(!m_arrIsCanUse[1]);
}
void CHoleIncrementSetDlg::OnChkM20()
{
	m_arrIsCanUse[2]=!m_arrIsCanUse[2];
	((CEdit*)GetDlgItem(IDC_E_M20))->SetReadOnly(!m_arrIsCanUse[2]);
}
void CHoleIncrementSetDlg::OnChkM24()
{
	m_arrIsCanUse[3]=!m_arrIsCanUse[3];
	((CEdit*)GetDlgItem(IDC_E_M24))->SetReadOnly(!m_arrIsCanUse[3]);
}
void CHoleIncrementSetDlg::OnChkCutSH()
{
	m_arrIsCanUse[4]=!m_arrIsCanUse[4];
	((CEdit*)GetDlgItem(IDC_E_CUT_SH))->SetReadOnly(!m_arrIsCanUse[4]);
}
void CHoleIncrementSetDlg::OnChkProSH()
{
	m_arrIsCanUse[5] = !m_arrIsCanUse[5];
	((CEdit*)GetDlgItem(IDC_E_PRO_SH))->SetReadOnly(!m_arrIsCanUse[5]);
}