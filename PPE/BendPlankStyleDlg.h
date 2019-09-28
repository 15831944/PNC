#pragma once


// CBendPlankStyleDlg �Ի���

class CBendPlankStyleDlg : public CDialog
{
	DECLARE_DYNAMIC(CBendPlankStyleDlg)

public:
	CBendPlankStyleDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CBendPlankStyleDlg();

// �Ի�������
	enum { IDD = IDD_BEND_PLANK_STYLE_DLG };
	double	m_fBendAngle;
	double	m_fNormX;
	double	m_fNormY;
	double	m_fNormZ;
	int		m_iBendFaceStyle;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	afx_msg void OnBnPasteNorm();
	virtual BOOL OnInitDialog();
	afx_msg void OnRdoBendFaceStyle();
	DECLARE_MESSAGE_MAP()
};
