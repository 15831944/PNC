// LineFeatDlg.cpp : implementation file
//

#include "stdafx.h"
#include "LineFeatDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLineFeatDlg dialog
BOOL ReadClipPoint(f3dPoint &point);
void WritePointToClip(f3dPoint point);

CLineFeatDlg::CLineFeatDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLineFeatDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLineFeatDlg)
	m_fEndX = 0.0;
	m_fEndY = 0.0;
	m_fEndZ = 0.0;
	m_fLength = 0.0;
	m_fStartX = 0.0;
	m_fStartY = 0.0;
	m_fStartZ = 0.0;
	m_sLineType = _T("��ͨ������");
	m_iEdgeType = 0;
	m_fDeformCoef = 0.35;
	m_fHuoQuR = 0.0;
	m_iSectorAngleStyle = 0;
	m_iRotateStyle = 0;
	m_fArcAngleOrR = 0.0;
	m_fCenterX = 0.0;
	m_fCenterY = 0.0;
	m_fCenterZ = 0.0;
	m_fColAxisX = 0.0;
	m_fColAxisY = 0.0;
	m_fColAxisZ = 0.0;
	m_bWeldEdge = FALSE;
	m_bRollEdge = FALSE;
	m_fLocalPointY = 0.0;
	m_iLocalPointVec = 0;	//0.�޶Ի�� 1.�Ի����50->100����ͬ��ʼ-->�շ��� 2.���ʼ-->�շ����෴
	m_nManuSpace = 0;
	//}}AFX_DATA_INIT
	line_type=0;
	m_fHuoQuAngle=0;
	m_fThick=0;
	m_pArcLine=NULL;			//�л�Բ����ʽʱ���뾶ת��Ϊ��Ӧ�ĽǶ�
	m_nRollEdgeOffsetDist=0;	//������ƶ���
}


void CLineFeatDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLineFeatDlg)
	DDX_Text(pDX, IDC_E_END_X, m_fEndX);
	DDX_Text(pDX, IDC_E_END_Y, m_fEndY);
	DDX_Text(pDX, IDC_E_END_Z, m_fEndZ);
	DDX_Text(pDX, IDC_E_LENGTH, m_fLength);
	DDX_Text(pDX, IDC_E_START_X, m_fStartX);
	DDX_Text(pDX, IDC_E_START_Y, m_fStartY);
	DDX_Text(pDX, IDC_E_START_Z, m_fStartZ);
	DDX_Text(pDX, IDC_S_LINE_TYPE, m_sLineType);
	DDX_CBIndex(pDX, IDC_CMB_EDGE_TYPE, m_iEdgeType);
	DDX_Text(pDX, IDC_E_DEFORM_COEF, m_fDeformCoef);
	DDV_MinMaxDouble(pDX, m_fDeformCoef, 0., 0.5);
	DDX_Text(pDX, IDC_E_HUOQU_R, m_fHuoQuR);
	DDX_Radio(pDX, IDC_RDO_SECTOR_ANGLE_STYLE, m_iSectorAngleStyle);
	DDX_Radio(pDX, IDC_RDO_ROTATE_STYLE, m_iRotateStyle);
	DDX_Text(pDX, IDC_E_ARC_ANGLE_OR_R, m_fArcAngleOrR);
	DDX_Text(pDX, IDC_E_CENTER_X, m_fCenterX);
	DDX_Text(pDX, IDC_E_CENTER_Y, m_fCenterY);
	DDX_Text(pDX, IDC_E_CENTER_Z, m_fCenterZ);
	DDX_Text(pDX, IDC_E_COL_AXIS_X, m_fColAxisX);
	DDX_Text(pDX, IDC_E_COL_AXIS_Y, m_fColAxisY);
	DDX_Text(pDX, IDC_E_COL_AXIS_Z, m_fColAxisZ);
	DDX_Check(pDX, IDC_CHK_WELD_EDGE, m_bWeldEdge);
	DDX_Check(pDX, IDC_CHK_ROLL_EDGE, m_bRollEdge);
	DDX_Text(pDX, IDC_E_LOCAL_POINT_Y, m_fLocalPointY);
	DDX_CBIndex(pDX, IDC_CMB_LOCAL_POINT_VEC, m_iLocalPointVec);
	DDX_Text(pDX, IDC_E_MANU_SPACE, m_nManuSpace);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLineFeatDlg, CDialog)
	//{{AFX_MSG_MAP(CLineFeatDlg)
	ON_EN_CHANGE(IDC_E_HUOQU_R, OnChangeEDeformPara)
	ON_CBN_SELCHANGE(IDC_CMB_EDGE_TYPE, OnSelchangeCmbEdgeType)
	ON_BN_CLICKED(IDC_RDO_SECTOR_ANGLE_STYLE, OnRdoSectorAngleStyle)
	ON_BN_CLICKED(IDC_BN_PASTE_CENTER, OnBnPasteCenter)
	ON_BN_CLICKED(IDC_BN_PASTE_COL_AXIS, OnBnPasteColAxis)
	ON_BN_CLICKED(IDC_CHK_WELD_EDGE, OnChkWeldEdge)
	ON_BN_CLICKED(IDC_CHK_ROLL_EDGE, OnChkRollEdge)
	ON_CBN_SELCHANGE(IDC_CMB_LOCAL_POINT_VEC, OnSelchangeLocalPointVec)
	ON_BN_CLICKED(IDC_RADIO14, OnRdoSectorAngleStyle)
	ON_BN_CLICKED(IDC_RADIO15, OnRdoSectorAngleStyle)
	ON_EN_CHANGE(IDC_E_DEFORM_COEF, OnChangeEDeformPara)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLineFeatDlg message handlers

BOOL CLineFeatDlg::OnInitDialog() 
{
	if(line_type==0)	//��ͨ������
	{
		m_sLineType="��ͨ������";
		/*GetDlgItem(IDC_S_HUOQU)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_E_HUOQU_R)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_E_DEFORM_COEF)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_S_DEFORMED_LENGTH)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_S_MIDDLE_LAYER)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_CMB_EDGE_TYPE)->EnableWindow(TRUE);
		GetDlgItem(IDC_E_ARC_ANGLE_OR_R)->EnableWindow(TRUE);*/
	}
	else if(line_type==2)	//������
	{
		m_sLineType="������";
		/*GetDlgItem(IDC_S_HUOQU)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_E_HUOQU_R)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_E_DEFORM_COEF)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_S_DEFORMED_LENGTH)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_CMB_EDGE_TYPE)->EnableWindow(FALSE);
		GetDlgItem(IDC_E_ARC_ANGLE_OR_R)->EnableWindow(FALSE);*/
	}
	else
	{
		m_sLineType="��ͨ������";
		/*GetDlgItem(IDC_S_HUOQU)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_E_HUOQU_R)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_E_DEFORM_COEF)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_S_DEFORMED_LENGTH)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_S_MIDDLE_LAYER)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_CMB_EDGE_TYPE)->EnableWindow(TRUE);
		GetDlgItem(IDC_E_ARC_ANGLE_OR_R)->EnableWindow(TRUE);*/
	}
	GetDlgItem(IDC_CMB_LOCAL_POINT_VEC)->EnableWindow(m_bWeldEdge);
	CDialog::OnInitDialog();
	OnChangeEDeformPara();
	OnSelchangeCmbEdgeType();
	OnRdoSectorAngleStyle();
	OnSelchangeLocalPointVec();
	if(m_bRollEdge)
	{
		GetDlgItem(IDC_S_PARAM_TITLE)->SetWindowText("��߸߶�:");
		GetDlgItem(IDC_S_POS_Y)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_E_LOCAL_POINT_Y)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_S_POS_Y)->SetWindowText(" ���������:");
		m_fLocalPointY=m_nRollEdgeOffsetDist;	//��ʼ����������� wht 11-04-02
		UpdateData(FALSE);
	}
	return TRUE;
}

void CLineFeatDlg::OnChangeEDeformPara() 
{
	UpdateData();
	double r=m_fHuoQuR;
	double x=m_fDeformCoef;
	double odd_len;	//����������
	if(m_fHuoQuAngle>0)	//����,�����ǻ�׼��
		odd_len=m_fHuoQuAngle*(r+x*m_fThick)-2*r*tan(m_fHuoQuAngle/2);
	else	//����,�����ǻ�׼��
		odd_len=-m_fHuoQuAngle*(r+x*m_fThick)-2*(r+m_fThick)*tan(-m_fHuoQuAngle/2);
	CString ss;
	if(odd_len>0)
		ss.Format("����%.1f�����,�쳤%.1fmm",fabs(m_fHuoQuAngle*DEGTORAD_COEF),odd_len);
	else
		ss.Format("����%.1f�����,����%.1fmm",fabs(m_fHuoQuAngle*DEGTORAD_COEF),-odd_len);
	GetDlgItem(IDC_S_DEFORMED_LENGTH)->SetWindowText(ss);
}

void CLineFeatDlg::OnSelchangeCmbEdgeType() 
{
	UpdateData();
	if(m_iEdgeType==0)
	{	//ֱ��
		GetDlgItem(IDC_E_HUOQU_R)->EnableWindow(line_type==2);
		GetDlgItem(IDC_CHK_ROLL_EDGE)->EnableWindow(line_type!=2);
		GetDlgItem(IDC_E_DEFORM_COEF)->EnableWindow(line_type==2);
		GetDlgItem(IDC_RDO_SECTOR_ANGLE_STYLE)->EnableWindow(FALSE);
		GetDlgItem(IDC_RADIO14)->EnableWindow(FALSE);
		GetDlgItem(IDC_RADIO15)->EnableWindow(FALSE);
		GetDlgItem(IDC_RDO_ROTATE_STYLE)->EnableWindow(FALSE);
		GetDlgItem(IDC_RADIO3)->EnableWindow(FALSE);
		GetDlgItem(IDC_E_ARC_ANGLE_OR_R)->EnableWindow(FALSE);
	}
	else
	{	//Բ��
		GetDlgItem(IDC_E_HUOQU_R)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHK_ROLL_EDGE)->EnableWindow(FALSE);
		((CButton*)GetDlgItem(IDC_CHK_ROLL_EDGE))->SetCheck(FALSE);
		m_bRollEdge=FALSE;
		GetDlgItem(IDC_E_DEFORM_COEF)->EnableWindow(FALSE);
		GetDlgItem(IDC_RDO_SECTOR_ANGLE_STYLE)->EnableWindow(TRUE);
		GetDlgItem(IDC_RADIO14)->EnableWindow(TRUE);
		GetDlgItem(IDC_RADIO15)->EnableWindow(TRUE);
		GetDlgItem(IDC_RDO_ROTATE_STYLE)->EnableWindow(TRUE);
		GetDlgItem(IDC_RADIO3)->EnableWindow(TRUE);
		GetDlgItem(IDC_E_ARC_ANGLE_OR_R)->EnableWindow(m_iSectorAngleStyle==0);
	}
	
}

void CLineFeatDlg::OnRdoSectorAngleStyle() 
{
	int iOldSectorAngleStyle=m_iSectorAngleStyle;
	UpdateData();
	GetDlgItem(IDC_E_ARC_ANGLE_OR_R)->EnableWindow(m_iEdgeType!=0);
	if(m_iSectorAngleStyle==0)
	{
		GetDlgItem(IDC_S_ANGLE_OR_R)->SetWindowText("���ν�:");
		GetDlgItem(IDC_S_ARC_PARA_UNIT)->SetWindowText("��");
		if(m_pArcLine&&iOldSectorAngleStyle==1)
			m_fArcAngleOrR=m_pArcLine->SectorAngle()*DEGTORAD_COEF;
	}
	else if(m_iSectorAngleStyle==1)
	{
		GetDlgItem(IDC_S_ANGLE_OR_R)->SetWindowText("Բ��R:");
		GetDlgItem(IDC_S_ARC_PARA_UNIT)->SetWindowText("mm");
		if(m_pArcLine&&iOldSectorAngleStyle==0)
			m_fArcAngleOrR=m_pArcLine->Radius();
	}
	else if(m_iSectorAngleStyle==2)	//��Բ
	{
		GetDlgItem(IDC_S_ANGLE_OR_R)->SetWindowText("Բ��R:");
		GetDlgItem(IDC_S_ARC_PARA_UNIT)->SetWindowText("mm");
	}
	UpdateData(FALSE);
}

void CLineFeatDlg::OnBnPasteCenter() 
{
	UpdateData();
	f3dPoint point;
	ReadClipPoint(point);
	m_fCenterX = point.x;
	m_fCenterY = point.y;
	m_fCenterZ = point.z;
	UpdateData(FALSE);
}

void CLineFeatDlg::OnBnPasteColAxis() 
{
	UpdateData();
	f3dPoint point;
	ReadClipPoint(point);
	m_fColAxisX = point.x;
	m_fColAxisY = point.y;
	m_fColAxisZ = point.z;
	UpdateData(FALSE);
}

void CLineFeatDlg::OnChkWeldEdge() 
{
	UpdateData();
	if(m_bWeldEdge)
	{
		m_bRollEdge=FALSE;
		GetDlgItem(IDC_S_PARAM_TITLE)->SetWindowText("�ӹ���϶:");
		if(m_iLocalPointVec>0)
		{
			GetDlgItem(IDC_S_POS_Y)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_E_LOCAL_POINT_Y)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_S_POS_Y)->SetWindowText("�Ի��Y����:");
		}
		else
		{
			GetDlgItem(IDC_S_POS_Y)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_E_LOCAL_POINT_Y)->ShowWindow(SW_HIDE);
		}
	}
	else if(m_bRollEdge)
	{
		GetDlgItem(IDC_S_POS_Y)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_E_LOCAL_POINT_Y)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_S_POS_Y)->SetWindowText(" ���������:");
	}
	else 
	{
		GetDlgItem(IDC_S_POS_Y)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_E_LOCAL_POINT_Y)->ShowWindow(SW_HIDE);
	}
	GetDlgItem(IDC_CMB_LOCAL_POINT_VEC)->EnableWindow(m_bWeldEdge);
	UpdateData(FALSE);
}

void CLineFeatDlg::OnChkRollEdge() 
{
	UpdateData();
	if(m_bRollEdge)
	{
		m_bWeldEdge=FALSE;
		GetDlgItem(IDC_S_PARAM_TITLE)->SetWindowText("��߸߶�:");
		GetDlgItem(IDC_S_POS_Y)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_E_LOCAL_POINT_Y)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_S_POS_Y)->SetWindowText(" ���������:");
	}
	if(m_bWeldEdge&&m_iLocalPointVec>0)
	{
		GetDlgItem(IDC_S_POS_Y)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_E_LOCAL_POINT_Y)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_S_POS_Y)->SetWindowText("�Ի��Y����:");
	}
	else if(!m_bRollEdge)
	{
		GetDlgItem(IDC_S_POS_Y)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_E_LOCAL_POINT_Y)->ShowWindow(SW_HIDE);
	}
	GetDlgItem(IDC_CMB_LOCAL_POINT_VEC)->EnableWindow(m_bWeldEdge);
	UpdateData(FALSE);
}

void CLineFeatDlg::OnSelchangeLocalPointVec() 
{
	UpdateData();
	if(m_iLocalPointVec>0)
	{
		GetDlgItem(IDC_S_POS_Y)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_E_LOCAL_POINT_Y)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_S_POS_Y)->SetWindowText("�Ի��Y����:");
	}
	else 
	{
		GetDlgItem(IDC_S_POS_Y)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_E_LOCAL_POINT_Y)->ShowWindow(SW_HIDE);
	}
	UpdateData(FALSE);
}

void CLineFeatDlg::OnOK() 
{
	UpdateData();
	//�ж����������Ƿ�Ϸ�
	if(m_iEdgeType==1&&m_iSectorAngleStyle==1)	//ָ���뾶��Բ��
	{
		f3dPoint startPt(m_fStartX,m_fStartY,m_fStartZ);
		f3dPoint endPt(m_fEndX,m_fEndY,m_fEndZ);
		double len=DISTANCE(startPt,endPt);
		if((2*m_fArcAngleOrR-len+0.000001)<0)	//+0.000001����뾶=1/2�ҳ������
		{	//�뾶Ӧ����1/2�ҳ�
			AfxMessageBox("ָ��Բ���뾶��С�������ɺϷ�Բ����������ָ��Բ���뾶��");
			return;
		}
	}
	if(m_bRollEdge)
		m_nRollEdgeOffsetDist=(short)m_fLocalPointY;	//������������ wht 11-04-02
	CDialog::OnOK();
}
