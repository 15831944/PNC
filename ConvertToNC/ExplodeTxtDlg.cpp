// ExplodeTxtDlg.cpp: ʵ���ļ�
//

#include "stdafx.h"
#include "ExplodeTxtDlg.h"
#include "folder_dialog.h"
#include "CadToolFunc.h"
#include "folder_dialog.h"
#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using std::vector;
// CExplodeTxtDlg �Ի���
#ifndef __UBOM_ONLY_

#ifdef __SUPPORT_DOCK_UI_
IMPLEMENT_DYNCREATE(CExplodeTxtDlg, CAcUiDialog)
#else
IMPLEMENT_DYNAMIC(CExplodeTxtDlg, CDialog)
#endif

#ifdef __SUPPORT_DOCK_UI_
CExplodeTxtDlg::CExplodeTxtDlg(CWnd* pParent /*=NULL*/)
	: CAcUiDialog(CExplodeTxtDlg::IDD, pParent)
#else
CExplodeTxtDlg::CExplodeTxtDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CExplodeTxtDlg::IDD, pParent)
#endif
{

}

CExplodeTxtDlg::~CExplodeTxtDlg()
{
}

void CExplodeTxtDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE_CTRL, m_treeCtrl);
}


BEGIN_MESSAGE_MAP(CExplodeTxtDlg, CDialog)
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_CTRL, OnTvnSelchangedTreeCtrl)
	ON_NOTIFY(NM_RCLICK, IDC_TREE_CTRL, OnRclickTreeCtrl)
	ON_COMMAND(ID_IMPORT_ITEM, OnImportItem)
	ON_COMMAND(ID_EXPLODE_ITEM, OnExplodeItem)
	ON_COMMAND(ID_SAVE_ITEM, OnSaveItem)
	ON_COMMAND(ID_CLOSE_ITEM, OnCloseItem)
	ON_MESSAGE(WM_ACAD_KEEPFOCUS, OnAcadKeepFocus)
END_MESSAGE_MAP()


// CExplodeTxtDlg ��Ϣ�������
BOOL CExplodeTxtDlg::OnInitDialog()
{
#ifdef __SUPPORT_DOCK_UI_
	CAcUiDialog::OnInitDialog();
#else
	CDialog::OnInitDialog();
#endif
	m_treeCtrl.ModifyStyle(0, TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS);
	m_treeCtrl.DeleteAllItems();
	m_hProjectItem = m_treeCtrl.InsertItem("�ӹ��ļ�", 0, 0, TVI_ROOT);
	TREEITEM_INFO* pInfo = itemInfoList.append(TREEITEM_INFO(TREEITEM_INFO::PROJECT_ITEM, 0));
	m_treeCtrl.SetItemData(m_hProjectItem, (DWORD)pInfo);
	for (CDxfFolder* pFolder = g_explodeModel.EnumFirst(); pFolder; pFolder = g_explodeModel.EnumNext())
		RefreshFolderItem(pFolder);
	//
	UpdateData(FALSE);
	return TRUE;
}
void CExplodeTxtDlg::OnOK()
{
}
void CExplodeTxtDlg::OnCancel()
{
}
void CExplodeTxtDlg::OnClose()
{
	DestroyWindow();
}
void CExplodeTxtDlg::PreSubclassWindow()
{
#ifdef __SUPPORT_DOCK_UI_
	ModifyStyle(DS_SETFONT | DS_MODALFRAME | WS_POPUP | WS_CAPTION, DS_SETFONT | WS_CHILD);
	CAcUiDialog::PreSubclassWindow();
#else
	//ModifyStyle(WS_CHILD, DS_SETFONT | DS_MODALFRAME | WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU);
	CDialog::PreSubclassWindow();
#endif
}
BOOL CExplodeTxtDlg::CreateDlg()
{
	return CDialog::Create(CExplodeTxtDlg::IDD);
}
void CExplodeTxtDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	if (m_treeCtrl.GetSafeHwnd())
	{
		RECT rectList;
		m_treeCtrl.GetWindowRect(&rectList);
		ScreenToClient(&rectList);
		m_treeCtrl.MoveWindow(0, rectList.top, cx, cy - rectList.top);
	}
}
void CExplodeTxtDlg::OnTvnSelchangedTreeCtrl(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	HTREEITEM hItem = m_treeCtrl.GetSelectedItem();
	if (hItem == NULL)
		return;
	TREEITEM_INFO *pItemInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(hItem);
	if (pItemInfo->itemType == TREEITEM_INFO::DXF_FILE_ITEM)
	{
		CDxfFolder::DXF_ITEM* pDxfItem = (CDxfFolder::DXF_ITEM*)pItemInfo->dwRefData;
		AcApDocument* pDoc = SearchDoc(pDxfItem->m_sFilePath);
		if (pDoc)
			acDocManager->activateDocument(pDoc);
	}
	*pResult = 0;
}
void CExplodeTxtDlg::OnRclickTreeCtrl(NMHDR* pNMHDR, LRESULT* pResult)
{
	TVHITTESTINFO HitTestInfo;
	GetCursorPos(&HitTestInfo.pt);
	m_treeCtrl.ScreenToClient(&HitTestInfo.pt);
	m_treeCtrl.HitTest(&HitTestInfo);
	m_treeCtrl.Select(HitTestInfo.hItem, TVGN_CARET);
	ContextMenu(this, HitTestInfo.pt);
	*pResult = 0;
}
void CExplodeTxtDlg::OnImportItem()
{
	HTREEITEM hSelectedItem = m_treeCtrl.GetSelectedItem();
	TREEITEM_INFO *pItemInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelectedItem);
	if (hSelectedItem==NULL || pItemInfo == NULL)
		return;
	CLogErrorLife logErrLife;
	if (pItemInfo->itemType == TREEITEM_INFO::PROJECT_ITEM)
	{	
		CString sDxfFolder;
		if (!InvokeFolderPickerDlg(sDxfFolder))
			return;
		CDxfFolder* pFolder = g_explodeModel.AppendFolder();
		pFolder->m_sFolderPath = sDxfFolder;
		pFolder->m_sFolderName.Printf("�ļ���%d", g_explodeModel.GetNum());
		//��ָ��·���²���DXF�ļ�
		CFileFind file_find;
		BOOL bFind = file_find.FindFile(CXhChar200("%s\\*.dxf", sDxfFolder));
		while (bFind)
		{
			bFind = file_find.FindNextFile();
			if (file_find.IsDots() || file_find.IsHidden() || file_find.IsReadOnly() ||
				file_find.IsSystem() || file_find.IsTemporary() || file_find.IsDirectory())
				continue;
			CString file_path = file_find.GetFilePath();
			CString str_ext = file_path.Right(4);	//ȡ��׺��
			str_ext.MakeLower();
			if (str_ext.CompareNoCase(".dxf") != 0)
				continue;
			CDxfFolder::DXF_ITEM dxf_item;
			dxf_item.m_sFilePath = file_path;
			dxf_item.m_sFileName.Copy(file_find.GetFileName());
			pFolder->m_xDxfFileSet.push_back(dxf_item);
		}
		file_find.Close();
		//��CAD�д���Щ�ļ�
		for (size_t i = 0; i < pFolder->m_xDxfFileSet.size(); i++)
		{
			//��ָ���ļ�
			CString sFileName = pFolder->m_xDxfFileSet[i].m_sFilePath;
			Acad::ErrorStatus eStatue;
#ifdef _ARX_2007
			eStatue = acDocManager->appContextOpenDocument(_bstr_t(sFileName.GetBuffer()));
#else
			eStatue = acDocManager->appContextOpenDocument((const char*)sFileName.GetBuffer());
#endif
			if (eStatue == Acad::eOk)
			{
				acDocManager->lockDocument(curDoc(), AcAp::kWrite);
#ifdef _ARX_2007
				acDocManager->sendStringToExecute(curDoc(), _bstr_t("txtexp\n"), false);
				acDocManager->sendStringToExecute(curDoc(), _bstr_t("all\n"), false);
				acDocManager->sendStringToExecute(curDoc(), _bstr_t("\n"), false);
#else
				acDocManager->sendStringToExecute(curDoc(), CXhChar200("txtexp\n"), false);
				acDocManager->sendStringToExecute(curDoc(), CXhChar200("all\n"), false);
				acDocManager->sendStringToExecute(curDoc(), CXhChar200("\n"), false);
#endif
				acDocManager->unlockDocument(curDoc());
			}
			else
				logerr.Log("%s��ʧ��!", sFileName.GetBuffer());
		}
		//ˢ�����б�
		RefreshFolderItem(pFolder);
	}
}
void CExplodeTxtDlg::OnExplodeItem()
{
	HTREEITEM hSelectedItem = m_treeCtrl.GetSelectedItem();
	TREEITEM_INFO *pItemInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelectedItem);
	if (hSelectedItem == NULL || pItemInfo == NULL)
		return;
	if (pItemInfo->itemType == TREEITEM_INFO::DXF_FOLDER)
	{
		HTREEITEM hCurItem = m_treeCtrl.GetChildItem(hSelectedItem);
		while (hCurItem)
		{
			m_treeCtrl.SelectItem(hCurItem);
			hCurItem = m_treeCtrl.GetNextSiblingItem(hCurItem);
		}
	}
}
void CExplodeTxtDlg::OnSaveItem()
{
	HTREEITEM hSelectedItem = m_treeCtrl.GetSelectedItem();
	TREEITEM_INFO *pItemInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelectedItem);
	if (hSelectedItem == NULL || pItemInfo == NULL)
		return;
	if (pItemInfo->itemType == TREEITEM_INFO::DXF_FOLDER)
	{
		CDxfFolder* pFolder = (CDxfFolder*)pItemInfo->dwRefData;
		CXhChar500 sWorkDir("%s�ֽ�", pFolder->m_sFolderPath);
		MakeDirectory(sWorkDir);
		for (size_t i = 0; i < pFolder->m_xDxfFileSet.size(); i++)
		{
			AcApDocument* pDoc = SearchDoc(pFolder->m_xDxfFileSet[i].m_sFilePath);
			if (pDoc == NULL)
				continue;
			//��ȡָ���ĵ�ʵ�弯��
			AcDbObjectIdArray entObjIds;
			GetAcadEntArr(pDoc, entObjIds);
			//���浽DXF�ļ���
			acDocManager->lockDocument(pDoc, AcAp::kWrite, NULL, NULL, true);
			CXhChar200 file_path("%s\\%s", (char*)sWorkDir, (char*)pFolder->m_xDxfFileSet[i].m_sFileName);
			AcDbDatabase* pOut = NULL;
			Acad::ErrorStatus es = pDoc->database()->wblock(pOut, entObjIds, AcGePoint3d(0, 0, 0));
			if (pOut)
			{
				pOut->setWorldview(true);
#ifdef _ARX_2007
				es = pOut->dxfOut(_bstr_t(file_path));
#else
				es = pOut->dxfOut(file_path);
#endif
				delete pOut;
			}
			acDocManager->unlockDocument(pDoc);
		}
		ShellExecute(NULL, "open", NULL, NULL, sWorkDir, SW_SHOW);
	}
}

void CExplodeTxtDlg::OnCloseItem()
{
	HTREEITEM hSelectedItem = m_treeCtrl.GetSelectedItem();
	TREEITEM_INFO *pItemInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(hSelectedItem);
	if (hSelectedItem == NULL || pItemInfo == NULL)
		return;
	if (pItemInfo->itemType == TREEITEM_INFO::DXF_FOLDER)
	{
		CDxfFolder* pFolder = (CDxfFolder*)pItemInfo->dwRefData;
		for (size_t i = 0; i < pFolder->m_xDxfFileSet.size(); i++)
		{
			AcApDocument* pDoc = SearchDoc(pFolder->m_xDxfFileSet[i].m_sFilePath);
			if (pDoc == NULL)
				continue;
			CloseDoc(pDoc);
		}
	}
	OnClose();
}

void CExplodeTxtDlg::ContextMenu(CWnd *pWnd, CPoint point)
{
	CPoint scr_point = point;
	m_treeCtrl.ClientToScreen(&scr_point);
	CMenu popMenu;
	popMenu.LoadMenu(IDR_ITEM_CMD_POPUP);
	CMenu *pMenu = popMenu.GetSubMenu(0);
	pMenu->DeleteMenu(0, MF_BYPOSITION);
	HTREEITEM hItem = m_treeCtrl.GetSelectedItem();
	if (hItem == NULL)
		return;
	TREEITEM_INFO *pItemInfo = (TREEITEM_INFO*)m_treeCtrl.GetItemData(hItem);
	if (pItemInfo->itemType == TREEITEM_INFO::PROJECT_ITEM)
		pMenu->AppendMenu(MF_STRING, ID_IMPORT_ITEM, "����");
	else if (pItemInfo->itemType == TREEITEM_INFO::DXF_FOLDER)
	{
		pMenu->AppendMenu(MF_STRING, ID_EXPLODE_ITEM, "���");
		pMenu->AppendMenu(MF_STRING, ID_SAVE_ITEM, "����");
		pMenu->AppendMenu(MF_STRING, ID_CLOSE_ITEM, "�ر�");
	}
	popMenu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, scr_point.x, scr_point.y, this);
}
void CExplodeTxtDlg::RefreshFolderItem(CDxfFolder* pDxfFolder)
{
	if (pDxfFolder == NULL)
		return;
	HTREEITEM hFolderItem = m_treeCtrl.InsertItem(pDxfFolder->m_sFolderName, 0, 0, m_hProjectItem);
	TREEITEM_INFO* pItemInfo = itemInfoList.append(TREEITEM_INFO(TREEITEM_INFO::DXF_FOLDER, (DWORD)pDxfFolder));
	m_treeCtrl.SetItemData(hFolderItem, (DWORD)pItemInfo);
	for (size_t i = 0; i < pDxfFolder->m_xDxfFileSet.size(); i++)
	{
		CDxfFolder::DXF_ITEM* pDxfItem = &pDxfFolder->m_xDxfFileSet[i];
		HTREEITEM hItem = m_treeCtrl.InsertItem(pDxfItem->m_sFileName, 0, 0, hFolderItem);
		pItemInfo = itemInfoList.append(TREEITEM_INFO(TREEITEM_INFO::DXF_FILE_ITEM, (DWORD)pDxfItem));
		m_treeCtrl.SetItemData(hItem, (DWORD)pItemInfo);
	}
	m_treeCtrl.Expand(hFolderItem, TVE_EXPAND);
	m_treeCtrl.SelectItem(hFolderItem);
}
LRESULT CExplodeTxtDlg::OnAcadKeepFocus(WPARAM, LPARAM)
{
	return(TRUE);
}
#endif
