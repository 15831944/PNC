// PartListDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "PartListDlg.h"
#include "PNCModel.h"
#include "ArrayList.h"
#include "CadToolFunc.h"
#include "CadHighlightEntManager.h"
#include "PNCCmd.h"
#include "acdocman.h"
#include "DrawDamBoard.h"
#include "AdjustPlateMCS.h"
#include "AcUiDialogPanel.h"
#include "PNCSysPara.h"

#ifndef __UBOM_ONLY_
// CPartListDlg �Ի���
#ifdef __SUPPORT_DOCK_UI_
IMPLEMENT_DYNCREATE(CPartListDlg, CAcUiDialog)
CPartListDlg::CPartListDlg(CWnd* pParent /*=NULL*/)
	: CAcUiDialog(CPartListDlg::IDD, pParent)
	, m_bEditMK(FALSE)
{
#else
IMPLEMENT_DYNCREATE(CPartListDlg, CDialog)
CPartListDlg::CPartListDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPartListDlg::IDD, pParent)
	, m_bEditMK(FALSE)
{
#endif

}

CPartListDlg::~CPartListDlg()
{
}

void CPartListDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PART_LIST, m_partList);
	DDX_Check(pDX, IDC_CHK_EDIT_MK, m_bEditMK);
}


BEGIN_MESSAGE_MAP(CPartListDlg, CDialog)
	ON_WM_SIZE()
	ON_NOTIFY(NM_CLICK, IDC_PART_LIST, &CPartListDlg::OnNMClickPartList)
	ON_NOTIFY(LVN_KEYDOWN, IDC_PART_LIST, &CPartListDlg::OnKeydownListPart)
	ON_BN_CLICKED(IDC_BTN_SEND_TO_PPE, &CPartListDlg::OnBnClickedBtnSendToPpe)
	ON_BN_CLICKED(IDC_BTN_EXPORT_DXF, &CPartListDlg::OnBnClickedBtnExportDxf)
	ON_BN_CLICKED(IDC_BTN_ANTICLOCKWISE_ROTATION, &CPartListDlg::OnBnClickedBtnAnticlockwiseRotation)
	ON_BN_CLICKED(IDC_BTN_CLOCKWISE_ROTATION, &CPartListDlg::OnBnClickedBtnClockwiseRotation)
	ON_BN_CLICKED(IDC_BTN_MIRROR, &CPartListDlg::OnBnClickedBtnMirror)
	ON_BN_CLICKED(IDC_BTN_MOVE, &CPartListDlg::OnBnClickedBtnMove)
	ON_BN_CLICKED(IDC_BTN_MOVE_MK_RECT, &CPartListDlg::OnBnClickedBtnMoveMkRect)
	ON_BN_CLICKED(IDC_CHK_EDIT_MK, &CPartListDlg::OnBnClickedChkEditMk)
END_MESSAGE_MAP()


// CPartListDlg ��Ϣ�������
void CPartListDlg::OnOK()
{
#ifndef __SUPPORT_DOCK_UI_
	return CDialog::OnOK();
#endif
}
void CPartListDlg::OnCancel()
{
#ifndef __SUPPORT_DOCK_UI_
	CDialog::OnCancel();
#endif
}

void CPartListDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);	
	RECT rectList;
	if (m_partList.GetSafeHwnd())
	{
		m_partList.GetWindowRect(&rectList);
		ScreenToClient(&rectList);
		m_partList.MoveWindow(0,rectList.top,cx,cy-rectList.top);
	}
}

BOOL CPartListDlg::CreateDlg()
{
	return CDialog::Create(CPartListDlg::IDD);
}

BOOL CPartListDlg::OnInitDialog()
{
#ifdef __SUPPORT_DOCK_UI_
	CAcUiDialog::OnInitDialog();
#else
	CDialog::OnInitDialog();
#endif
	m_partList.AddColumnHeader("����",80);
	m_partList.AddColumnHeader("���",45);
	m_partList.AddColumnHeader("����",45);
	m_partList.AddColumnHeader("����",45);
	m_partList.InitListCtrl();

	if(m_bmpLeftRotate.LoadBitmap(IDB_LEFT_ROTATE))
		((CButton*)GetDlgItem(IDC_BTN_ANTICLOCKWISE_ROTATION))->SetBitmap(m_bmpLeftRotate);
	if(m_bmpRightRotate.LoadBitmap(IDB_RIGHT_ROTATE))
		((CButton*)GetDlgItem(IDC_BTN_CLOCKWISE_ROTATION))->SetBitmap(m_bmpRightRotate);
	if(m_bmpMirror.LoadBitmap(IDB_MIRROR))
		((CButton*)GetDlgItem(IDC_BTN_MIRROR))->SetBitmap(m_bmpMirror);
	if(m_bmpMoveMk.LoadBitmapA(IDB_MOVE_MK))
		//((CButton*)GetDlgItem(IDC_CHK_EDIT_MK))->SetBitmap(m_bmpMoveMk);
		((CButton*)GetDlgItem(IDC_BTN_MOVE_MK_RECT))->SetBitmap(m_bmpMoveMk);
	if(m_bmpMove.LoadBitmap(IDB_MOVE))
		((CButton*)GetDlgItem(IDC_BTN_MOVE))->SetBitmap(m_bmpMove);
	
	return TRUE;
}

BOOL CPartListDlg::UpdatePartList()
{
	m_partList.DeleteAllItems();
	CStringArray str_arr;
	str_arr.SetSize(4);
	CSortedModel sortedModel(&model);
	for(CPlateProcessInfo *pPlate=sortedModel.EnumFirstPlate();pPlate;pPlate=sortedModel.EnumNextPlate())
	{
		str_arr[0]=pPlate->xPlate.GetPartNo();
		str_arr[1].Format("-%.f",pPlate->xPlate.m_fThick);
		str_arr[2]=CProcessPart::QuerySteelMatMark(pPlate->xPlate.cMaterial);
		str_arr[3].Format("%d",pPlate->xPlate.feature);
		int iItem=m_partList.InsertItemRecord(-1,str_arr);
		m_partList.SetItemData(iItem,(DWORD)pPlate);
		if (pPlate->xPlate.m_fThick <= 0)
			m_partList.SetItemTextColor(iItem, 1, RGB(255, 0, 0));
		else
			m_partList.SetItemTextColor(iItem, 1, RGB(0, 0, 0));
	}
	if(CDrawDamBoard::m_bDrawAllBamBoard&&g_pncSysPara.m_bAutoLayout == CPNCSysPara::LAYOUT_SEG)
	{
		m_xDamBoardManager.DrawAllDamBoard(&model);
		m_xDamBoardManager.DrawAllSteelSealRect(&model);
	}
	return TRUE;
}

void CPartListDlg::SelectPart(int iCurSel) 
{
	CPlateProcessInfo *pPlate=(CPlateProcessInfo*)m_partList.GetItemData(iCurSel);
	if(pPlate==NULL)
		return;
	//ѡ�иְ����ʵ��
	if (pPlate->vertexList.GetNodeNum()>3)
	{	//�ְ���ȡ�ɹ�����ʾ
		SCOPE_STRU scope = pPlate->GetCADEntScope(TRUE);
		f2dPoint leftBtm(scope.fMinX, scope.fMinY);
		f2dRect rect;
		rect.topLeft.Set(leftBtm.x, leftBtm.y + CDrawDamBoard::BOARD_HEIGHT);
		rect.bottomRight.Set(leftBtm.x + CDrawDamBoard::BOARD_HEIGHT, leftBtm.y);
		ZoomAcadView(rect, 100);
		//
		CLockDocumentLife lock;
		if (!CDrawDamBoard::m_bDrawAllBamBoard)
			m_xDamBoardManager.EraseAllDamBoard();
		m_xDamBoardManager.DrawDamBoard(pPlate);
	}
	else
	{	//�ְ���ȡʧ�ܣ���ʾ
		f3dPoint dimPt = pPlate->dim_pos;
		f3dPoint dimVecH = pPlate->dim_vec;
		f3dPoint dimVecV = dimVecH;
		RotateVectorAroundVector(dimVecV, 3.14159*0.5, GEPOINT(0, 0, 1));
		SCOPE_STRU scope;
		scope.VerifyVertex(GEPOINT(dimPt + dimVecV * 15 - dimVecH * 30));
		scope.VerifyVertex(GEPOINT(dimPt - dimVecV * 15 - dimVecH * 30));
		scope.VerifyVertex(GEPOINT(dimPt - dimVecV * 15 + dimVecH * 30));
		scope.VerifyVertex(GEPOINT(dimPt + dimVecV * 15 + dimVecH * 30));
		ZoomAcadView(scope, 20);
	}
	//�޸���ͼλ�ú���Ҫ��ʱ���½���,����acedSSGet()���ܲ��ܻ�ȡ��ȷ��ʵ�弯 wht 11-06-25
	actrTransactionManager->flushGraphics();
	acedUpdateDisplay();
}

void CPartListDlg::OnNMClickPartList(NMHDR *pNMHDR, LRESULT *pResult)
{
	POSITION pos = m_partList.GetFirstSelectedItemPosition();
	if(pos!=NULL)
	{
		int iCurSel = m_partList.GetNextSelectedItem(pos);
		if(iCurSel>=0)
			SelectPart(iCurSel);
	}
	*pResult = 0;
}

void CPartListDlg::OnKeydownListPart(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_KEYDOWN* pLVKeyDow = (LV_KEYDOWN*)pNMHDR;
	ProcessKeyDown(pLVKeyDow->wVKey);
	*pResult = 0;
}

void CPartListDlg::ProcessKeyDown(WORD wVKey)
{
	int iCurSel = -1;
	POSITION pos = m_partList.GetFirstSelectedItemPosition();
	if (pos != NULL)
		iCurSel = m_partList.GetNextSelectedItem(pos);
	if(iCurSel>=0)
	{
		if (wVKey == VK_UP)
		{
			if (iCurSel >= 1)
				iCurSel--;
			if (iCurSel >= 0)
				SelectPart(iCurSel);
		}
		else if (wVKey == VK_DOWN)
		{
			if (iCurSel < m_partList.GetItemCount())
				iCurSel++;
			if (iCurSel >= 0)
				SelectPart(iCurSel);
		}
	}
}

void CPartListDlg::OnBnClickedBtnSendToPpe()
{
#ifndef __UBOM_ONLY_
	SendPartEditor();
#endif
}

void CPartListDlg::OnBnClickedBtnExportDxf()
{
	CLockDocumentLife lock;
	CString sDxfFolderPath,sFilePath;
	GetCurWorkPath(sDxfFolderPath,TRUE,"DXF");
	for(CPlateProcessInfo *pPlate=model.EnumFirstPlate(FALSE);pPlate;pPlate=model.EnumNextPlate(FALSE))
	{
		ARRAY_LIST<AcDbObjectId> entIdList;
		for(ULONG *pId=pPlate->m_cloneEntIdList.GetFirst();pId;pId=pPlate->m_cloneEntIdList.GetNext())
			entIdList.append(MkCadObjId(*pId));
		sFilePath.Format("%s%s.dxf",sDxfFolderPath,(char*)pPlate->GetPartNo());
		SaveAsDxf(sFilePath,entIdList,true);
	}
	ShellExecute(NULL,"open",NULL,NULL,sDxfFolderPath,SW_SHOW);
}


void CPartListDlg::OnBnClickedBtnAnticlockwiseRotation()
{
	int iCurSel=m_partList.GetSelectedItem();
	if(iCurSel<0)
		return;
	CPlateProcessInfo *pPlateInfo=(CPlateProcessInfo*)m_partList.GetItemData(iCurSel);
	if(pPlateInfo==NULL)
		return;
	CPlateReactorLife reactorLife(pPlateInfo, FALSE);
	CAdjustPlateMCS mcs(pPlateInfo);
	mcs.AnticlockwiseRotation();
	
	CLockDocumentLife lockLife;
	m_xDamBoardManager.DrawSteelSealRect(pPlateInfo);
	//����PPI�ļ�
	CString file_path;
	GetCurWorkPath(file_path);
	pPlateInfo->CreatePPiFile(file_path);
	//ˢ�½���
	actrTransactionManager->flushGraphics();
	acedUpdateDisplay();
}


void CPartListDlg::OnBnClickedBtnClockwiseRotation()
{
	int iCurSel=m_partList.GetSelectedItem();
	if(iCurSel<0)
		return;
	CPlateProcessInfo *pPlateInfo=(CPlateProcessInfo*)m_partList.GetItemData(iCurSel);
	if(pPlateInfo==NULL)
		return;
	CPlateReactorLife reactorLife(pPlateInfo, FALSE);
	CAdjustPlateMCS mcs(pPlateInfo);
	mcs.ClockwiseRotation();

	CLockDocumentLife lockLife;
	m_xDamBoardManager.DrawSteelSealRect(pPlateInfo);
	//����PPI�ļ�
	CString file_path;
	GetCurWorkPath(file_path);
	pPlateInfo->CreatePPiFile(file_path);
	//ˢ�½���
	actrTransactionManager->flushGraphics();
	acedUpdateDisplay();
}

void CPartListDlg::OnBnClickedBtnMirror()
{
	int iCurSel=m_partList.GetSelectedItem();
	if(iCurSel<0)
		return;
	CPlateProcessInfo *pPlateInfo=(CPlateProcessInfo*)m_partList.GetItemData(iCurSel);
	if(pPlateInfo==NULL)
		return;
	CPlateReactorLife reactorLife(pPlateInfo,FALSE);
	CAdjustPlateMCS mcs(pPlateInfo);
	mcs.Mirror();

	CLockDocumentLife lockLife;
	m_xDamBoardManager.DrawSteelSealRect(pPlateInfo);
	//����PPI�ļ�
	CString file_path;
	GetCurWorkPath(file_path);
	pPlateInfo->CreatePPiFile(file_path);
	//ˢ�½���
	actrTransactionManager->flushGraphics();
	acedUpdateDisplay();
}


void CPartListDlg::OnBnClickedBtnMove()
{
	int iCurSel=m_partList.GetSelectedItem();
	if(iCurSel<0)
		return;
	CPlateProcessInfo *pPlateInfo=(CPlateProcessInfo*)m_partList.GetItemData(iCurSel);
	if(pPlateInfo==NULL)
		return;
	CLockDocumentLife lock;
	DRAGSET.ClearEntSet();
	ARRAY_LIST<AcDbObjectId> entIdList;
	for(ULONG *pId=pPlateInfo->m_cloneEntIdList.GetFirst();pId;pId=pPlateInfo->m_cloneEntIdList.GetNext())
	{
		DRAGSET.Add(MkCadObjId(*pId));
		entIdList.append(MkCadObjId(*pId));
	}
	f2dRect rect=GetCadEntRect(entIdList);
	f3dPoint basepnt(rect.topLeft.x,rect.bottomRight.y);
#ifdef AFX_TARG_ENU_ENGLISH
	CXhChar100 sPrompt("please enter the position\n");
#else
	CXhChar100 sPrompt("������һ����λ��\n");
#endif
	int nRetCode=DragEntSet(basepnt,sPrompt);
	if(nRetCode==RTNORM)
	{	//���¹������λ��
		m_xDamBoardManager.EraseDamBoard(pPlateInfo);
		m_xDamBoardManager.DrawDamBoard(pPlateInfo);
		for(ACAD_LINEID *pLineId=pPlateInfo->m_hashCloneEdgeEntIdByIndex.GetFirst();pLineId;pLineId=pPlateInfo->m_hashCloneEdgeEntIdByIndex.GetNext())
			pLineId->UpdatePos();
		//�����ֺ�λ��
		CLockDocumentLife lockLife;
		m_xDamBoardManager.DrawSteelSealRect(pPlateInfo);
		//���½���
		actrTransactionManager->flushGraphics();
		acedUpdateDisplay();
	}
}

void CPartListDlg::OnBnClickedBtnMoveMkRect()
{	//������ӡ��λ��
	int iCurSel = m_partList.GetSelectedItem();
	if (iCurSel < 0)
		return;
	CPlateProcessInfo *pPlateInfo = (CPlateProcessInfo*)m_partList.GetItemData(iCurSel);
	if (pPlateInfo == NULL)
		return;
	CLockDocumentLife lock;
	DRAGSET.ClearEntSet();
	AcDbObjectId pointId = MkCadObjId(pPlateInfo->m_xMkDimPoint.idCadEnt);
	DRAGSET.Add(pointId);
	AcDbObjectId entId=m_xDamBoardManager.GetSteelSealRectId(pPlateInfo);
	DRAGSET.Add(entId);
	AcDbEntity *pEnt = NULL;
	acdbOpenAcDbEntity(pEnt, pointId, AcDb::kForRead);
	if (pEnt == NULL)
		return;
	pEnt->close();
	AcDbPoint *pPoint = (AcDbPoint*)pEnt;
	f3dPoint basepnt(pPoint->position().x, pPoint->position().y);
#ifdef AFX_TARG_ENU_ENGLISH
	CXhChar100 sPrompt("please enter the position\n");
#else
	CXhChar100 sPrompt("������һ����λ��\n");
#endif
	int nRetCode = DragEntSet(basepnt, sPrompt);
	if (nRetCode == RTNORM)
	{	//�����ֺ�λ��
		m_xDamBoardManager.DrawSteelSealRect(pPlateInfo);
		//�����ֺ���λ��֮��ͬ������PPI�ļ��и�ӡ��λ��
		pPlateInfo->SyncSteelSealPos();
		//����PPI�ļ�
		CString file_path;
		GetCurWorkPath(file_path);
		pPlateInfo->CreatePPiFile(file_path);
		//���½���
		actrTransactionManager->flushGraphics();
		acedUpdateDisplay();
	}
}

/*
BOOL m_bEnabelWatch = FALSE;
void WatchEditMkShortCut(const MSG *pMsg)
{
	if (pMsg->message == WM_CHAR && (pMsg->wParam == VK_UP || pMsg->wParam == VK_DOWN||pMsg->wParam== VK_F2))
	{
		if (g_pPartListDockBar&&g_pPartListDockBar->GetDlgPtr())
		{
			CPartListDlg *pDlg = (CPartListDlg*)g_pPartListDockBar->GetDlgPtr();
			if (pDlg)
				pDlg->ProcessKeyDown(pMsg->wParam);
		}
	}
}

void SetupEditMkWatch()
{
	if (m_bEnabelWatch == TRUE) // already has the hook?
		return;
	if (acedRegisterWatchWinMsg(WatchEditMkShortCut))
		m_bEnabelWatch = TRUE;
	else
		m_bEnabelWatch = FALSE;
}

void UninstallEditMkWatch()
{
	if (m_bEnabelWatch == TRUE) {
		acedRemoveWatchWinMsg(WatchEditMkShortCut);
		m_bEnabelWatch = FALSE;
	}
}
*/
void CPartListDlg::OnBnClickedChkEditMk()
{
	/*m_bEditMK = !m_bEditMK;
	if (m_bEditMK)
		SetupEditMkWatch();
	else
		UninstallEditMkWatch();*/
}

void CPartListDlg::PreSubclassWindow()
{
#ifdef __SUPPORT_DOCK_UI_
	ModifyStyle(DS_SETFONT | DS_MODALFRAME | WS_POPUP | WS_CAPTION, DS_SETFONT | WS_CHILD);
	CAcUiDialog::PreSubclassWindow();
#else
	//ModifyStyle(WS_CHILD, DS_SETFONT | DS_MODALFRAME | WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU);
	CDialog::PreSubclassWindow();
#endif
}
#endif
