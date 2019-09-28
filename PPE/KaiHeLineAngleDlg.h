#pragma once


// CKaiHeLineAngleDlg �Ի���

class CKaiHeLineAngleDlg : public CDialog
{
	DECLARE_DYNAMIC(CKaiHeLineAngleDlg)

public:
	CKaiHeLineAngleDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CKaiHeLineAngleDlg();

// �Ի�������
	enum { IDD = IDD_KAIHE_JG_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	DECLARE_MESSAGE_MAP()
public:
	BOOL m_bHasKaiHeWing;	//�ж��Ƿ����п���֫
	float m_fKaiHeAngle;	//���ϽǶ�
	int m_iDimPos;			//��עλ�ã���ʼ�˾��룩
	int m_iLeftLen;			//��࿪�ϳ���
	int m_iRightLen;		//�Ҳ࿪�ϳ���
	int m_iKaiHeWing;		//����֫
	CString m_sKaiHeWing;
};
