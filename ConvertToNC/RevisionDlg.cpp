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
		CProcessPart *pPart = (CProcessPart*)pItem->m_idProp;
		CBomFile* pBom = (CBomFile*)pItemInfo->dwRefData;
		CProjectTowerType *pPrjTowerType = pBom?pBom->BelongModel():NULL;
		if (pPart&&pBom&&pPrjTowerType)
		{
			if (pPart->IsPlate())
			{
				CPlateProcessInfo *pPlateInfo=pPrjTowerType->FindPlateInfoByPartNo(pPart->GetPartNo());
				if (pPlateInfo)
				{
					scope = pPlateInfo->GetCADEntScope();
					bRetCode = TRUE;
				}
			}
			else if (pPart->IsAngle())
			{
				CAngleProcessInfo *pAngleInfo = pPrjTowerType->FindAngleInfoByPartNo(pPart->GetPartNo());
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
	ON_COMMAND(ID_REVISE_TEH_PLATE,OnReviseThePlate)
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
	m_xListReport.AddColumnHeader("����",75);	
	m_xListReport.AddColumnHeader("���",70);	
	m_xListReport.AddColumnHeader("����",50);	
	m_xListReport.AddColumnHeader("����",50);
	m_xListReport.AddColumnHeader("�ӹ���", 52);
	m_xListReport.AddColumnHeader("������", 52);
	m_xListReport.AddColumnHeader("��ע", 100);
	m_xListReport.AddColumnHeader("�ӹ�����", 65);
	m_xListReport.InitListCtrl();
	m_xListReport.EnableSortItems();
	m_xListReport.SetCompareItemFunc(FireCompareItem);
	m_xListReport.SetItemChangedFunc(FireItemChanged);
	m_xListReport.SetContextMenuFunc(FireContextMenu);
	RefreshListCtrl(NULL);
	//��ʼ�����б�
	m_imageList.Create(IDB_IL_PROJECT, 16, 1, RGB(0,255,0));
	m_treeCtrl.SetImageList(&m_imageList,TVSIL_NORMAL);
	m_treeCtrl.ModifyStyle(0,TVS_HASLINES|TVS_HASBUTTONS|TVS_LINESATROOT|TVS_SHOWSELALWAYS|TVS_FULLROWSELECT);
	RefreshTreeCtrl();
	//
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
	CProcessPart *pPart,CHashStrList<BOOL> *pHashBoolByPropName=NULL)
{
	COLORREF clr=RGB(230,100,230);
	CListCtrlItemInfo *lpInfo=new CListCtrlItemInfo();
	//����
	lpInfo->SetSubItemText(0,pPart->GetPartNo(),TRUE);
	//���
	lpInfo->SetSubItemText(1,pPart->GetSpec(FALSE),TRUE);
	if(pHashBoolByPropName&&pHashBoolByPropName->GetValue("spec"))
		lpInfo->SetSubItemColor(1,clr);
	//����
	lpInfo->SetSubItemText(2,CBomModel::QueryMatMarkIncQuality(pPart),TRUE);
	if(pHashBoolByPropName&&pHashBoolByPropName->GetValue("cMaterial"))
		lpInfo->SetSubItemColor(2,clr);
	//����
	lpInfo->SetSubItemText(3,CXhChar50("%d",pPart->m_wLength),TRUE);
	if(pHashBoolByPropName&&pHashBoolByPropName->GetValue("m_fLength"))
		lpInfo->SetSubItemColor(3,clr);
	//�ӹ���
	lpInfo->SetSubItemText(4, CXhChar50("%d", pPart->feature), TRUE);
	//������
	lpInfo->SetSubItemText(5, CXhChar50("%d", pPart->m_nDanJiNum), TRUE);
	if (pHashBoolByPropName&&pHashBoolByPropName->GetValue("m_nDanJiNum"))
		lpInfo->SetSubItemColor(5, clr);
	//��ע
	lpInfo->SetSubItemText(6, pPart->GetNotes(), TRUE);
	if (pHashBoolByPropName&&pHashBoolByPropName->GetValue("m_sNotes"))
		lpInfo->SetSubItemColor(5, clr);
	//�ӹ�����
	CXhChar50 sValue("%.f", pPart->m_fSumWeight);
	SimplifiedNumString(sValue);
	lpInfo->SetSubItemText(7, sValue, TRUE);
	if (pHashBoolByPropName&&pHashBoolByPropName->GetValue("m_fSumWeight"))
		lpInfo->SetSubItemColor(7, clr);
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
	if(!bCompared)
	{
		CXhChar100 sName;
		if(pInfo->itemType==BOM_ITEM)
		{
			m_xListReport.DeleteAllItems();
			CBomFile* pBom=(CBomFile*)pInfo->dwRefData;
			for(CProcessPart *pPart=pBom->EnumFirstPart();pPart;pPart=pBom->EnumNextPart())
			{
				CSuperGridCtrl::CTreeItem *pItem=InsertPartToList(m_xListReport,NULL,pPart,NULL);
				pItem->m_idProp=(long)pPart;
			}
			_splitpath(pBom->GetFileName(),NULL,NULL,sName,NULL);
			m_sCurFile.Format("%s",(char*)sName);
			m_sRecordNum=pBom->GetPartNumStr();
		}
		else if(pInfo->itemType==ANGLE_DWG_ITEM)
		{
			m_xListReport.DeleteAllItems();
			CDwgFileInfo* pDwg=(CDwgFileInfo*)pInfo->dwRefData;
			for(CAngleProcessInfo *pAngleInfo=pDwg->EnumFirstJg();pAngleInfo;pAngleInfo=pDwg->EnumNextJg())
			{
				CSuperGridCtrl::CTreeItem *pItem=InsertPartToList(m_xListReport,NULL,&pAngleInfo->m_xAngle,NULL);
				pItem->m_idProp=(long)pAngleInfo;
			}
			_splitpath(pDwg->GetFileName(),NULL,NULL,sName,NULL);
			m_sCurFile.Format("%s",(char*)sName);
			m_sRecordNum.Format("%d",pDwg->GetJgNum());
		}
		else if(pInfo->itemType==PLATE_DWG_ITEM)
		{
			m_xListReport.DeleteAllItems();
			CDwgFileInfo* pDwg=(CDwgFileInfo*)pInfo->dwRefData;
			for(CPlateProcessInfo *pPlateInfo=pDwg->EnumFirstPlate();pPlateInfo;pPlateInfo=pDwg->EnumNextPlate())
			{
				CSuperGridCtrl::CTreeItem *pItem=InsertPartToList(m_xListReport,NULL,&pPlateInfo->xPlate,NULL);
				pItem->m_idProp=(long)pPlateInfo;
			}
			_splitpath(pDwg->GetFileName(),NULL,NULL,sName,NULL);
			m_sCurFile.Format("%s",(char*)sName);
			m_sRecordNum.Format("%d", pDwg->GetPlateNum());
		}
	}
	else
	{
		m_xListReport.DeleteAllItems();
		long nSumCount=0;
		CProjectTowerType::COMPARE_PART_RESULT *pResult=NULL;
		CProjectTowerType* pProject=GetProject(hItem);
		m_sRecordNum.Format("%d", pProject->GetResultCount());
		CXhChar100 sName1,sName2;
		_splitpath(pProject->m_xLoftBom.GetFileName(),NULL,NULL,sName1,NULL);
		if(m_iCompareMode==CProjectTowerType::COMPARE_BOM_FILE)
		{
			for(CProcessPart *pPart=pProject->m_xOrigBom.EnumFirstPart();pPart;pPart=pProject->m_xOrigBom.EnumNextPart())
			{
				pResult=pProject->GetResult(pPart->GetPartNo());
				if(pResult==NULL)
					continue;	//����ʾ��ͬ����
				CSuperGridCtrl::CTreeItem *pItem=InsertPartToList(m_xListReport,NULL,pPart);
				pItem->m_idProp=(long)pPart;
				if(pResult->pLoftPart==NULL)
				{	//�����У�����û��
					pItem->m_bStrikeout=TRUE;
					pItem->SetBkColor(RGB(140,140,255));
				}
				else //���ݲ�һ��
					InsertPartToList(m_xListReport,pItem,pResult->pLoftPart,&pResult->hashBoolByPropName);
			}
			for(pResult=pProject->EnumFirstResult();pResult;pResult=pProject->EnumNextResult())
			{	//������в����ñ���ɫ
				if(pResult->pOrgPart)
					continue;
				CSuperGridCtrl::CTreeItem *pItem=InsertPartToList(m_xListReport,NULL,pResult->pLoftPart);
				pItem->SetBkColor(RGB(150,220,150));
			}
			_splitpath(pProject->m_xOrigBom.GetFileName(),NULL,NULL,sName2,NULL);
			m_sCurFile.Format("%s��%sУ��",(char*)sName1,(char*)sName2);
		}
		else if(m_iCompareMode==CProjectTowerType::COMPARE_ANGLE_DWG)
		{
			CDwgFileInfo* pJgDwg=(CDwgFileInfo*)pInfo->dwRefData;
			for(CAngleProcessInfo *pJgInfo=pJgDwg->EnumFirstJg();pJgInfo;pJgInfo=pJgDwg->EnumNextJg())
			{
				pResult=pProject->GetResult(pJgInfo->m_xAngle.GetPartNo());
				if(pResult==NULL)
					continue;	//����ʾ��ͬ����
				CSuperGridCtrl::CTreeItem *pItem=InsertPartToList(m_xListReport,NULL,&pJgInfo->m_xAngle,NULL);
				pItem->m_idProp=(long)pJgInfo;
				if(pResult->pLoftPart==NULL)
				{	//��������û�д˹���
					pItem->m_bStrikeout=TRUE;
					pItem->SetBkColor(RGB(140,140,255));
				}
				else
					InsertPartToList(m_xListReport,pItem,pResult->pLoftPart,&pResult->hashBoolByPropName);
			}
			//���û�ж�Ӧ���տ��ĽǸ��б� wht 19-08-14
			CProjectTowerType::COMPARE_PART_RESULT *pResult = NULL;
			for (pResult = pProject->EnumFirstResult(); pResult; pResult = pProject->EnumNextResult())
			{
				if (!(pResult->pLoftPart&&pResult->pOrgPart == NULL))
					continue;
				if (!pResult->pLoftPart->IsAngle())
					continue;	//�ų��ǽǸֶԱȽ��
				CSuperGridCtrl::CTreeItem *pItem = InsertPartToList(m_xListReport, NULL, pResult->pLoftPart, NULL);
				pItem->m_idProp = 0;
				pItem->SetBkColor(RGB(128, 128, 255));
			}
			_splitpath(pJgDwg->GetFileName(),NULL,NULL,sName2,NULL);
			m_sCurFile.Format("%s��%sУ��",(char*)sName1,(char*)sName2);
		}
		else if(m_iCompareMode==CProjectTowerType::COMPARE_PLATE_DWG)
		{
			CDwgFileInfo* pPlateDwg=(CDwgFileInfo*)pInfo->dwRefData;
			for(CPlateProcessInfo *pPlateInfo=pPlateDwg->EnumFirstPlate();pPlateInfo;pPlateInfo=pPlateDwg->EnumNextPlate())
			{
				pResult=pProject->GetResult(pPlateInfo->xPlate.GetPartNo());
				if(pResult==NULL)
					continue;	//����ʾ��ͬ����
				CSuperGridCtrl::CTreeItem *pItem=InsertPartToList(m_xListReport,NULL,&pPlateInfo->xPlate,NULL);
				pItem->m_idProp=(long)pPlateInfo;
				if(pResult->pLoftPart==NULL)
				{	//��������û�д˹���
					pItem->m_bStrikeout=TRUE;
					pItem->SetBkColor(RGB(140,140,255));
				}
				else
					InsertPartToList(m_xListReport,pItem,pResult->pLoftPart,&pResult->hashBoolByPropName);
			}
			//���û�ж�Ӧ������ĽǸ��б� wht 19-08-14
			CProjectTowerType::COMPARE_PART_RESULT *pResult = NULL;
			for (pResult = pProject->EnumFirstResult(); pResult; pResult = pProject->EnumNextResult())
			{
				if (!(pResult->pLoftPart&&pResult->pOrgPart == NULL))
					continue;
				if (!pResult->pLoftPart->IsPlate())
					continue;	//�ų��Ǹְ�ԱȽ��
				CSuperGridCtrl::CTreeItem *pItem = InsertPartToList(m_xListReport, NULL, pResult->pLoftPart, NULL);
				pItem->m_idProp = 0;
				pItem->SetBkColor(RGB(128, 128, 255));
			}
			_splitpath(pPlateDwg->GetFileName(),NULL,NULL,sName2,NULL);
			m_sCurFile.Format("%s��%sУ��",(char*)sName1,(char*)sName2);
		}
		else if(m_iCompareMode==CProjectTowerType::COMPARE_ANGLE_DWGS || 
			m_iCompareMode==CProjectTowerType::COMPARE_PLATE_DWGS)
		{
			if(m_iCompareMode==CProjectTowerType::COMPARE_ANGLE_DWGS)
				m_sCurFile="�Ѵ򿪽Ǹ�DWG�ļ�©�ż��";
			else
				m_sCurFile="�Ѵ򿪸ְ�DWG�ļ�©�ż��";
			for(pResult=pProject->EnumFirstResult();pResult;pResult=pProject->EnumNextResult())
			{	//������в����ñ���ɫ
				if(pResult->pOrgPart)
					continue;
				CSuperGridCtrl::CTreeItem *pItem=InsertPartToList(m_xListReport,NULL,pResult->pLoftPart);
				pItem->SetBkColor(RGB(150,220,150));
			}
		}
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
	CXhChar100 sName;
	hBomGroup=pTreeCtrl->InsertItem("�ϵ��ļ���",PRJ_IMG_FILEGROUP,PRJ_IMG_FILEGROUP,hProjItem);
	pItemInfo=itemInfoList.append(TREEITEM_INFO(BOM_GROUP,NULL));
	pTreeCtrl->SetItemData(hBomGroup,(DWORD)pItemInfo);
	if(pProject->m_xLoftBom.GetPartNum()>0)
	{
		_splitpath(pProject->m_xLoftBom.GetFileName(),NULL,NULL,sName,NULL);
		hSonItem=pTreeCtrl->InsertItem(sName,PRJ_IMG_FILE,PRJ_IMG_FILE,hBomGroup);
		pItemInfo=itemInfoList.append(TREEITEM_INFO(BOM_ITEM,(DWORD)&(pProject->m_xLoftBom)));
		pTreeCtrl->SetItemData(hSonItem,(DWORD)pItemInfo);
	}
	if(pProject->m_xOrigBom.GetPartNum()>0)
	{
		_splitpath(pProject->m_xOrigBom.GetFileName(),NULL,NULL,sName,NULL);
		hSonItem=pTreeCtrl->InsertItem(sName,PRJ_IMG_FILE,PRJ_IMG_FILE,hBomGroup);
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
			_splitpath(pDwgInfo->GetFileName(),NULL,NULL,sName,NULL);
			hSonItem=pTreeCtrl->InsertItem(sName,PRJ_IMG_FILE,PRJ_IMG_FILE,hJgGroup);
			pItemInfo=itemInfoList.append(TREEITEM_INFO(ANGLE_DWG_ITEM,(DWORD)pDwgInfo));
			pTreeCtrl->SetItemData(hSonItem,(DWORD)pItemInfo);
		}
		else
		{
			_splitpath(pDwgInfo->GetFileName(),NULL,NULL,sName,NULL);
			hSonItem=pTreeCtrl->InsertItem(sName,PRJ_IMG_FILE,PRJ_IMG_FILE,hPlateGroup);
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
		pMenu->AppendMenu(MF_STRING,ID_MODIFY_ERP_FILE,"����BOM����");
		//pMenu->AppendMenu(MF_STRING, ID_MODIFY_TMA_FILE, "������������");
		pMenu->AppendMenu(MF_STRING,ID_COMPARE_DATA,"�ϵ�����У��");
		pMenu->AppendMenu(MF_STRING,ID_EXPORT_COMPARE_RESULT,"����У����");
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
		pMenu->AppendMenu(MF_STRING,ID_REFRESH_PART_NUM,"���¼ӹ���");
		pMenu->AppendMenu(MF_SEPARATOR);
		pMenu->AppendMenu(MF_STRING, ID_RETRIEVED_ANGLES, "������ȡ�Ǹ�");
		pMenu->AppendMenu(MF_STRING, ID_DELETE_ITEM, "ɾ���Ǹ�ͼֽ");
	}
	else if(pItemInfo->itemType==PLATE_DWG_ITEM)
	{
		pMenu->AppendMenu(MF_STRING,ID_COMPARE_DATA,"�ְ�����У��");
		pMenu->AppendMenu(MF_STRING,ID_EXPORT_COMPARE_RESULT,"����У����");
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
			if(strstr(file_path,pDwgInfo->GetFileName()))
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
	dlg.m_ofn.lpstrTitle="ѡ��TMA���������嵥��������ERP�����嵥";
	if(dlg.DoModal()!=IDOK)
		return;
	CWaitCursor waitCursor;
	CProjectTowerType* pProject=GetProject(hSelectedItem);
	POSITION pos=dlg.GetStartPosition();
	int nFileCount = 0;
	BOOL bFindTmaBom = FALSE, bFindErpBom = FALSE;
	CString sFirstFilePath;
	while(pos)
	{
		nFileCount++;
		CString sFilePath=dlg.GetNextPathName(pos);//��ȡ�ļ��� 
		char sFileName[MAX_PATH]="";
		_splitpath(sFilePath, NULL, NULL, sFileName, NULL);
		if (nFileCount == 1)
			sFirstFilePath = sFilePath;
		if (strstr(sFileName, "TMA") || strstr(sFileName, "tma"))
		{
			pProject->InitBomInfo(sFilePath, TRUE);
			bFindTmaBom = TRUE;
		}
		else if (strstr(sFileName, "ERP") || strstr(sFileName, "erp"))
		{
			pProject->InitBomInfo(sFilePath, FALSE);
			bFindErpBom = TRUE;
		}
	}
	if (nFileCount == 1 && bFindErpBom == FALSE && bFindTmaBom == FALSE)
		pProject->InitBomInfo(sFirstFilePath,TRUE);
	//���BOM���ڵ�
	CXhChar100 sName;
	pTreeCtrl->Expand(hSelectedItem,TVE_EXPAND);
	if(pProject->m_xLoftBom.GetPartNum()>0)
	{
		_splitpath(pProject->m_xLoftBom.GetFileName(),NULL,NULL,sName,NULL);
		hItem=FindTreeItem(hSelectedItem,sName);
		if(hItem)
			pTreeCtrl->DeleteItem(hItem);
		hItem=m_treeCtrl.InsertItem(sName,PRJ_IMG_FILE,PRJ_IMG_FILE,hSelectedItem);
		pItemInfo=itemInfoList.append(TREEITEM_INFO(BOM_ITEM,(DWORD)&(pProject->m_xLoftBom)));
		m_treeCtrl.SetItemData(hItem,(DWORD)pItemInfo);
		m_treeCtrl.SelectItem(hItem);
	}
	else 
		logerr.Log("���ط��������嵥ʧ��!");
	//
	if(pProject->m_xOrigBom.GetPartNum()>0)
	{
		_splitpath(pProject->m_xOrigBom.GetFileName(),NULL,NULL,sName,NULL);
		hItem=FindTreeItem(hSelectedItem,sName);
		if(hItem)
			pTreeCtrl->DeleteItem(hItem);
		hItem=m_treeCtrl.InsertItem(sName,PRJ_IMG_FILE,PRJ_IMG_FILE,hSelectedItem);
		pItemInfo=itemInfoList.append(TREEITEM_INFO(BOM_ITEM,(DWORD)&(pProject->m_xOrigBom)));
		m_treeCtrl.SetItemData(hItem,(DWORD)pItemInfo);
		m_treeCtrl.SelectItem(hItem);
	}
	else if(nFileCount>1 && bFindErpBom)
		logerr.Log("����ERP�����嵥ʧ��!");
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
	CXhChar100 sName;
	_splitpath(pJgDwgInfo->GetFileName(),NULL,NULL,sName,NULL);
	hItem=FindTreeItem(hSelectedItem,sName);
	if(hItem==NULL)
	{
		hItem=m_treeCtrl.InsertItem(sName,PRJ_IMG_FILE,PRJ_IMG_FILE,hSelectedItem);
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
	CXhChar100 sName;
	_splitpath(pPlateDwgInfo->GetFileName(),NULL,NULL,sName,NULL);
	hItem=FindTreeItem(hSelectedItem,sName);
	if(hItem==NULL)
	{
		hItem=m_treeCtrl.InsertItem(sName,PRJ_IMG_FILE,PRJ_IMG_FILE,hSelectedItem);
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
		if(pProject->CompareLoftAndAngleDwg(pDwgInfo->GetFileName())!=1)
			return;
	}
	else if(pItemInfo->itemType==PLATE_DWG_ITEM)
	{	//����TMA�ϵ��͸ְ�DWG�Ա�
		m_iCompareMode=CProjectTowerType::COMPARE_PLATE_DWG;
		CDwgFileInfo* pDwgInfo=(CDwgFileInfo*)pItemInfo->dwRefData;
		if(pProject->CompareLoftAndPlateDwg(pDwgInfo->GetFileName())!=1)
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

//
void CRevisionDlg::OnReviseThePlate()
{
	CLogErrorLife logErrLife;
	HTREEITEM hSelItem=m_treeCtrl.GetSelectedItem();
	TREEITEM_INFO *pItemInfo;
	pItemInfo=(TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelItem);
	if(pItemInfo==NULL || pItemInfo->itemType!=PLATE_DWG_ITEM)
		return;
	CDwgFileInfo* pDwgInfo=(CDwgFileInfo*)pItemInfo->dwRefData;
	if(pDwgInfo->IsJgDwgInfo())
		return;
	int iSelIndex=m_xListReport.GetSelectedItem();
	CSuperGridCtrl::CTreeItem* pSelItem=m_xListReport.GetTreeItem(iSelIndex);
	CPlateProcessInfo* pPlate=(CPlateProcessInfo*)pSelItem->m_idProp;
	if(pDwgInfo->ReviseThePlate(pPlate->xPlate.GetPartNo()))
	{
		m_xListReport.SetSubItemText(pSelItem,0,pPlate->xPlate.GetPartNo());
		m_xListReport.SetSubItemText(pSelItem,1,pPlate->xPlate.GetSpec(FALSE));
		m_xListReport.SetSubItemText(pSelItem,2,CBomModel::QueryMatMarkIncQuality(&pPlate->xPlate));
		m_xListReport.SetSubItemText(pSelItem,6,CXhChar50("%d",pPlate->xPlate.feature));
	}
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
	pDwgInfo->InitDwgInfo(pDwgInfo->GetFileName(), TRUE);
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
	pDwgInfo->InitDwgInfo(pDwgInfo->GetFileName(), FALSE);
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