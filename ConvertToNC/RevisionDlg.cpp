// TowerInstanceDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "RevisionDlg.h"
#include "ProcessPart.h"
#include "CadToolFunc.h"
#include "SortFunc.h"
#include "image.h"
#include "folder_dialog.h"
#include "InputAnValDlg.h"
#include "MsgBox.h"
#include "ComparePartNoString.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#if defined(__UBOM_) || defined(__UBOM_ONLY_)
CRevisionDlg *g_pRevisionDlg;
IMPLEMENT_DYNAMIC(CRevisionDlg, CDialog)
//
static BOOL FireItemChanged(CSuperGridCtrl* pListCtrl,CSuperGridCtrl::CTreeItem* pItem,NM_LISTVIEW* pNMListView)
{	//ѡ������仯�����������
	if(pItem->m_idProp==NULL)
		return FALSE;
	CRevisionDlg *pRevisionDlg=(CRevisionDlg*)pListCtrl->GetParent();
	CXhTreeCtrl* pTreeCtrl=pRevisionDlg->GetTreeCtrl();
	HTREEITEM hSelectedItem=pTreeCtrl->GetSelectedItem();
	if(hSelectedItem==NULL)
		return FALSE;
	TREEITEM_INFO* pItemInfo=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hSelectedItem);
	SCOPE_STRU scope;
	BOOL bRetCode = FALSE;
	if (pItemInfo->itemType == BOM_ITEM)
	{
		BOMPART *pPart = (BOMPART*)pItem->m_idProp;
		CBomFile* pBom = (CBomFile*)pItemInfo->dwRefData;
		CProjectTowerType *pPrjTowerType = pBom?pBom->BelongModel():NULL;
		if (pPart&&pBom&&pPrjTowerType)
		{
			if(pPart->cPartType==BOMPART::PLATE)
			{
				CPlateProcessInfo *pPlateInfo=pPrjTowerType->FindPlateInfoByPartNo(pPart->sPartNo);
				if (pPlateInfo)
				{
					scope = pPlateInfo->GetCADEntScope();
					bRetCode = TRUE;
				}
			}
			else if(pPart->cPartType==BOMPART::ANGLE)
			{
				CAngleProcessInfo *pAngleInfo = pPrjTowerType->FindAngleInfoByPartNo(pPart->sPartNo);
				if (pAngleInfo)
				{
					scope = pAngleInfo->GetCADEntScope();
					bRetCode = TRUE;
				}
			}
		}
	}
	else if(pItemInfo->itemType==ANGLE_DWG_ITEM)
	{
		CAngleProcessInfo* pJgInfo=(CAngleProcessInfo*)pItem->m_idProp;
		if (pJgInfo)
		{
			scope = pJgInfo->GetCADEntScope();
			bRetCode = TRUE;
		}
	}
	else if(pItemInfo->itemType==PLATE_DWG_ITEM)
	{
		CPlateProcessInfo* pPlateInfo=(CPlateProcessInfo*)pItem->m_idProp;
		if (pPlateInfo)
		{
			scope = pPlateInfo->GetCADEntScope();
			bRetCode = TRUE;
		}
	}
	if (bRetCode)	//�����������Ӧ��ʵ��
		ZoomAcadView(scope, 20);
	return bRetCode;
}
static int FireCompareItem(const CSuperGridCtrl::CSuperGridCtrlItemPtr& pItem1,const CSuperGridCtrl::CSuperGridCtrlItemPtr& pItem2,DWORD lPara)
{
	COMPARE_FUNC_EXPARA* pExPara=(COMPARE_FUNC_EXPARA*)lPara;
	int iSubItem=0;
	BOOL bAscending=true;
	if(pExPara)
	{
		iSubItem=pExPara->iSubItem;
		bAscending=pExPara->bAscending;
	}
	CString sText1=pItem1->m_lpNodeInfo->GetSubItemText(iSubItem);
	CString sText2=pItem2->m_lpNodeInfo->GetSubItemText(iSubItem);
	int result = 0;
	if (iSubItem == 0)
		result = ComparePartNoString(sText1, sText2);
	else if (iSubItem == 1)
		result = CompareMultiSectionString(sText1, sText2);
	else
		result = sText1.Compare(sText2);
	if (!bAscending)
		result *= -1;
	return result;
}
static BOOL FireContextMenu(CSuperGridCtrl* pListCtrl,CSuperGridCtrl::CTreeItem* pSelItem,CPoint point)
{
	CRevisionDlg *pRevisionDlg=(CRevisionDlg*)pListCtrl->GetParent();
	if(pRevisionDlg==NULL)
		return FALSE;
	CXhTreeCtrl *pTreeCtrl=pRevisionDlg->GetTreeCtrl();
	HTREEITEM hSelItem=pTreeCtrl->GetSelectedItem();
	TREEITEM_INFO *pItemInfo=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hSelItem);
	if(pItemInfo==NULL || pItemInfo->itemType!=PLATE_DWG_ITEM)
		return FALSE;
	CMenu popMenu;
	popMenu.LoadMenu(IDR_ITEM_CMD_POPUP);
	CMenu *pMenu=popMenu.GetSubMenu(0);
	pMenu->DeleteMenu(0,MF_BYPOSITION);
	pMenu->AppendMenu(MF_STRING,ID_REVISE_TEH_PLATE,"�޶��ض��ְ�");
	CPoint menu_pos=point;
	pListCtrl->ClientToScreen(&menu_pos);
	popMenu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON,menu_pos.x,menu_pos.y,pRevisionDlg);
	return TRUE;
}
// CRevisionDlg �Ի���
CRevisionDlg::CRevisionDlg(CWnd* pParent /*=NULL*/)
		: CDialog(CRevisionDlg::IDD, pParent)
		, m_sCurFile(_T(""))
		, m_sRecordNum(_T(""))
{
	m_bEnableWindowMoveListen=false;
	m_nScrLocationX=0;
	m_nScrLocationY=0;
	m_nRightMargin=0;
	m_nBtmMargin=0;
	m_iCompareMode=0;
	DisplayProcess = NULL;
}

CRevisionDlg::~CRevisionDlg()
{
}

void CRevisionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_REPORT, m_xListReport);
	DDX_Control(pDX, IDC_TREE_CONTRL, m_treeCtrl);
	DDX_Text(pDX, IDC_E_FILE_NAME, m_sCurFile);
	DDX_Text(pDX, IDC_E_NUM, m_sRecordNum);
}


BEGIN_MESSAGE_MAP(CRevisionDlg, CDialog)
	ON_WM_SIZE()
	ON_WM_MOVE()
	ON_WM_CLOSE()
	ON_NOTIFY(NM_RCLICK, IDC_TREE_CONTRL, &OnNMRClickTreeCtrl)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_CONTRL, &OnTvnSelchangedTreeContrl)
	ON_COMMAND(ID_NEW_ITEM,OnNewItem)
	ON_COMMAND(ID_LOAD_PROJECT,OnLoadProjFile)
	ON_COMMAND(ID_EXPORT_PROJECT,OnExportProjFile)
	ON_COMMAND(ID_PROJECT_PROPERTY,OnProjectProperty)
	ON_COMMAND(ID_IMPORT_BOM_FILE,OnImportBomFile)
	ON_COMMAND(ID_IMPORT_ANGLE_DWG,OnImportAngleDwg)
	ON_COMMAND(ID_IMPORT_PLATE_DWG,OnImportPlateDwg)
	ON_COMMAND(ID_COMPARE_DATA,OnCompareData)
	ON_COMMAND(ID_EXPORT_COMPARE_RESULT,OnExportCompResult)
	ON_COMMAND(ID_REFRESH_PART_NUM,OnRefreshPartNum)
	ON_COMMAND(ID_MODIFY_ERP_FILE,OnModifyErpFile)
	ON_COMMAND(ID_MODIFY_TMA_FILE, OnModifyTmaFile)
	ON_COMMAND(ID_RETRIEVED_ANGLES, OnRetrievedAngles)
	ON_COMMAND(ID_RETRIEVED_PLATES, OnRetrievedPlates)
	ON_COMMAND(ID_DELETE_ITEM,OnDeleteItem)
END_MESSAGE_MAP()

// CRevisionDlg ��Ϣ�������
BOOL CRevisionDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	//��ʼ���б��
	m_xListReport.EmptyColumnHeader();
	m_xListReport.AddColumnHeader("����", 75);
	m_xListReport.AddColumnHeader("���", 70);
	m_xListReport.AddColumnHeader("����", 50);
	m_xListReport.AddColumnHeader("����", 50);
	if (g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_AnHui_HongYuan||
		g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_AnHui_TingYang)
	{
		m_xListReport.AddColumnHeader("������", 52);
		m_xListReport.AddColumnHeader("�ӹ���", 52);
		m_xListReport.AddColumnHeader("�ӹ�����", 65);
		m_xListReport.AddColumnHeader("��ע", 150);
	}
	if(g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_JiangSu_HuaDian)
		m_xListReport.AddColumnHeader("������", 52);
	if (g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_SiChuan_ChengDu)
	{
		m_xListReport.AddColumnHeader("�ӹ���", 52);
		m_xListReport.AddColumnHeader("����", 44);
		m_xListReport.AddColumnHeader("����", 44);
		m_xListReport.AddColumnHeader("�н�", 44);
		m_xListReport.AddColumnHeader("���", 44);
		m_xListReport.AddColumnHeader("����", 44);
		m_xListReport.AddColumnHeader("�ٽ�", 44);
		m_xListReport.AddColumnHeader("����", 44);
		m_xListReport.AddColumnHeader("�Ͻ�", 44);
		m_xListReport.AddColumnHeader("���Ŷ�", 50);
		m_xListReport.AddColumnHeader("��ע", 150);
	}
	m_xListReport.EnableSortItems(true,true);
	m_xListReport.SetGridLineColor(RGB(220, 220, 220));
	m_xListReport.SetEvenRowBackColor(RGB(224, 237, 236));
	m_xListReport.InitListCtrl();
	m_xListReport.SetCompareItemFunc(FireCompareItem);
	m_xListReport.SetItemChangedFunc(FireItemChanged);
	//m_xListReport.SetContextMenuFunc(FireContextMenu);
	RefreshListCtrl(NULL);
	//��ʼ�����б�
	m_imageList.Create(IDB_IL_PROJECT, 16, 1, RGB(0,255,0));
	m_treeCtrl.SetImageList(&m_imageList,TVSIL_NORMAL);
	m_treeCtrl.ModifyStyle(0,TVS_HASLINES|TVS_HASBUTTONS|TVS_LINESATROOT|TVS_SHOWSELALWAYS|TVS_FULLROWSELECT);
	RefreshTreeCtrl();
	//�ƶ����ڵ�����λ��
	CRect xRect;
	CWnd::GetWindowRect(xRect);
	int width = xRect.Width();
	int height=xRect.Height();
	xRect.left=m_nScrLocationX;
	xRect.top=m_nScrLocationY;
	xRect.right = xRect.left+width;
	xRect.bottom = xRect.top+height;
	MoveWindow(xRect,TRUE);

	RECT rect,clientRect;
	GetClientRect(&clientRect);
	GetDlgItem(IDC_LIST_REPORT)->GetWindowRect(&rect);
	ScreenToClient(&rect);
	m_nRightMargin=clientRect.right-rect.right;
	m_nBtmMargin=clientRect.bottom-rect.bottom;
	m_bEnableWindowMoveListen=true;
	UpdateData(FALSE);
	return TRUE;
}
void CRevisionDlg::OnOK()
{
}
void CRevisionDlg::OnCancel()
{	
}
void CRevisionDlg::OnClose()
{
	DestroyWindow();
}
BOOL CRevisionDlg::Create()
{
	return CDialog::Create(CRevisionDlg::IDD);
}
void CRevisionDlg::InitRevisionDlg()
{
	m_bEnableWindowMoveListen=false;
	if(GetSafeHwnd()==0)
		Create();
	else
		OnInitDialog();
	UpdateData(FALSE);
}
HTREEITEM CRevisionDlg::FindTreeItem(HTREEITEM hParentItem,CXhChar100 sName)
{
	if(hParentItem==NULL)
		return NULL;
	CXhTreeCtrl* pTreeCtrl=GetTreeCtrl();
	HTREEITEM hItem;
	hItem=pTreeCtrl->GetChildItem(hParentItem);
	while(hItem)
	{
		CString sItemName=pTreeCtrl->GetItemText(hItem);
		if(stricmp(sItemName,sName)==0)
			break;
		hItem=pTreeCtrl->GetNextItem(hItem,TVGN_NEXT);
	}
	return hItem;
}
CProjectTowerType* CRevisionDlg::GetProject(HTREEITEM hItem)
{
	if(hItem==NULL)
		return NULL;
	CProjectTowerType* pProject=NULL;
	CXhTreeCtrl* pTreeCtrl=GetTreeCtrl();
	TREEITEM_INFO *pItemInfo=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hItem);
	if(pItemInfo->itemType==PROJECT_ITEM)
		pProject=(CProjectTowerType*)pItemInfo->dwRefData;
	else if(pItemInfo->itemType==BOM_GROUP || pItemInfo->itemType==ANGLE_GROUP ||
		pItemInfo->itemType==PLATE_GROUP)
	{
		HTREEITEM hParentItem=pTreeCtrl->GetParentItem(hItem);
		pItemInfo=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hParentItem);
		if(pItemInfo&&pItemInfo->itemType==PROJECT_ITEM)
			pProject=(CProjectTowerType*)pItemInfo->dwRefData;
	}
	else if(pItemInfo->itemType==BOM_ITEM)
	{
		CBomFile* pBom=(CBomFile*)pItemInfo->dwRefData;
		pProject=pBom->BelongModel();
	}
	else if(pItemInfo->itemType==ANGLE_DWG_ITEM || pItemInfo->itemType==PLATE_DWG_ITEM)
	{
		CDwgFileInfo* pDwg=(CDwgFileInfo*)pItemInfo->dwRefData;
		pProject=pDwg->BelongModel();
	}
	return pProject;
}
static CSuperGridCtrl::CTreeItem *InsertPartToList(CSuperGridCtrl &list,CSuperGridCtrl::CTreeItem *pParentItem,
	BOMPART *pPart,CHashStrList<BOOL> *pHashBoolByPropName=NULL)
{
	COLORREF clr=RGB(230,100,230);
	CListCtrlItemInfo *lpInfo=new CListCtrlItemInfo();
	//����
	lpInfo->SetSubItemText(0,pPart->sPartNo,TRUE);
	//���
	lpInfo->SetSubItemText(1,pPart->sSpec,TRUE);
	if(pHashBoolByPropName&&pHashBoolByPropName->GetValue("spec"))
		lpInfo->SetSubItemColor(1,clr);
	//����
	lpInfo->SetSubItemText(2,CBomModel::QueryMatMarkIncQuality(pPart),TRUE);
	if(pHashBoolByPropName&&pHashBoolByPropName->GetValue("cMaterial"))
		lpInfo->SetSubItemColor(2,clr);
	//����
	lpInfo->SetSubItemText(3,CXhChar50("%.0f",pPart->length),TRUE);
	if(pHashBoolByPropName&&pHashBoolByPropName->GetValue("fLength"))
		lpInfo->SetSubItemColor(3,clr);
	if (g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_AnHui_HongYuan||
		g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_AnHui_TingYang)
	{	//���պ�ԴBOM��ʾ��Ϣ
		//������
		lpInfo->SetSubItemText(4, CXhChar50("%d", pPart->GetPartNum()), TRUE);
		if (pHashBoolByPropName&&pHashBoolByPropName->GetValue("nSingNum"))
			lpInfo->SetSubItemColor(4, clr);
		//�ӹ���
		lpInfo->SetSubItemText(5, CXhChar50("%d", pPart->feature1), TRUE);
		if (pHashBoolByPropName&&pHashBoolByPropName->GetValue("nManuNum"))
			lpInfo->SetSubItemColor(5, clr);
		//��ע
		lpInfo->SetSubItemText(6, pPart->sNotes, TRUE);
		if (pHashBoolByPropName&&pHashBoolByPropName->GetValue("sNotes"))
			lpInfo->SetSubItemColor(6, clr);
		//�ӹ�����
		CXhChar50 sValue("%.f", pPart->fSumWeight);
		SimplifiedNumString(sValue);
		lpInfo->SetSubItemText(7, sValue, TRUE);
		if (pHashBoolByPropName&&pHashBoolByPropName->GetValue("fSumWeight"))
			lpInfo->SetSubItemColor(7, clr);
	}
	if (g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_JiangSu_HuaDian)
	{
		lpInfo->SetSubItemText(4, CXhChar50("%d", pPart->GetPartNum()), TRUE);
		if (pHashBoolByPropName&&pHashBoolByPropName->GetValue("nSingNum"))
			lpInfo->SetSubItemColor(4, clr);
	}
	if (g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_SiChuan_ChengDu)
	{	//�ɶ�����BOM��ʾ��Ϣ
		//�ӹ���
		lpInfo->SetSubItemText(4, CXhChar50("%d", pPart->GetPartNum()), TRUE);
		if (pHashBoolByPropName&&pHashBoolByPropName->GetValue("nSingNum"))
			lpInfo->SetSubItemColor(4, clr);
		CXhChar50 sWeld, sZhiWan, sCutAngle, sPushFlat, sCutRoot;
		CXhChar50 sCutBer, sKaiJiao, sHeJiao, sHasFootNail;
		//����
		if (pPart->bWeldPart)
			sWeld.Copy("*");
		lpInfo->SetSubItemText(5, sWeld, TRUE);
		if (pHashBoolByPropName&&pHashBoolByPropName->GetValue("Weld"))
			lpInfo->SetSubItemColor(5, clr);
		//����
		if (pPart->siZhiWan > 0)
			sZhiWan.Copy("*");
		lpInfo->SetSubItemText(6, sZhiWan, TRUE);
		if (pHashBoolByPropName&&pHashBoolByPropName->GetValue("ZhiWan"))
			lpInfo->SetSubItemColor(6, clr);
		PART_ANGLE* pBomJg = (pPart->cPartType == BOMPART::ANGLE) ? (PART_ANGLE*)pPart : NULL;
		//�н�
		if (pBomJg && pBomJg->bCutAngle)
			sCutAngle.Copy("*");
		lpInfo->SetSubItemText(7, sCutAngle, TRUE);
		if (pHashBoolByPropName&&pHashBoolByPropName->GetValue("CutAngle"))
			lpInfo->SetSubItemColor(7, clr);
		//���
		if (pBomJg && pBomJg->nPushFlat > 0)
			sPushFlat.Copy("*");
		lpInfo->SetSubItemText(8, sPushFlat, TRUE);
		if (pHashBoolByPropName&&pHashBoolByPropName->GetValue("PushFlat"))
			lpInfo->SetSubItemColor(8, clr);
		//����
		if (pBomJg && pBomJg->bCutBer)
			sCutBer.Copy("*");
		lpInfo->SetSubItemText(9, sCutBer, TRUE);
		if (pHashBoolByPropName&&pHashBoolByPropName->GetValue("CutBer"))
			lpInfo->SetSubItemColor(9, clr);
		//�ٽ�
		if (pBomJg && pBomJg->bCutRoot)
			sCutRoot.Copy("*");
		lpInfo->SetSubItemText(10, sCutRoot, TRUE);
		if (pHashBoolByPropName&&pHashBoolByPropName->GetValue("CutRoot"))
			lpInfo->SetSubItemColor(10, clr);
		//����
		if (pBomJg && pBomJg->bKaiJiao)
			sKaiJiao.Copy("*");
		lpInfo->SetSubItemText(11, sKaiJiao, TRUE);
		if (pHashBoolByPropName&&pHashBoolByPropName->GetValue("KaiJiao"))
			lpInfo->SetSubItemColor(11, clr);
		//�Ͻ�
		if (pBomJg && pBomJg->bHeJiao)
			sHeJiao.Copy("*");
		lpInfo->SetSubItemText(12, sHeJiao, TRUE);
		if (pHashBoolByPropName&&pHashBoolByPropName->GetValue("HeJiao"))
			lpInfo->SetSubItemColor(12, clr);
		//���Ŷ�
		if (pBomJg && pBomJg->bHasFootNail)
			sHasFootNail.Copy("*");
		lpInfo->SetSubItemText(13, sHasFootNail, TRUE);
		if (pHashBoolByPropName&&pHashBoolByPropName->GetValue("FootNail"))
			lpInfo->SetSubItemColor(13, clr);
		//��ע
		lpInfo->SetSubItemText(14, pPart->sNotes, TRUE);
		if (pHashBoolByPropName&&pHashBoolByPropName->GetValue("sNotes"))
			lpInfo->SetSubItemColor(14, clr);
	}
	if(pParentItem)
		return list.InsertItem(pParentItem,lpInfo,-1,true);
	else
		return list.InsertRootItem(lpInfo);
}
void CRevisionDlg::RefreshListCtrl(HTREEITEM hItem,BOOL bCompared/*=FALSE*/)
{
	if(hItem==NULL)
		return;
	m_sRecordNum="";
	m_sCurFile="";
	TREEITEM_INFO *pInfo=(TREEITEM_INFO*)m_treeCtrl.GetItemData(hItem);
	if(pInfo==NULL)
		return;
	CSuperGridCtrl::CTreeItem *pItem = NULL;
	int index = 0, nNum = 0;
	if(!bCompared)
	{
		if (DisplayProcess)
			DisplayProcess(0, "��ʾ��ȡ���......");
		if(pInfo->itemType==BOM_ITEM)
		{
			m_xListReport.DeleteAllItems();
			CBomFile* pBom=(CBomFile*)pInfo->dwRefData;
			nNum = pBom->GetPartNum();
			for(BOMPART *pPart=pBom->EnumFirstPart();pPart;pPart=pBom->EnumNextPart(), index++)
			{
				if (DisplayProcess)
					DisplayProcess(int(100 * index / nNum), "��ʾ��ȡ���......");
				pItem=InsertPartToList(m_xListReport,NULL,pPart,NULL);
				pItem->m_idProp=(long)pPart;
			}
			m_sCurFile.Format("%s",(char*)pBom->m_sBomName);
			m_sRecordNum=pBom->GetPartNumStr();
		}
		else if(pInfo->itemType==ANGLE_DWG_ITEM)
		{
			m_xListReport.DeleteAllItems();
			CDwgFileInfo* pDwg=(CDwgFileInfo*)pInfo->dwRefData;
			nNum = pDwg->GetJgNum();
			for(CAngleProcessInfo *pAngleInfo=pDwg->EnumFirstJg();pAngleInfo;pAngleInfo=pDwg->EnumNextJg(),index++)
			{
				if (DisplayProcess)
					DisplayProcess(int(100 * index / nNum), "��ʾ��ȡ���......");
				pItem=InsertPartToList(m_xListReport,NULL,&pAngleInfo->m_xAngle,NULL);
				pItem->m_idProp=(long)pAngleInfo;
			}
			m_sCurFile.Format("%s",(char*)pDwg->m_sDwgName);
			m_sRecordNum.Format("%d",pDwg->GetJgNum());
		}
		else if(pInfo->itemType==PLATE_DWG_ITEM)
		{
			m_xListReport.DeleteAllItems();
			CDwgFileInfo* pDwg=(CDwgFileInfo*)pInfo->dwRefData;
			nNum = pDwg->GetPlateNum();
			for(CPlateProcessInfo *pPlateInfo=pDwg->EnumFirstPlate();pPlateInfo;pPlateInfo=pDwg->EnumNextPlate(),index++)
			{
				if (DisplayProcess)
					DisplayProcess(int(100 * index / nNum), "��ʾ��ȡ���......");
				pItem=InsertPartToList(m_xListReport,NULL,&pPlateInfo->xBomPlate,NULL);
				pItem->m_idProp=(long)pPlateInfo;
			}
			m_sCurFile.Format("%s",(char*)pDwg->m_sDwgName);
			m_sRecordNum.Format("%d", pDwg->GetPlateNum());
		}
		if (DisplayProcess)
			DisplayProcess(100, "��ʾ��ȡ���......");
	}
	else
	{
		if (DisplayProcess)
			DisplayProcess(0, "��ʾУ����......");
		m_xListReport.DeleteAllItems();
		CProjectTowerType::COMPARE_PART_RESULT *pResult=NULL;
		CProjectTowerType* pProject=GetProject(hItem);
		m_sRecordNum.Format("%d", pProject->GetResultCount());
		if(m_iCompareMode==CProjectTowerType::COMPARE_BOM_FILE)
		{
			nNum = pProject->GetResultCount();
			for(pResult=pProject->EnumFirstResult();pResult;pResult=pProject->EnumNextResult(),index++)
			{	
				if (DisplayProcess)
					DisplayProcess(int(100 * index / nNum), "��ʾУ����......");
				//������в����ñ���ɫ
				if (pResult->pOrgPart)
				{
					pItem = InsertPartToList(m_xListReport, NULL, pResult->pOrgPart);	
					if (pResult->pLoftPart == NULL)
					{	//�����У�����û��
						pItem->m_bStrikeout = TRUE;
						pItem->SetBkColor(RGB(140, 140, 255));
					}
					else //���ݲ�һ��
						InsertPartToList(m_xListReport, pItem, pResult->pLoftPart, &pResult->hashBoolByPropName);
				}
				else
				{
					pItem = InsertPartToList(m_xListReport, NULL, pResult->pLoftPart);
					pItem->SetBkColor(RGB(128, 128, 255));
				}
			}
			m_sCurFile.Format("%s��%sУ��",(char*)pProject->m_xLoftBom.m_sBomName,(char*)pProject->m_xOrigBom.m_sBomName);
		}
		else if(m_iCompareMode==CProjectTowerType::COMPARE_ANGLE_DWG)
		{
			CDwgFileInfo* pJgDwg=(CDwgFileInfo*)pInfo->dwRefData;
			nNum = pProject->GetResultCount();
			for (pResult = pProject->EnumFirstResult(); pResult; pResult = pProject->EnumNextResult(),index++)
			{
				if (DisplayProcess)
					DisplayProcess(int(100 * index / nNum), "��ʾУ����......");
				if (pResult->pOrgPart && pResult->pOrgPart->cPartType == BOMPART::ANGLE)
				{
					CAngleProcessInfo *pAngleInfo=pJgDwg->FindAngleByPartNo(pResult->pOrgPart->sPartNo);
					pItem = InsertPartToList(m_xListReport, NULL, pResult->pOrgPart, NULL);
					if(pAngleInfo)
						pItem->m_idProp = (long)pAngleInfo;
					if (pResult->pLoftPart == NULL)
					{	//DWG��û�д˹���
						pItem->m_bStrikeout = TRUE;
						pItem->SetBkColor(RGB(140, 140, 255));
					}
					else
						InsertPartToList(m_xListReport, pItem, pResult->pLoftPart, &pResult->hashBoolByPropName);
				}
				else if (pResult->pOrgPart == NULL && pResult->pLoftPart && pResult->pLoftPart->cPartType == BOMPART::ANGLE)
				{
					pItem = InsertPartToList(m_xListReport, NULL, pResult->pLoftPart, NULL);
					pItem->SetBkColor(RGB(128, 128, 255));
				}
			}
			m_sCurFile.Format("%s��%sУ��",(char*)pProject->m_xLoftBom.m_sBomName,(char*)pJgDwg->m_sDwgName);
		}
		else if(m_iCompareMode==CProjectTowerType::COMPARE_PLATE_DWG)
		{
			CDwgFileInfo* pPlateDwg=(CDwgFileInfo*)pInfo->dwRefData;
			nNum = pProject->GetResultCount();
			for (pResult = pProject->EnumFirstResult(); pResult; pResult = pProject->EnumNextResult(), index++)
			{
				if (DisplayProcess)
					DisplayProcess(int(100 * index / nNum), "��ʾУ����......");
				if (pResult->pOrgPart && pResult->pOrgPart->cPartType==BOMPART::PLATE)
				{
					CPlateProcessInfo *pPlateInfo = pPlateDwg->FindPlateByPartNo(pResult->pOrgPart->sPartNo);
					pItem = InsertPartToList(m_xListReport, NULL, pResult->pOrgPart, NULL);
					if(pPlateInfo)
						pItem->m_idProp = (long)pPlateInfo;
					if (pResult->pLoftPart == NULL)
					{	//��������û�д˹���
						pItem->m_bStrikeout = TRUE;
						pItem->SetBkColor(RGB(140, 140, 255));
					}
					else
						InsertPartToList(m_xListReport, pItem, pResult->pLoftPart, &pResult->hashBoolByPropName);

				}
				else if (pResult->pOrgPart==NULL && pResult->pLoftPart &&pResult->pLoftPart->cPartType == BOMPART::PLATE)
				{
					pItem = InsertPartToList(m_xListReport, NULL, pResult->pLoftPart, NULL);
					pItem->SetBkColor(RGB(128, 128, 255));
				}
			}
			m_sCurFile.Format("%s��%sУ��",(char*)pProject->m_xLoftBom.m_sBomName,(char*)pPlateDwg->m_sDwgName);
		}
		else if(m_iCompareMode==CProjectTowerType::COMPARE_ANGLE_DWGS || 
			m_iCompareMode==CProjectTowerType::COMPARE_PLATE_DWGS)
		{
			if(m_iCompareMode==CProjectTowerType::COMPARE_ANGLE_DWGS)
				m_sCurFile="�Ѵ򿪽Ǹ�DWG�ļ�©�ż��";
			else
				m_sCurFile="�Ѵ򿪸ְ�DWG�ļ�©�ż��";
			nNum = pProject->GetResultCount();
			for(pResult=pProject->EnumFirstResult();pResult;pResult=pProject->EnumNextResult(), index++)
			{	//������в����ñ���ɫ
				if (DisplayProcess)
					DisplayProcess(int(100 * index / nNum), "��ʾУ����......");
				if(pResult->pOrgPart)
					continue;
				CSuperGridCtrl::CTreeItem *pItem=InsertPartToList(m_xListReport,NULL,pResult->pLoftPart);
				pItem->SetBkColor(RGB(150,220,150));
			}
		}
		if (DisplayProcess)
			DisplayProcess(100, "��ʾУ����......");
	}
	UpdateData(FALSE);
}
void CRevisionDlg::RefreshTreeCtrl()
{
	CString ss;
	itemInfoList.Empty();
	m_treeCtrl.DeleteAllItems();
	HTREEITEM hItem=NULL;
	HTREEITEM hRootItem=m_treeCtrl.InsertItem("��������",PRJ_IMG_CALMODULE,PRJ_IMG_CALMODULE,TVI_ROOT);
	TREEITEM_INFO *pItemInfo=itemInfoList.append(TREEITEM_INFO(PROJECT_GROUP,0));
	m_treeCtrl.SetItemData(hRootItem,(DWORD)pItemInfo);
	for(CProjectTowerType *pPrjTowerType=g_xUbomModel.m_xPrjTowerTypeList.GetFirst();pPrjTowerType;pPrjTowerType=g_xUbomModel.m_xPrjTowerTypeList.GetNext())
		RefreshProjectItem(hRootItem, pPrjTowerType);
}
void CRevisionDlg::RefreshProjectItem(HTREEITEM hParenItem,CProjectTowerType* pProject)
{
	if(pProject==NULL)
		return;
	CTreeCtrl *pTreeCtrl=GetTreeCtrl();
	HTREEITEM hProjItem,hBomGroup,hJgGroup,hPlateGroup,hSonItem;
	hProjItem=pTreeCtrl->InsertItem(pProject->m_sProjName,PRJ_IMG_MODULECASE,PRJ_IMG_MODULECASE,hParenItem);
	TREEITEM_INFO* pItemInfo=itemInfoList.append(TREEITEM_INFO(PROJECT_ITEM,(DWORD)pProject));
	pTreeCtrl->SetItemData(hProjItem,(DWORD)pItemInfo);
	//�ϵ��ڵ�
	hBomGroup=pTreeCtrl->InsertItem("�ϵ��ļ���",PRJ_IMG_FILEGROUP,PRJ_IMG_FILEGROUP,hProjItem);
	pItemInfo=itemInfoList.append(TREEITEM_INFO(BOM_GROUP,NULL));
	pTreeCtrl->SetItemData(hBomGroup,(DWORD)pItemInfo);
	if(pProject->m_xLoftBom.GetPartNum()>0)
	{
		hSonItem=pTreeCtrl->InsertItem(pProject->m_xLoftBom.m_sBomName,PRJ_IMG_FILE,PRJ_IMG_FILE,hBomGroup);
		pItemInfo=itemInfoList.append(TREEITEM_INFO(BOM_ITEM,(DWORD)&(pProject->m_xLoftBom)));
		pTreeCtrl->SetItemData(hSonItem,(DWORD)pItemInfo);
	}
	if(pProject->m_xOrigBom.GetPartNum()>0)
	{
		hSonItem=pTreeCtrl->InsertItem(pProject->m_xOrigBom.m_sBomName,PRJ_IMG_FILE,PRJ_IMG_FILE,hBomGroup);
		pItemInfo=itemInfoList.append(TREEITEM_INFO(BOM_ITEM,(DWORD)&(pProject->m_xOrigBom)));
		pTreeCtrl->SetItemData(hSonItem,(DWORD)pItemInfo);
	}
	//dwg�ļ��ڵ�
	hJgGroup=pTreeCtrl->InsertItem("�Ǹ�dwg�ļ���",PRJ_IMG_FILEGROUP,PRJ_IMG_FILEGROUP,hProjItem);
	pItemInfo=itemInfoList.append(TREEITEM_INFO(ANGLE_GROUP,NULL));
	pTreeCtrl->SetItemData(hJgGroup,(DWORD)pItemInfo);
	hPlateGroup=pTreeCtrl->InsertItem("�ְ�dwg�ļ���",PRJ_IMG_FILEGROUP,PRJ_IMG_FILEGROUP,hProjItem);
	pItemInfo=itemInfoList.append(TREEITEM_INFO(PLATE_GROUP,NULL));
	pTreeCtrl->SetItemData(hPlateGroup,(DWORD)pItemInfo);
	for(CDwgFileInfo* pDwgInfo=pProject->dwgFileList.GetFirst();pDwgInfo;pDwgInfo=pProject->dwgFileList.GetNext())
	{
		if(pDwgInfo->IsJgDwgInfo())
		{
			hSonItem=pTreeCtrl->InsertItem(pDwgInfo->m_sDwgName,PRJ_IMG_FILE,PRJ_IMG_FILE,hJgGroup);
			pItemInfo=itemInfoList.append(TREEITEM_INFO(ANGLE_DWG_ITEM,(DWORD)pDwgInfo));
			pTreeCtrl->SetItemData(hSonItem,(DWORD)pItemInfo);
		}
		else
		{
			hSonItem=pTreeCtrl->InsertItem(pDwgInfo->m_sDwgName,PRJ_IMG_FILE,PRJ_IMG_FILE,hPlateGroup);
			pItemInfo=itemInfoList.append(TREEITEM_INFO(PLATE_DWG_ITEM,(DWORD)pDwgInfo));
			pTreeCtrl->SetItemData(hSonItem,(DWORD)pItemInfo);
		}
	}
	//
	pTreeCtrl->Expand(hProjItem,TVE_EXPAND);
	pTreeCtrl->Expand(hBomGroup,TVE_EXPAND);
	pTreeCtrl->Expand(hJgGroup,TVE_EXPAND);
	pTreeCtrl->Expand(hPlateGroup,TVE_EXPAND);
	pTreeCtrl->SelectItem(hProjItem);
}
void CRevisionDlg::ContextMenu(CWnd *pWnd, CPoint point)
{
	CTreeCtrl *pTreeCtrl=GetTreeCtrl();
	if(pTreeCtrl==NULL)
		return;
	//��ʼ���˵�
	CMenu popMenu;
	popMenu.LoadMenu(IDR_ITEM_CMD_POPUP);
	CMenu *pMenu=popMenu.GetSubMenu(0);
	while(pMenu->GetMenuItemCount()>0)
		pMenu->DeleteMenu(0,MF_BYPOSITION);
	//��Ӳ˵���
	CPoint scr_point = point;
	pTreeCtrl->ClientToScreen(&scr_point);
	HTREEITEM hItem=pTreeCtrl->GetSelectedItem();
	TREEITEM_INFO *pItemInfo=NULL;
	if(hItem)
		pItemInfo=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hItem);
	if(pItemInfo==NULL)
		return;
	if(pItemInfo->itemType==PROJECT_GROUP)	//����������
	{
		pMenu->AppendMenu(MF_STRING,ID_NEW_ITEM,"�½�����");
		pMenu->AppendMenu(MF_STRING,ID_LOAD_PROJECT,"���ع����ļ�");
	}
	else if(pItemInfo->itemType==PROJECT_ITEM)
	{
		pMenu->AppendMenu(MF_STRING,ID_PROJECT_PROPERTY,"�޸Ĺ�������");
		pMenu->AppendMenu(MF_STRING,ID_EXPORT_PROJECT,"���ɹ����ļ�");
	}
	else if(pItemInfo->itemType==BOM_GROUP)
	{
		pMenu->AppendMenu(MF_STRING,ID_IMPORT_BOM_FILE,"�����ϵ��ļ�");
		if (g_xUbomModel.m_uiCustomizeSerial== CBomModel::ID_AnHui_HongYuan)
		{	//���պ�ԴҪ�����EPR��TMA���ϵ�У��
			pMenu->AppendMenu(MF_STRING, ID_MODIFY_ERP_FILE, "����BOM����");
			//pMenu->AppendMenu(MF_STRING, ID_MODIFY_TMA_FILE, "������������");
			pMenu->AppendMenu(MF_STRING, ID_COMPARE_DATA, "�ϵ�����У��");
			pMenu->AppendMenu(MF_STRING, ID_EXPORT_COMPARE_RESULT, "����У����");
		}
		else if (g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_JiangSu_HuaDian)
		{	//���ջ���Ҫ����з�����ͼֽ���ϵ�У��
			pMenu->AppendMenu(MF_STRING, ID_COMPARE_DATA, "�ϵ�����У��");
			pMenu->AppendMenu(MF_STRING, ID_EXPORT_COMPARE_RESULT, "����У����");
		}
	}
	else if(pItemInfo->itemType==ANGLE_GROUP)
	{
		pMenu->AppendMenu(MF_STRING,ID_IMPORT_ANGLE_DWG,"���ؽǸ�DWG�ļ�");
		pMenu->AppendMenu(MF_STRING,ID_COMPARE_DATA,"�Ǹ�DWG©�ż��");
		pMenu->AppendMenu(MF_STRING,ID_EXPORT_COMPARE_RESULT,"����©�ż����");
	}
	else if(pItemInfo->itemType==PLATE_GROUP)
	{
		pMenu->AppendMenu(MF_STRING,ID_IMPORT_PLATE_DWG,"���ظְ�DWG�ļ�");
		pMenu->AppendMenu(MF_STRING,ID_COMPARE_DATA,"�ְ�DWG©�ż��");
		pMenu->AppendMenu(MF_STRING,ID_EXPORT_COMPARE_RESULT,"����©�ż����");
	}
	else if(pItemInfo->itemType==BOM_ITEM)
		pMenu->AppendMenu(MF_STRING, ID_DELETE_ITEM, "ɾ���ϱ�");
	else if(pItemInfo->itemType==ANGLE_DWG_ITEM)
	{
		pMenu->AppendMenu(MF_STRING,ID_COMPARE_DATA,"�Ǹ�����У��");
		pMenu->AppendMenu(MF_STRING,ID_EXPORT_COMPARE_RESULT,"����У����");
		if (g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_AnHui_HongYuan||
			g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_AnHui_TingYang)
			pMenu->AppendMenu(MF_STRING,ID_REFRESH_PART_NUM,"���¼ӹ���");
		pMenu->AppendMenu(MF_SEPARATOR);
		pMenu->AppendMenu(MF_STRING, ID_RETRIEVED_ANGLES, "������ȡ�Ǹ�");
		pMenu->AppendMenu(MF_STRING, ID_DELETE_ITEM, "ɾ���Ǹ�ͼֽ");
	}
	else if(pItemInfo->itemType==PLATE_DWG_ITEM)
	{
		pMenu->AppendMenu(MF_STRING,ID_COMPARE_DATA,"�ְ�����У��");
		pMenu->AppendMenu(MF_STRING,ID_EXPORT_COMPARE_RESULT,"����У����");
		if (g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_AnHui_HongYuan||
			g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_AnHui_TingYang)
			pMenu->AppendMenu(MF_STRING, ID_REFRESH_PART_NUM, "���¼ӹ���");
		pMenu->AppendMenu(MF_SEPARATOR);
		pMenu->AppendMenu(MF_STRING, ID_RETRIEVED_PLATES,"������ȡ�ְ�");
		pMenu->AppendMenu(MF_STRING, ID_DELETE_ITEM, "ɾ���ְ�ͼֽ");
	}
	pMenu->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON,scr_point.x,scr_point.y,this);
}
//
void CRevisionDlg::OnNMRClickTreeCtrl(NMHDR *pNMHDR, LRESULT *pResult)
{
	TVHITTESTINFO HitTestInfo;
	GetCursorPos(&HitTestInfo.pt);
	CXhTreeCtrl *pTreeCtrl = GetTreeCtrl();
	if(pTreeCtrl==NULL)
		return;
	pTreeCtrl->ScreenToClient(&HitTestInfo.pt);
	pTreeCtrl->HitTest(&HitTestInfo);
	pTreeCtrl->Select(HitTestInfo.hItem,TVGN_CARET);
	ContextMenu(pTreeCtrl,HitTestInfo.pt);
	*pResult = 0;
}
void CRevisionDlg::OnTvnSelchangedTreeContrl(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	HTREEITEM hItem = m_treeCtrl.GetSelectedItem();
	if(hItem==NULL)
		return;
	TREEITEM_INFO *pItemInfo=(TREEITEM_INFO*)m_treeCtrl.GetItemData(hItem);
	if(pItemInfo->itemType==ANGLE_DWG_ITEM || pItemInfo->itemType==PLATE_DWG_ITEM)
	{
		CDwgFileInfo* pDwgInfo=(CDwgFileInfo*)pItemInfo->dwRefData;
		AcApDocument *pDoc=NULL;
		AcApDocumentIterator *pIter=acDocManager->newAcApDocumentIterator();
		for(;!pIter->done();pIter->step())
		{
			pDoc=pIter->document();
			CXhChar500 file_path;
#ifdef _ARX_2007
			file_path.Copy(_bstr_t(pDoc->fileName()));
#else
			file_path.Copy(pDoc->fileName());
#endif
			if(strstr(file_path,pDwgInfo->m_sFileName))
				break;
		}
		if(pDoc)
			acDocManager->activateDocument(pDoc);	//����ָ���ļ�
	}
	RefreshListCtrl(hItem);
	*pResult = 0;
}
//�½���Ŀ
void CRevisionDlg::OnNewItem()
{
	CXhTreeCtrl *pTreeCtrl=GetTreeCtrl();
	if(pTreeCtrl==NULL)
		return;
	HTREEITEM hSelectedItem=pTreeCtrl->GetSelectedItem();
	TREEITEM_INFO *pItemInfo=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hSelectedItem);
	if(pItemInfo==NULL)
		return;
	if(pItemInfo->itemType==PROJECT_GROUP)
	{	//�½����ι���
		CProjectTowerType* pProject=g_xUbomModel.m_xPrjTowerTypeList.Add(0);
		pProject->m_sProjName.Copy("�½�����");
		RefreshProjectItem(hSelectedItem,pProject);
	}
}
//���ع����ļ�
void CRevisionDlg::OnLoadProjFile()
{
	CXhTreeCtrl *pTreeCtrl=GetTreeCtrl();
	if(pTreeCtrl==NULL)
		return;
	HTREEITEM hSelectedItem=pTreeCtrl->GetSelectedItem();
	TREEITEM_INFO *pItemInfo=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hSelectedItem);
	if(pItemInfo==NULL || pItemInfo->itemType!=PROJECT_GROUP)
		return;
	CFileDialog dlg(TRUE,"ubm","�����ļ�",OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,"�����ļ�(*.ubm)|*.ubm||");
	if(dlg.DoModal()!=IDOK)
		return;
	CProjectTowerType* pProject=g_xUbomModel.m_xPrjTowerTypeList.Add(0);
	pProject->ReadProjectFile(dlg.GetPathName());
	//ˢ�����б�
	RefreshProjectItem(hSelectedItem,pProject);
}
//���ɹ����ļ�
void CRevisionDlg::OnExportProjFile()
{
	CXhTreeCtrl *pTreeCtrl=GetTreeCtrl();
	if(pTreeCtrl==NULL)
		return;
	HTREEITEM hSelectedItem=pTreeCtrl->GetSelectedItem();
	TREEITEM_INFO *pItemInfo=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hSelectedItem);
	if(pItemInfo==NULL || pItemInfo->itemType!=PROJECT_ITEM)
		return;
	CProjectTowerType* pProject=(CProjectTowerType*)pItemInfo->dwRefData;
	CFileDialog dlg(FALSE,"ubm",pProject->m_sProjName,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,"�ϵ������ļ�(*.ubm)|*.ubm||");
	if(dlg.DoModal()==IDOK)
		pProject->WriteProjectFile(dlg.GetPathName());	
}
//�趨��������
void CRevisionDlg::OnProjectProperty()
{
	CXhTreeCtrl *pTreeCtrl=GetTreeCtrl();
	HTREEITEM hSelectedItem=pTreeCtrl->GetSelectedItem();
	TREEITEM_INFO *pItemInfo=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hSelectedItem);
	if(pItemInfo==NULL || pItemInfo->itemType!=PROJECT_ITEM)
		return;
	CProjectTowerType* pProject=(CProjectTowerType*)pItemInfo->dwRefData;
	CInputAnStringValDlg dlg;
	if(dlg.DoModal()!=IDOK)
		return;
	if(strlen(dlg.m_sItemValue)>0)
	{
		pProject->m_sProjName.Copy(dlg.m_sItemValue);
		pTreeCtrl->SetItemText(hSelectedItem,dlg.m_sItemValue);
	}
}
//�����ϵ��ļ�(�����ϵ��������ϵ�)
void CRevisionDlg::OnImportBomFile()
{
	CLogErrorLife logErrLife;
	CXhTreeCtrl *pTreeCtrl=GetTreeCtrl();
	HTREEITEM hItem,hSelectedItem;
	hSelectedItem=pTreeCtrl->GetSelectedItem();
	TREEITEM_INFO* pItemInfo=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hSelectedItem);
	if(pItemInfo==NULL || pItemInfo->itemType!=BOM_GROUP)
		return;
	CFileDialog dlg(TRUE,"xls","�����嵥.xls",
		OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_ALLOWMULTISELECT,
		"BOM�ļ�|*.xls;*.xlsx|Excel(*.xls)|*.xls|Excel(*.xlsx)|*.xlsx|�����ļ�(*.*)|*.*||");
	if (g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_AnHui_HongYuan)
		dlg.m_ofn.lpstrTitle = "ѡ��TMA���������嵥��������ERP�����嵥";
	else if(g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_JiangSu_HuaDian)
		dlg.m_ofn.lpstrTitle = "ѡ�������ͼ�����嵥��ͼֽ�����嵥";
	else
		dlg.m_ofn.lpstrTitle = "ѡ�������嵥";
	if(dlg.DoModal()!=IDOK)
		return;
	CWaitCursor waitCursor;
	CProjectTowerType* pProject=GetProject(hSelectedItem);
	if (g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_AnHui_HongYuan||
		g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_JiangSu_HuaDian)
	{	//���պ�Դ��Ҫ����TMA�ϵ���ERP�ϵ�
		//���ջ�����Ҫ��������ϵ���ͼֽ�ϵ�
		POSITION pos = dlg.GetStartPosition();
		while (pos)
		{
			CString sFilePath = dlg.GetNextPathName(pos);
			if(pProject->IsTmaBomFile(sFilePath))
				pProject->InitBomInfo(sFilePath, TRUE);
			else if (pProject->IsErpBomFile(sFilePath))
				pProject->InitBomInfo(sFilePath, FALSE);
		}
		//���BOM���ڵ�
		pTreeCtrl->Expand(hSelectedItem, TVE_EXPAND);
		if (pProject->m_xLoftBom.GetPartNum() > 0)
		{
			hItem = FindTreeItem(hSelectedItem, pProject->m_xLoftBom.m_sBomName);
			if (hItem)
				pTreeCtrl->DeleteItem(hItem);
			hItem = m_treeCtrl.InsertItem(pProject->m_xLoftBom.m_sBomName, PRJ_IMG_FILE, PRJ_IMG_FILE, hSelectedItem);
			pItemInfo = itemInfoList.append(TREEITEM_INFO(BOM_ITEM, (DWORD)&(pProject->m_xLoftBom)));
			m_treeCtrl.SetItemData(hItem, (DWORD)pItemInfo);
			m_treeCtrl.SelectItem(hItem);
		}
		else
		{
			if (g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_AnHui_HongYuan)
				logerr.Log("���ط��������嵥ʧ��!");
			else if(g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_JiangSu_HuaDian)
				logerr.Log("���ط�����ͼ�����嵥ʧ��!");
			else
				logerr.Log("���������嵥ʧ��!");
		}
		//
		if (pProject->m_xOrigBom.GetPartNum() > 0)
		{
			hItem = FindTreeItem(hSelectedItem, pProject->m_xOrigBom.m_sBomName);
			if (hItem)
				pTreeCtrl->DeleteItem(hItem);
			hItem = m_treeCtrl.InsertItem(pProject->m_xOrigBom.m_sBomName, PRJ_IMG_FILE, PRJ_IMG_FILE, hSelectedItem);
			pItemInfo = itemInfoList.append(TREEITEM_INFO(BOM_ITEM, (DWORD)&(pProject->m_xOrigBom)));
			m_treeCtrl.SetItemData(hItem, (DWORD)pItemInfo);
			m_treeCtrl.SelectItem(hItem);
		}
		else
		{
			if (g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_AnHui_HongYuan)
				logerr.Log("����ERP�����嵥ʧ��!");
			else if(g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_JiangSu_HuaDian)
				logerr.Log("����ͼֽ�����嵥!");
			else
				logerr.Log("���������嵥ʧ��!");
		}		
	}
	else
	{	//ֻ�赼��һ���ϵ�
		CString sFilePath = dlg.GetPathName();//��ȡ�ļ��� 
		pProject->InitBomInfo(sFilePath, TRUE);
		pTreeCtrl->Expand(hSelectedItem, TVE_EXPAND);
		if (pProject->m_xLoftBom.GetPartNum() > 0)
		{
			hItem = FindTreeItem(hSelectedItem, pProject->m_xLoftBom.m_sBomName);
			if (hItem)
				pTreeCtrl->DeleteItem(hItem);
			hItem = m_treeCtrl.InsertItem(pProject->m_xLoftBom.m_sBomName, PRJ_IMG_FILE, PRJ_IMG_FILE, hSelectedItem);
			pItemInfo = itemInfoList.append(TREEITEM_INFO(BOM_ITEM, (DWORD)&(pProject->m_xLoftBom)));
			m_treeCtrl.SetItemData(hItem, (DWORD)pItemInfo);
			m_treeCtrl.SelectItem(hItem);
		}
		else
			logerr.Log("���������嵥ʧ��!");
	}	
}
//����Ǹ�DWG�ļ�
void CRevisionDlg::OnImportAngleDwg()
{
	CLogErrorLife logErrLife;
	CXhTreeCtrl *pTreeCtrl=GetTreeCtrl();
	HTREEITEM hItem,hSelectedItem;
	hSelectedItem=pTreeCtrl->GetSelectedItem();
	TREEITEM_INFO *pItemInfo=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hSelectedItem);
	if(pItemInfo==NULL || pItemInfo->itemType!=ANGLE_GROUP)
		return;
	CFileDialog dlg(TRUE,"dwg","�Ǹּӹ��ļ�.dwg",
					OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,//|OFN_ALLOWMULTISELECT,
					"DWG�ļ�(*.dwg)|*.dwg|�����ļ�(*.*)|*.*||");
	if(dlg.DoModal()!=IDOK)
		return;
	CProjectTowerType* pProject=GetProject(hSelectedItem);
	CDwgFileInfo* pJgDwgInfo=pProject->AppendDwgBomInfo(dlg.GetPathName(),TRUE);
	if(pJgDwgInfo==NULL)
		return;
	//��ӽǸ�DWG�ļ����ڵ�
	hItem=FindTreeItem(hSelectedItem, pJgDwgInfo->m_sDwgName);
	if(hItem==NULL)
	{
		hItem=m_treeCtrl.InsertItem(pJgDwgInfo->m_sDwgName,PRJ_IMG_FILE,PRJ_IMG_FILE,hSelectedItem);
		pItemInfo=itemInfoList.append(TREEITEM_INFO(ANGLE_DWG_ITEM,(DWORD)pJgDwgInfo));
		m_treeCtrl.SetItemData(hItem,(DWORD)pItemInfo);
	}
	m_treeCtrl.SelectItem(hItem);
	pTreeCtrl->Expand(hSelectedItem,TVE_EXPAND);
	RefreshListCtrl(hItem);
}
//����ְ�DWG�ļ�
void CRevisionDlg::OnImportPlateDwg()
{
	CLogErrorLife logErrLife;
	CXhTreeCtrl *pTreeCtrl=GetTreeCtrl();
	HTREEITEM hItem,hSelectedItem;
	hSelectedItem=pTreeCtrl->GetSelectedItem();
	TREEITEM_INFO *pItemInfo=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hSelectedItem);
	if(pItemInfo==NULL || pItemInfo->itemType!=PLATE_GROUP)
		return;
	CFileDialog dlg(TRUE,"dwg","�ְ�ӹ��ļ�.dwg",
					OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,//|OFN_ALLOWMULTISELECT,
					"DWG�ļ�(*.dwg)|*.dwg|�����ļ�(*.*)|*.*||");
	if(dlg.DoModal()!=IDOK)
		return;
	CProjectTowerType* pProject=GetProject(hSelectedItem);
	CDwgFileInfo* pPlateDwgInfo=pProject->AppendDwgBomInfo(dlg.GetPathName(),FALSE);
	if(pPlateDwgInfo==NULL)
		return;
	//��Ӹְ�DWG�ļ����ڵ�
	hItem=FindTreeItem(hSelectedItem, pPlateDwgInfo->m_sDwgName);
	if(hItem==NULL)
	{
		hItem=m_treeCtrl.InsertItem(pPlateDwgInfo->m_sDwgName,PRJ_IMG_FILE,PRJ_IMG_FILE,hSelectedItem);
		pItemInfo=itemInfoList.append(TREEITEM_INFO(PLATE_DWG_ITEM,(DWORD)pPlateDwgInfo));
		m_treeCtrl.SetItemData(hItem,(DWORD)pItemInfo);
	}
	m_treeCtrl.SelectItem(hItem);
	pTreeCtrl->Expand(hSelectedItem,TVE_EXPAND);
	RefreshListCtrl(hItem);
}
void CRevisionDlg::OnMove(int x, int y)
{
	CDialog::OnMove(x, y);

	if(m_bEnableWindowMoveListen)
	{
		m_nScrLocationX=x;
		m_nScrLocationY=y;
	}
}
void CRevisionDlg::OnSize(UINT nType, int cx, int cy)
{
	RECT rect;
	CWnd* pWnd=CWnd::GetDlgItem(IDC_TREE_CONTRL);
	if(pWnd->GetSafeHwnd()!=NULL)
	{
		/*pWnd->GetWindowRect(&rect);
		ScreenToClient(&rect);
		rect.right=cx-m_nRightMargin;
		rect.bottom=cy-m_nBtmMargin;
		pWnd->MoveWindow(&rect);*/
	}
	pWnd=CWnd::GetDlgItem(IDC_LIST_REPORT);
	if(pWnd->GetSafeHwnd()!=NULL)
	{
		pWnd->GetWindowRect(&rect);
		ScreenToClient(&rect);
		rect.bottom=cy-m_nBtmMargin;
		rect.right = cx - m_nRightMargin;
		pWnd->MoveWindow(&rect);
	}
	CDialog::OnSize(nType, cx, cy);
}
//
void CRevisionDlg::OnCompareData()
{
	CLogErrorLife logErrLife;
	CXhTreeCtrl *pTreeCtrl=GetTreeCtrl();
	HTREEITEM hSelItem=pTreeCtrl->GetSelectedItem();
	if(hSelItem==NULL)
		return;
	TREEITEM_INFO *pItemInfo=(TREEITEM_INFO*)pTreeCtrl->GetItemData(hSelItem);
	CProjectTowerType* pProject=GetProject(hSelItem);
	if(pProject==NULL)
	{
		logerr.Log("û���ҵ���������");
		return;
	}
	if(pItemInfo->itemType==ANGLE_DWG_ITEM)
	{	//����TMA�ϵ��ͽǸ�DWG�Ա�
		m_iCompareMode=CProjectTowerType::COMPARE_ANGLE_DWG;
		CDwgFileInfo* pDwgInfo=(CDwgFileInfo*)pItemInfo->dwRefData;
		if(pProject->CompareLoftAndAngleDwg(pDwgInfo->m_sFileName)!=1)
			return;
	}
	else if(pItemInfo->itemType==PLATE_DWG_ITEM)
	{	//����TMA�ϵ��͸ְ�DWG�Ա�
		m_iCompareMode=CProjectTowerType::COMPARE_PLATE_DWG;
		CDwgFileInfo* pDwgInfo=(CDwgFileInfo*)pItemInfo->dwRefData;
		if(pProject->CompareLoftAndPlateDwg(pDwgInfo->m_sFileName)!=1)
			return;
	}
	else if(pItemInfo->itemType==BOM_GROUP)
	{	//���з�����ERP���ϵ��Ա�
		m_iCompareMode=CProjectTowerType::COMPARE_BOM_FILE;
		if(pProject->CompareOrgAndLoftParts()!=1)
			return;
	}
	else if(pItemInfo->itemType==ANGLE_GROUP)
	{	//�Ǹ�DWG�ļ�����©�ż��
		m_iCompareMode=CProjectTowerType::COMPARE_ANGLE_DWGS;
		if(pProject->CompareLoftAndAngleDwgs()!=1)
			return;
	}
	else if(pItemInfo->itemType==PLATE_GROUP)
	{	//�ְ�DWG�ļ�����©�ż��
		m_iCompareMode=CProjectTowerType::COMPARE_PLATE_DWGS;
		if(pProject->CompareLoftAndPlateDwgs()!=1)
			return;
	}
	RefreshListCtrl(hSelItem,TRUE);
}
//
void CRevisionDlg::OnExportCompResult()
{
	HTREEITEM hSelItem=m_treeCtrl.GetSelectedItem();
	TREEITEM_INFO *pItemInfo=(TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelItem);
	CProjectTowerType* pProject=GetProject(hSelItem);
	if(pProject==NULL)
		return;
	if(pItemInfo->itemType==BOM_GROUP && pProject->GetResultCount()>0)
		pProject->ExportCompareResult(CProjectTowerType::COMPARE_BOM_FILE);
	else if(pItemInfo->itemType==ANGLE_DWG_ITEM && pProject->GetResultCount()>0)
		pProject->ExportCompareResult(CProjectTowerType::COMPARE_ANGLE_DWG);
	else if(pItemInfo->itemType==PLATE_DWG_ITEM && pProject->GetResultCount()>0)
		pProject->ExportCompareResult(CProjectTowerType::COMPARE_PLATE_DWG);
	else if(pItemInfo->itemType==ANGLE_GROUP && pProject->GetResultCount()>0)
		pProject->ExportCompareResult(CProjectTowerType::COMPARE_ANGLE_DWGS);
	else if(pItemInfo->itemType==PLATE_GROUP && pProject->GetResultCount()>0)
		pProject->ExportCompareResult(CProjectTowerType::COMPARE_PLATE_DWGS);
	else
		AfxMessageBox("�ȶԽ����ͬ!");
}
//
void CRevisionDlg::OnRefreshPartNum()
{
	CLogErrorLife logErrLife;
	HTREEITEM hSelItem=m_treeCtrl.GetSelectedItem();
	TREEITEM_INFO *pItemInfo;
	pItemInfo=(TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelItem);
	if(pItemInfo==NULL)
		return;
	CDwgFileInfo* pDwgInfo=(CDwgFileInfo*)pItemInfo->dwRefData;
	if(pDwgInfo->IsJgDwgInfo())
		pDwgInfo->ModifyAngleDwgPartNum();
	else
		pDwgInfo->ModifyPlateDwgPartNum();
#ifdef _ARX_2007
	SendCommandToCad(CString(L"RE "));
#else
	SendCommandToCad(CString("RE "));
#endif
	RefreshListCtrl(hSelItem);
}
//
void CRevisionDlg::OnModifyErpFile()
{
	CLogErrorLife logErrLife;
	CTreeCtrl* pTree=GetTreeCtrl();
	HTREEITEM hSelItem=pTree->GetSelectedItem();
	TREEITEM_INFO *pItemInfo=(TREEITEM_INFO*)pTree->GetItemData(hSelItem);
	if(pItemInfo==NULL || pItemInfo->itemType!=BOM_GROUP)
		return;
	BYTE ciMatCharPosType=1;	//0.ǰ��|1.����
	strcpy(BTN_ID_YES_TEXT, "����ǰ");
	strcpy(BTN_ID_NO_TEXT, "���ź�");
	strcpy(BTN_ID_CANCEL_TEXT, "�ر�");
	int nRetCode = MsgBox(adsw_acadMainWnd(), "��ѡ������м򻯲����ַ���λ�ã�", "��������", MB_YESNOCANCEL | MB_ICONQUESTION);
	if (nRetCode == IDYES)
		ciMatCharPosType = 0;
	else if (nRetCode == IDNO)
		ciMatCharPosType = 1;
	else
		return;
	CProjectTowerType* pProject=(CProjectTowerType*)GetProject(hSelItem);
	if (pProject == NULL)
		return;
	BOOL bErpBOM = pProject->ModifyErpBomPartNo(ciMatCharPosType);
	BOOL bTmaBOM = pProject->ModifyTmaBomPartNo(ciMatCharPosType);
	if (bErpBOM && bTmaBOM)
		AfxMessageBox("ERP�ϵ��������ϵ��������ݸ������!");
	else if (bErpBOM)
		AfxMessageBox("ERP�ϵ��������ݸ������!");
	else if (bTmaBOM)
	{
		AfxMessageBox("�����ϵ��������ݸ������!");
	}
	else
		return;
	//ˢ�½���
	HTREEITEM hChildItem = m_treeCtrl.GetChildItem(hSelItem);
	if (hChildItem != NULL)
		m_treeCtrl.SelectItem(hChildItem);
}

void CRevisionDlg::OnModifyTmaFile()
{
	CLogErrorLife logErrLife;
	CTreeCtrl* pTree = GetTreeCtrl();
	HTREEITEM hSelItem = pTree->GetSelectedItem();
	TREEITEM_INFO *pItemInfo = (TREEITEM_INFO*)pTree->GetItemData(hSelItem);
	if (pItemInfo == NULL || pItemInfo->itemType != BOM_GROUP)
		return;
	BYTE ciMatCharPosType = 1;	//0.ǰ��|1.����
	strcpy(BTN_ID_YES_TEXT, "����ǰ");
	strcpy(BTN_ID_NO_TEXT, "���ź�");
	strcpy(BTN_ID_CANCEL_TEXT, "�ر�");
	int nRetCode = MsgBox(adsw_acadMainWnd(), "��ѡ������м򻯲����ַ���λ�ã�", "��������", MB_YESNOCANCEL | MB_ICONQUESTION);
	if (nRetCode == IDYES)
		ciMatCharPosType = 1;
	else if (nRetCode == IDNO)
		ciMatCharPosType = 0;
	else
		return;
	CProjectTowerType* pProject = (CProjectTowerType*)GetProject(hSelItem);
	if (pProject->ModifyErpBomPartNo(ciMatCharPosType))
		AfxMessageBox("ERP�ϵ��м������ݸ������!");
}

void CRevisionDlg::OnRetrievedAngles()
{
	CLogErrorLife logErrLife;
	HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem();
	TREEITEM_INFO *pItemInfo;
	pItemInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelItem);
	if (pItemInfo == NULL || pItemInfo->itemType != ANGLE_DWG_ITEM)
		return;
	CDwgFileInfo* pDwgInfo = (CDwgFileInfo*)pItemInfo->dwRefData;
	if (pDwgInfo==NULL || !pDwgInfo->IsJgDwgInfo())
		return;
	pDwgInfo->ExtractDwgInfo(pDwgInfo->m_sFileName, TRUE);
}
void CRevisionDlg::OnRetrievedPlates()
{
	CLogErrorLife logErrLife;
	HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem();
	TREEITEM_INFO *pItemInfo;
	pItemInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelItem);
	if (pItemInfo == NULL || pItemInfo->itemType != PLATE_DWG_ITEM)
		return;
	CDwgFileInfo* pDwgInfo = (CDwgFileInfo*)pItemInfo->dwRefData;
	if (pDwgInfo == NULL || pDwgInfo->IsJgDwgInfo())
		return;
	pDwgInfo->ExtractDwgInfo(pDwgInfo->m_sFileName, FALSE);
}

void CRevisionDlg::OnDeleteItem()
{
	CLogErrorLife logErrLife;
	HTREEITEM hSelItem = m_treeCtrl.GetSelectedItem();
	TREEITEM_INFO *pItemInfo=NULL;
	pItemInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelItem);
	if (pItemInfo == NULL)
		return;
	CProjectTowerType *pPrjTowerType = NULL;
	if (pItemInfo->itemType == PLATE_DWG_ITEM || pItemInfo->itemType == ANGLE_DWG_ITEM)
	{
		CDwgFileInfo* pDwgInfo = (CDwgFileInfo*)pItemInfo->dwRefData;
		if (pDwgInfo == NULL)
			return;
		pPrjTowerType = pDwgInfo->BelongModel();
		if (pPrjTowerType == NULL)
			return;
		for (CDwgFileInfo *pTempDwgInfo = pPrjTowerType->dwgFileList.GetFirst(); pTempDwgInfo; pTempDwgInfo = pPrjTowerType->dwgFileList.GetNext())
		{
			if (pTempDwgInfo == pDwgInfo)
			{
				pPrjTowerType->dwgFileList.DeleteCursor(TRUE);
				m_treeCtrl.DeleteItem(hSelItem);	//ɾ��ѡ����
				break;
			}
		}
	}
	else if (pItemInfo->itemType == BOM_ITEM)
	{
		CBomFile *pBomFile = (CBomFile*)pItemInfo->dwRefData;
		if (pBomFile == NULL)
			return;
		pPrjTowerType = pBomFile->BelongModel();
		if (pPrjTowerType == NULL)
			return;
		if (pBomFile == &pPrjTowerType->m_xLoftBom || pBomFile == &pPrjTowerType->m_xOrigBom)
		{
			pBomFile->Empty();
			m_treeCtrl.DeleteItem(hSelItem);
		}
	}
	else if (pItemInfo->itemType == PROJECT_ITEM)
	{
		CProjectTowerType *pPrjTowerType = (CProjectTowerType*)pItemInfo->dwRefData;
		for (CProjectTowerType *pTemp = g_xUbomModel.m_xPrjTowerTypeList.GetFirst(); pTemp; pTemp = g_xUbomModel.m_xPrjTowerTypeList.GetNext())
		{
			if (pPrjTowerType == pTemp)
			{
				g_xUbomModel.m_xPrjTowerTypeList.DeleteCursor(TRUE);
				m_treeCtrl.DeleteItem(hSelItem);
			}
		}
	}
}
#endif