// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "PPEView.h"
#include "PPE.h"
#include "MainFrm.h"
#include "SideBaffleHighDlg.h"
#include "NcSetDlg.h"
#include "ProcessPart.h"
#include "SysPara.h"
#include "PromptDlg.h"
#include "NcPart.h"
#include "PartTreeDlg.h"
#include "PartPropertyDlg.h"
#include "LicFuncDef.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame
IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx)
const int  iMaxUserToolbars = 10;
const UINT uiFirstUserToolBarId = AFX_IDW_CONTROLBAR_FIRST + 40;
const UINT uiLastUserToolBarId = uiFirstUserToolBarId + iMaxUserToolbars - 1;

BYTE GetCurPartType()
{	//�õ���ǰ��������
	CPPEView *pView=theApp.GetView();
	CProcessPart *pProcessPart=pView->GetCurProcessPart();
	if(pProcessPart)
		return pProcessPart->m_cPartType;
	else 
		return 0;
}

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_COMMAND(ID_VIEW_CUSTOMIZE, &CMainFrame::OnViewCustomize)
	ON_REGISTERED_MESSAGE(AFX_WM_CREATETOOLBAR, &CMainFrame::OnToolbarCreateNew)
	ON_COMMAND_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &CMainFrame::OnApplicationLook)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &CMainFrame::OnUpdateApplicationLook)
	ON_WM_SETTINGCHANGE()
	ON_COMMAND(ID_PREV_PART, OnPrevPart)
	ON_COMMAND(ID_ROTATE_ANTI_CLOCKWISE, OnRotateAntiClockwise)
	ON_COMMAND(ID_NEXT_PART, OnNextPart)
	ON_COMMAND(ID_ROTATE_CLOCKWISE, OnRotateClockwise)
	ON_COMMAND(ID_OVERTURN_PLATE, OnOverturnPlate)
	ON_COMMAND(ID_FIRST_PART, OnFirstPart)
	ON_COMMAND(ID_FINAL_PART, OnFinalPart)
	ON_COMMAND(ID_DRAW_MODE_NC, &CMainFrame::OnDrawModeNc)
	ON_COMMAND(ID_DRAW_MODE_EDIT, &CMainFrame::OnDrawModeEdit)
	ON_COMMAND(ID_DRAW_MODE_PROCESSCARD, &CMainFrame::OnDrawModeProcesscard)
	ON_UPDATE_COMMAND_UI(ID_DRAW_MODE_NC, &CMainFrame::OnUpdateDrawModeNc)
	ON_UPDATE_COMMAND_UI(ID_DRAW_MODE_EDIT, &CMainFrame::OnUpdateDrawModeEdit)
	ON_UPDATE_COMMAND_UI(ID_DRAW_MODE_PROCESSCARD, &CMainFrame::OnUpdateDrawModeProcesscard)
	ON_UPDATE_COMMAND_UI(ID_ROTATE_ANTI_CLOCKWISE, &CMainFrame::OnUpdateRotateAntiClockwise)
	ON_UPDATE_COMMAND_UI(ID_ROTATE_CLOCKWISE, &CMainFrame::OnUpdateRotateClockwise)
	ON_UPDATE_COMMAND_UI(ID_OVERTURN_PLATE, &CMainFrame::OnUpdateOverturnPlate)
	ON_COMMAND(ID_INSERT_VERTEX, &CMainFrame::OnInsertVertex)
	ON_UPDATE_COMMAND_UI(ID_INSERT_VERTEX, &CMainFrame::OnUpdateInsertVertex)
	ON_COMMAND(ID_NEW_LS, &CMainFrame::OnNewLs)
	ON_UPDATE_COMMAND_UI(ID_NEW_LS, &CMainFrame::OnUpdateNewLs)
	ON_COMMAND(ID_FEATURE_PROP, &CMainFrame::OnFeatureProp)
	ON_UPDATE_COMMAND_UI(ID_FEATURE_PROP, &CMainFrame::OnUpdateFeatureProp)
	ON_COMMAND(ID_SPEC_WCS_ORGIN, &CMainFrame::OnSpecWcsOrgin)
	ON_UPDATE_COMMAND_UI(ID_SPEC_WCS_ORGIN, &CMainFrame::OnUpdateSpecWcsOrgin)
	ON_COMMAND(ID_SPEC_AXIS_X_VERTEX, &CMainFrame::OnSpecAxisXVertex)
	ON_UPDATE_COMMAND_UI(ID_SPEC_AXIS_X_VERTEX, &CMainFrame::OnUpdateSpecAxisXVertex)
	ON_COMMAND(ID_ASSEMBLE_CS, &CMainFrame::OnAssembleCs)
	ON_COMMAND(ID_WORK_CS, &CMainFrame::OnWorkCs)
	ON_COMMAND(ID_DEL_PART_FEATURE, &CMainFrame::OnDelPartFeature)
	ON_UPDATE_COMMAND_UI(ID_DEL_PART_FEATURE, &CMainFrame::OnUpdateDelPartFeature)
	ON_COMMAND(ID_CAL_PLATE_PROFILE, &CMainFrame::OnCalPlateProfile)
	ON_UPDATE_COMMAND_UI(ID_CAL_PLATE_PROFILE, &CMainFrame::OnUpdateCalPlateProfile)
	ON_UPDATE_COMMAND_UI(ID_NEW_LS, &CMainFrame::OnUpdateNewLs)
	ON_COMMAND(ID_GL_ALL_ZOOM, &CMainFrame::OnGlAllZoom)
	ON_COMMAND(ID_VIEW_PROP_LIST, &CMainFrame::OnViewPropList)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PROP_LIST, &CMainFrame::OnUpdateViewPropList)
	ON_COMMAND(ID_VIEW_PART_LIST, &CMainFrame::OnViewPartList)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PART_LIST, &CMainFrame::OnUpdateViewPartList)
	ON_COMMAND(ID_KAIHE_JG,&CMainFrame::OnKaiHeJG)
	ON_UPDATE_COMMAND_UI(ID_KAIHE_JG, &CMainFrame::OnUpdateKaiHeJG)
	ON_COMMAND(ID_MEASURE_DIST,&CMainFrame::OnMeasureDist)
	ON_COMMAND(ID_AMEND_HOLE_INCREMENT,&CMainFrame::OnAmendHoleIncrement)
	ON_UPDATE_COMMAND_UI(ID_AMEND_HOLE_INCREMENT, &CMainFrame::OnUpdateAmendHoleIncrement)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_MOUSE,     // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};
/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	theApp.m_nAppLook = ID_VIEW_APPLOOK_WINDOWS_7;	//theApp.GetInt(_T("ApplicationLook"), ID_VIEW_APPLOOK_OFF_2007_BLUE);
	m_partPropertyView.Init(RUNTIME_CLASS(CPartPropertyDlg), IDD_PART_PROPERTY_DLG);
	m_partTreeView.Init(RUNTIME_CLASS(CPartTreeDlg), IDD_PART_TREE_DLG);
}

CMainFrame::~CMainFrame()
{
	
}
void CMainFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
{
	CString app_name="���չ�����";
#if defined(__PNC_)
	app_name.Append("-PNC");
#else
	if(theApp.starter.m_cProductType==PRODUCT_TMA)
		app_name.Append("-TMA");
	else if(theApp.starter.m_cProductType==PRODUCT_LMA)
		app_name.Append("-LMA");
	else if(theApp.starter.m_cProductType==PRODUCT_TAP)
		app_name.Append("-TAP");
#endif
	SetWindowText(app_name);
}

void CMainFrame::ModifyDockpageStatus(CRuntimeClass *pRuntimeClass, BOOL bShow)
{
	if(m_partPropertyView.GetDlgPtr()->IsKindOf(pRuntimeClass))
		m_partPropertyView.ShowPane(bShow,FALSE,TRUE);
	else if(m_partTreeView.GetDlgPtr()->IsKindOf(pRuntimeClass))
		m_partTreeView.ShowPane(bShow,FALSE,TRUE);
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
		return -1;
	//���ڳ־�ֵ�����Ӿ�����������ʽ
	OnApplicationLook(theApp.m_nAppLook);
	if (!m_wndMenuBar.Create(this,AFX_DEFAULT_TOOLBAR_STYLE,theApp.m_nMainMenuID))
	{
		TRACE0("δ�ܴ����˵���\n");
		return -1;      // δ�ܴ���
	}
	m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_FIXED | CBRS_TOOLTIPS | CBRS_TOP);
	// ��ֹ�˵����ڼ���ʱ��ý���
	CMFCPopupMenu::SetForceMenuFocus(FALSE);

	CXhPtrSet<CPPEToolBar> toolBarPtrSet;
	if(!CreateToolBar(this,m_wndToolBar,theApp.m_nMainMenuID,"��׼",TRUE))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}
	else
		toolBarPtrSet.append(&m_wndToolBar);
	if(CreateToolBar(this,m_wndToolBarFile,IDR_TOOLBAR_FILE,"�ļ�",TRUE))
		toolBarPtrSet.append(&m_wndToolBarFile);
	if(theApp.starter.IsSupportPlateEditing())
	{
		if(CreateToolBar(this,m_wndToolBarEdit,IDR_TOOLBAR_EDIT,"�༭",TRUE))
			toolBarPtrSet.append(&m_wndToolBarEdit);
		if(CreateToolBar(this,m_wndToolBarPlate,IDR_TOOLBAR_PLATE,"�ְ�NC",TRUE))
			toolBarPtrSet.append(&m_wndToolBarPlate);
		if(CreateToolBar(this,m_wndToolBarCutPlate,IDR_TOOLBAR_CUT_PLATE,"�ְ��и�",TRUE))
			toolBarPtrSet.append(&m_wndToolBarCutPlate);
		if(CreateToolBar(this,m_wndToolBarPBJ,IDR_TOOLBAR_PBJ,"�ְ����",TRUE))
			toolBarPtrSet.append(&m_wndToolBarPBJ);
	}
	if(!CreateToolBar(this,m_wndToolBarToolKit,IDR_TOOLBAR_TOOLKIT,"����",TRUE))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}
	else
		toolBarPtrSet.append(&m_wndToolBarToolKit);

	// �����û�����Ĺ���������:
	InitUserToolbars(NULL, uiFirstUserToolBarId, uiLastUserToolBarId);

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	m_wndStatusBar.SetPaneInfo(1,ID_INDICATOR_MOUSE,SBPS_NORMAL,150);

	HMENU hMenu=m_wndMenuBar.GetHMenu();
	if(!theApp.starter.IsSupportPlateEditing())
	{
		::DeleteMenu(hMenu,ID_INSERT_VERTEX,MF_BYCOMMAND);
		::DeleteMenu(hMenu,ID_CAL_PLATE_PROFILE,MF_BYCOMMAND);
	}
	if(!theApp.starter.IsMultiPartsMode())
	{	//ɾ�������л��˵�����������ť
		::DeleteMenu(hMenu,ID_FIRST_PART,MF_BYCOMMAND);
		::DeleteMenu(hMenu,ID_PREV_PART,MF_BYCOMMAND);
		::DeleteMenu(hMenu,ID_NEXT_PART,MF_BYCOMMAND);
		::DeleteMenu(hMenu,ID_FINAL_PART,MF_BYCOMMAND);
		//ɾ�������б���ͼ�˵�
		::DeleteMenu(hMenu,ID_VIEW_PART_LIST,MF_BYCOMMAND);
	}
#ifdef __PNC_	
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_PBJ_FILE))
	{
		::DeleteMenu(hMenu,ID_DISPLAY_BOLT_SORT,MF_BYCOMMAND);
		::DeleteMenu(hMenu,ID_SMART_SORT_BOLT,MF_BYCOMMAND);
		::DeleteMenu(hMenu,ID_BATCH_SORT_HOLE,MF_BYCOMMAND);
		::DeleteMenu(hMenu,ID_ADJUST_HOLE_ORDER,MF_BYCOMMAND);
		::DeleteMenu(hMenu,ID_FILE_SAVE_PBJ,MF_BYCOMMAND);
		::DeleteMenu(hMenu,ID_FILE_SAVE_PMZ,MF_BYCOMMAND);
	}
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))

	{
		::DeleteMenu(hMenu,ID_DEF_CUT_IN_PT,MF_BYCOMMAND);
		::DeleteMenu(hMenu,ID_PREVIEW_CUTTING_TRACK,MF_BYCOMMAND);
		::DeleteMenu(hMenu,ID_FILE_SAVE_NC,MF_BYCOMMAND);
		::DeleteMenu(hMenu,ID_FILE_SAVE_TXT,MF_BYCOMMAND);
		::DeleteMenu(hMenu,ID_FILE_SAVE_CNC,MF_BYCOMMAND);
	}
#endif	
	/*if(!theApp.starter.IsDuplexMode() && theApp.starter.mode!=0)
		pMenu->ModifyMenu(ID_FILE_SAVE_DXF,MF_BYCOMMAND);*/
	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
	AddPane(&m_wndMenuBar);	//ͣ���˵�
	for(CPPEToolBar *pBar=toolBarPtrSet.GetFirst();pBar;pBar=toolBarPtrSet.GetNext())
		pBar->EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	for(CPPEToolBar *pBar=toolBarPtrSet.GetFirst();pBar;pBar=toolBarPtrSet.GetNext())
		DockPane(pBar);
	CPPEToolBar *pPrevBar=toolBarPtrSet.GetTail();
	for(CPPEToolBar *pBar=toolBarPtrSet.GetPrev();pBar;pBar=toolBarPtrSet.GetPrev())
	{
		DockPaneLeftOf(pBar,pPrevBar);
		pPrevBar=pBar;
	}
	
	// ���� Visual Studio 2005 ��ʽͣ��������Ϊ
	CDockingManager::SetDockingMode(DT_SMART);
	// ���� Visual Studio 2005 ��ʽͣ�������Զ�������Ϊ
	EnableAutoHidePanes(CBRS_ALIGN_ANY);

	// ����ͣ������
	if (!CreateDockingWindows())
	{
		TRACE0("δ�ܴ���ͣ������\n");
		return -1;
	}
	
	m_partPropertyView.EnableDocking(CBRS_ALIGN_LEFT);
	DockPane(&m_partPropertyView);
	m_partTreeView.EnableDocking(CBRS_ALIGN_RIGHT);
	DockPane(&m_partTreeView);

	// ���ù�������ͣ�����ڲ˵��滻
	CString strCustomize;
	strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	EnablePaneMenu(TRUE, ID_VIEW_CUSTOMIZE, strCustomize, ID_VIEW_TOOLBAR);

	// ���ÿ���(��ס Alt �϶�)�������Զ���
	CMFCToolBar::EnableQuickCustomization();

	if (CMFCToolBar::GetUserImages() == NULL)
	{
		// �����û�����Ĺ�����ͼ��
		if (m_userImages.Load(_T(".\\res\\UserImages.bmp")))
		{
			CMFCToolBar::SetUserImages(&m_userImages);
		}
	}
	//������������
	/*LOGFONT logfont = {0};
	:: SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &logfont, 0);
	afxGlobalData.SetMenuFont(&logfont,true);*/
	LOGFONT lf;
	afxGlobalData.fontRegular.GetLogFont(&lf);   
	lf.lfHeight =-12;   
	lstrcpy(lf.lfFaceName, _T("����"));     // using without style office 2007   
	afxGlobalData.fontRegular.DeleteObject();   
	afxGlobalData.fontRegular.CreateFontIndirect(&lf); 
	//�ڱ������м���ָ��ͼ��
	HICON hIco;
#if defined(__PNC_)
	hIco=AfxGetApp()->LoadIcon(IDR_MAINFRAME_PNC);
#else
	hIco=AfxGetApp()->LoadIcon(IDR_MAINFRAME);
#endif
	//SetIcon(hIco,TRUE);
	SetIcon(hIco,FALSE);
	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWndEx::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

static BOOL CreateDockingWindow(CWnd *pParentWnd,UINT nDlgID,UINT nViewNameID,CDialogPanel &dlgPanel,
	DWORD dwPosStyle,int nWidth=200,int nHeight=200)
{
	CString sViewName="";
	BOOL bNameValid = sViewName.LoadString(nViewNameID);
	ASSERT(bNameValid);
	if (!dlgPanel.Create(sViewName, pParentWnd, CRect(0, 0, nWidth, nHeight), TRUE, nDlgID,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | dwPosStyle | CBRS_FLOAT_MULTI))
	{
		TRACE0("δ�ܴ�����"+sViewName+"������\n");
		return FALSE;
	}
	return TRUE;
}

static void SetDockingWindowIcon(CDialogPanel &dlgPanel,UINT nIdHC,UINT nCommonId,BOOL bHiColorIcons)
{
	HICON hViewIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), 
		MAKEINTRESOURCE(bHiColorIcons ? nIdHC : nCommonId), 
		IMAGE_ICON,::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
	dlgPanel.SetIcon(hViewIcon, FALSE);
}

BOOL CMainFrame::CreateDockingWindows()
{
	CreateDockingWindow(this,IDD_PART_TREE_DLG,IDS_PART_TREE_VIEW,m_partTreeView,CBRS_RIGHT,300);
	//SetDockingWindowIcon(m_towerPropertyView,ID_DD,ID_DD2,theApp.m_bHiColorIcons);
	CreateDockingWindow(this,IDD_PART_PROPERTY_DLG,IDS_PART_PROP_VIEW,m_partPropertyView,CBRS_LEFT);
	//SetDockingWindowIcon(m_towerPropertyView,ID_DD,ID_DD2,theApp.m_bHiColorIcons);
	return TRUE;
}
/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWndEx::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers
void CMainFrame::OnViewCustomize()
{
	CMFCToolBarsCustomizeDialog* pDlgCust = new CMFCToolBarsCustomizeDialog(this, TRUE /* ɨ��˵�*/);
	pDlgCust->EnableUserDefinedToolbars();
	pDlgCust->Create();
}

LRESULT CMainFrame::OnToolbarCreateNew(WPARAM wp,LPARAM lp)
{
	LRESULT lres = CFrameWndEx::OnToolbarCreateNew(wp,lp);
	if (lres == 0)
	{
		return 0;
	}

	CMFCToolBar* pUserToolbar = (CMFCToolBar*)lres;
	ASSERT_VALID(pUserToolbar);

	BOOL bNameValid;
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);

	pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
	return lres;
}

void CMainFrame::OnApplicationLook(UINT id)
{
	CWaitCursor wait;

	theApp.m_nAppLook = id;

	switch (theApp.m_nAppLook)
	{
	case ID_VIEW_APPLOOK_WIN_2000:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManager));
		break;

	case ID_VIEW_APPLOOK_OFF_XP:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOfficeXP));
		break;

	case ID_VIEW_APPLOOK_WIN_XP:
		CMFCVisualManagerWindows::m_b3DTabsXPTheme = TRUE;
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
		break;

	case ID_VIEW_APPLOOK_OFF_2003:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2003));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case ID_VIEW_APPLOOK_VS_2005:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2005));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case ID_VIEW_APPLOOK_VS_2008:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2008));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case ID_VIEW_APPLOOK_WINDOWS_7:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows7));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	default:
		switch (theApp.m_nAppLook)
		{
		case ID_VIEW_APPLOOK_OFF_2007_BLUE:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_LunaBlue);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_BLACK:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_ObsidianBlack);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_SILVER:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Silver);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_AQUA:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Aqua);
			break;
		}

		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));
		CDockingManager::SetDockingMode(DT_SMART);
	}

	RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);

	theApp.WriteInt(_T("ApplicationLook"), theApp.m_nAppLook);
}

void CMainFrame::OnUpdateApplicationLook(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(theApp.m_nAppLook == pCmdUI->m_nID);
}

BOOL CMainFrame::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext) 
{
	// ���ཫִ�������Ĺ���
	if (!CFrameWndEx::LoadFrame(nIDResource, dwDefaultStyle, pParentWnd, pContext))
	{
		return FALSE;
	}
	// Ϊ�����û������������Զ��尴ť
	BOOL bNameValid;
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);

	for (int i = 0; i < iMaxUserToolbars; i ++)
	{
		CMFCToolBar* pUserToolbar = GetUserToolBarByIndex(i);
		if (pUserToolbar != NULL)
		{
			pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
		}
	}
	return TRUE;
}
void CMainFrame::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CFrameWndEx::OnSettingChange(uFlags, lpszSection);
}
void CMainFrame::CloseProcess()
{
	CFrameWndEx::OnClose();
}
void CMainFrame::OnClose()
{
	if(theApp.starter.IsDuplexMode())
	{
		HANDLE hPipeWrite=GetStdHandle(STD_OUTPUT_HANDLE);
		BYTE byteArr[2]={0,0};	//������ֹ��ʶ
		DWORD dwWrite;
		WriteFile(hPipeWrite,&byteArr,2,&dwWrite,NULL);
	}
	//���������ļ�
	char cfg_file[MAX_PATH],APP_PATH[MAX_PATH];
	GetSysPath(APP_PATH);
	sprintf(cfg_file,"%s\\PPE.cfg",APP_PATH);
	g_sysPara.Write(cfg_file);
	CFrameWndEx::OnClose();
}
BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
	return CFrameWndEx::OnCreateClient(lpcs, pContext);
}

void CMainFrame::SetPaneText(int nIndex, LPCTSTR lpszNewText, BOOL bUpdate)
{
	m_wndStatusBar.SetPaneText(nIndex,lpszNewText,bUpdate);
}
//��һ������
void CMainFrame::OnFirstPart() 
{
	CPartTreeDlg *pPartTreeDlg=GetPartTreePage();
	if(pPartTreeDlg)
		pPartTreeDlg->JumpSelectItem(0);
}
//ǰһ������
void CMainFrame::OnPrevPart() 
{
	CPartTreeDlg *pPartTreeDlg=GetPartTreePage();
	if(pPartTreeDlg)
		pPartTreeDlg->JumpSelectItem(1);
}
//��һ������
void CMainFrame::OnNextPart() 
{
	CPartTreeDlg *pPartTreeDlg=GetPartTreePage();
	if(pPartTreeDlg)
		pPartTreeDlg->JumpSelectItem(2);
}
//���һ������
void CMainFrame::OnFinalPart() 
{
	CPartTreeDlg *pPartTreeDlg=GetPartTreePage();
	if(pPartTreeDlg)
		pPartTreeDlg->JumpSelectItem(3);
}
//�ְ����:��ʱ����ת
void CMainFrame::OnRotateAntiClockwise() 
{
	if(GetCurPartType()!=CProcessPart::TYPE_PLATE)
		return;
	CPPEView *pView=theApp.GetView();
	CProcessPlate* pPlate=(CProcessPlate*)pView->GetCurProcessPart();
	if(pPlate->mcsFlg.ciOverturn==TRUE)
		g_pPartEditor->ExecuteCommand(IPEC::CMD_ROTATECLOCKWISE_PLATE);
	else
		g_pPartEditor->ExecuteCommand(IPEC::CMD_ROTATEANTICLOCKWISE_PLATE);
	pView->SyncPartInfo(false);
}
//�ְ������˳ʱ����ת
void CMainFrame::OnRotateClockwise() 
{
	if(GetCurPartType()!=CProcessPart::TYPE_PLATE)
		return;
	CPPEView *pView=theApp.GetView();
	CProcessPlate* pPlate=(CProcessPlate*)pView->GetCurProcessPart();
	if(pPlate->mcsFlg.ciOverturn==TRUE)
		g_pPartEditor->ExecuteCommand(IPEC::CMD_ROTATEANTICLOCKWISE_PLATE);
	else
		g_pPartEditor->ExecuteCommand(IPEC::CMD_ROTATECLOCKWISE_PLATE);
	pView->SyncPartInfo(false);
}
//��ת�ְ�
void CMainFrame::OnOverturnPlate() 
{
	g_pPartEditor->ExecuteCommand(IPEC::CMD_OVERTURN_PART);
	CPPEView *pView=theApp.GetView();
	pView->SyncPartInfo(false);
}
void CMainFrame::OnDrawModeNc()
{
	g_pPartEditor->SetDrawMode(IPEC::DRAW_MODE_NC);
	CPPEView *pView=(CPPEView*)theApp.GetView();
	if(pView)
	{
		pView->Refresh(TRUE);
		pView->UpdatePropertyPage();
	}
}

void CMainFrame::OnUpdateDrawModeNc(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(g_pPartEditor->GetDrawMode()==IPEC::DRAW_MODE_NC);
}

void CMainFrame::OnDrawModeEdit()
{
	g_pPartEditor->SetDrawMode(IPEC::DRAW_MODE_EDIT);
	CPPEView *pView=(CPPEView*)theApp.GetView();
	if(pView)
	{
		pView->Refresh(TRUE);
		pView->UpdatePropertyPage();
	}
}

void CMainFrame::OnUpdateDrawModeEdit(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(g_pPartEditor->GetDrawMode()==IPEC::DRAW_MODE_EDIT);
}

void CMainFrame::OnDrawModeProcesscard()
{
	g_pPartEditor->SetDrawMode(IPEC::DRAW_MODE_PROCESSCARD);
	CPPEView *pView=(CPPEView*)theApp.GetView();
	if(pView)
	{
		pView->Refresh(TRUE);
		pView->UpdatePropertyPage();
	}
}

void CMainFrame::OnUpdateDrawModeProcesscard(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(g_pPartEditor->GetDrawMode()==IPEC::DRAW_MODE_PROCESSCARD);
}

void CMainFrame::OnUpdateRotateAntiClockwise(CCmdUI *pCmdUI)
{
	bool bOn=g_pPartEditor->GetDrawMode()==IPEC::DRAW_MODE_NC
		&&GetCurPartType()==CProcessPart::TYPE_PLATE;
	pCmdUI->Enable(bOn);
}

void CMainFrame::OnUpdateRotateClockwise(CCmdUI *pCmdUI)
{
	bool bOn=g_pPartEditor->GetDrawMode()==IPEC::DRAW_MODE_NC
		&&GetCurPartType()==CProcessPart::TYPE_PLATE;
	pCmdUI->Enable(bOn);
}

void CMainFrame::OnUpdateOverturnPlate(CCmdUI *pCmdUI)
{
	bool bOn=(g_pPartEditor->GetDrawMode()==IPEC::DRAW_MODE_NC
		&&GetCurPartType()==CProcessPart::TYPE_PLATE)||theApp.starter.IsCompareMode();
	pCmdUI->Enable(bOn);
}

void CMainFrame::OnInsertVertex()
{
	g_pPartEditor->ExecuteCommand(IPEC::CMD_INSERT_PLATE_VERTEX);
	CPPEView *pView=theApp.GetView();
	if(pView)
		pView->SyncPartInfo(false);
}

void CMainFrame::OnUpdateInsertVertex(CCmdUI *pCmdUI)
{
	bool bOn=g_pPartEditor->GetDrawMode()==IPEC::DRAW_MODE_EDIT
		&&GetCurPartType()==CProcessPart::TYPE_PLATE;
	pCmdUI->Enable(bOn);
}

void CMainFrame::OnNewLs()
{
	g_pPartEditor->ExecuteCommand(IPEC::CMD_NWE_BOLT_HOLE);
	CPPEView *pView=(CPPEView*)theApp.GetView();
	if(pView)
		pView->SyncPartInfo(false);
}
void CMainFrame::OnUpdateNewLs(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(g_pPartEditor->GetDrawMode()==IPEC::DRAW_MODE_EDIT);
}
void CMainFrame::OnFeatureProp()
{
	CPPEView *pView=(CPPEView*)theApp.GetView();
	pView->SetCurTast(CPPEView::TASK_VIEW_PART_FEATURE);
	g_pSolidSet->SetOperType(OPER_OSNAP);
	g_pSolidSnap->SetSnapType(SNAP_ALL);
	g_pSolidDraw->ReleaseSnapStatus();
}
void CMainFrame::OnUpdateFeatureProp(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(g_pPartEditor->GetDrawMode()==IPEC::DRAW_MODE_EDIT);
}
void CMainFrame::OnSpecWcsOrgin()
{
	CPPEView *pView=(CPPEView*)theApp.GetView();
	pView->SetCurTast(CPPEView::TASK_SPEC_WCS_ORIGIN);
	g_pSolidSet->SetOperType(OPER_OSNAP);
	g_pSolidSnap->SetSnapType(SNAP_POINT|SNAP_CIRCLE);
	g_pSolidDraw->ReleaseSnapStatus();
}
void CMainFrame::OnUpdateSpecWcsOrgin(CCmdUI *pCmdUI)
{
	//pCmdUI->Enable(g_pPartEditor->GetDrawMode()==IPEC::DRAW_MODE_EDIT);
}
void CMainFrame::OnSpecAxisXVertex()
{
	CPPEView *pView=(CPPEView*)theApp.GetView();
	pView->SetCurTast(CPPEView::TASK_SPEC_WCS_AXIS_X);
	g_pSolidSet->SetOperType(OPER_OSNAP);
	g_pSolidSnap->SetSnapType(SNAP_POINT|SNAP_CIRCLE);
	g_pSolidDraw->ReleaseSnapStatus();
}
void CMainFrame::OnUpdateSpecAxisXVertex(CCmdUI *pCmdUI)
{
	//pCmdUI->Enable(g_pPartEditor->GetDrawMode()==IPEC::DRAW_MODE_EDIT);
}
void CMainFrame::OnAssembleCs()
{
	
}

void CMainFrame::OnWorkCs()
{
	
}

void CMainFrame::OnDelPartFeature()
{
	g_pPartEditor->ExecuteCommand(IPEC::CMD_DEL_PART_FEATURE);
	CPPEView *pView=(CPPEView*)theApp.GetView();
	if(pView)
		pView->SyncPartInfo(false);
}

void CMainFrame::OnUpdateDelPartFeature(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(g_pPartEditor->GetDrawMode()==IPEC::DRAW_MODE_EDIT);
}

void CMainFrame::OnCalPlateProfile()
{
	g_pPartEditor->ExecuteCommand(IPEC::CMD_CAL_PLATE_PROFILE);
	CPPEView *pView=(CPPEView*)theApp.GetView();
	if(pView)
		pView->SyncPartInfo(false);
}

void CMainFrame::OnUpdateCalPlateProfile(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(g_pPartEditor->GetDrawMode()==IPEC::DRAW_MODE_EDIT
		&&GetCurPartType()==CProcessPart::TYPE_PLATE);
}

void CMainFrame::OnGlAllZoom()
{
	g_pSolidOper->ZoomAll(0.9);
}

void CMainFrame::OnViewPartList()
{
	g_sysPara.dock.pagePartList.bDisplay=!g_sysPara.dock.pagePartList.bDisplay;
	ModifyDockpageStatus(RUNTIME_CLASS(CPartTreeDlg),g_sysPara.dock.pagePartList.bDisplay);
}

void CMainFrame::OnUpdateViewPartList(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(g_sysPara.dock.pagePartList.bDisplay);	
}

void CMainFrame::OnViewPropList()
{
	g_sysPara.dock.pageProp.bDisplay=!g_sysPara.dock.pageProp.bDisplay;
	ModifyDockpageStatus(RUNTIME_CLASS(CPartPropertyDlg),g_sysPara.dock.pageProp.bDisplay);
}

void CMainFrame::OnUpdateViewPropList(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(g_sysPara.dock.pageProp.bDisplay);	
}
//�Ǹֿ��Ͻǲ���
#include "KaiHeLineAngleDlg.h"
void CMainFrame::OnKaiHeJG()
{
	//g_pPartEditor->ExecuteCommand(IPEC::CMD_KAIHE_LINEANGLE);
	//g_pPartEditor->SaveCurPartInfoToFile();
	CPPEView *pView=(CPPEView*)theApp.GetView();
	CProcessPart* pProcessPart=pView->GetCurProcessPart();
	if(pProcessPart==NULL || pProcessPart->m_cPartType!=CProcessPart::TYPE_LINEANGLE)
		return;
	CProcessAngle* pJg=(CProcessAngle*)pProcessPart;
	CKaiHeLineAngleDlg kaiHeJgDlg;
	if(pJg->IsWingKaiHe())
	{
		kaiHeJgDlg.m_bHasKaiHeWing=TRUE;
		kaiHeJgDlg.m_iKaiHeWing=pJg->kaihe_base_wing_x0_y1;
	}
	if(kaiHeJgDlg.DoModal()!=IDOK)
		return;
	if(!pJg->IsWingKaiHe())
		pJg->kaihe_base_wing_x0_y1=kaiHeJgDlg.m_iKaiHeWing;
	KAI_HE_JIAO kai_he_jiao;
	kai_he_jiao.decWingAngle=kaiHeJgDlg.m_fKaiHeAngle;	//��֫�н�
	kai_he_jiao.position=kaiHeJgDlg.m_iDimPos;			//�궨λ��
	kai_he_jiao.startLength=kaiHeJgDlg.m_iLeftLen;		//ʼ�˿��ϳ���
	kai_he_jiao.endLength=kaiHeJgDlg.m_iRightLen;		//�ն˿��ϳ���
	pJg->kaiHeJiaoList.append(kai_he_jiao);
	//��������
	pView->SyncPartInfo(true);
}
void CMainFrame::OnUpdateKaiHeJG(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(g_pPartEditor->GetDrawMode()==IPEC::DRAW_MODE_EDIT
		&&GetCurPartType()==CProcessPart::TYPE_LINEANGLE);
}
void CMainFrame::OnMeasureDist()
{
	CPPEView *pView=(CPPEView*)theApp.GetView();
	pView->SetCurTast(CPPEView::TASK_DIM_SIZE);
	g_pSolidSet->SetOperType(OPER_OSNAP);
	g_pSolidSnap->SetSnapType(SNAP_ALL);
	g_pSolidDraw->ReleaseSnapStatus();
	g_pPromptMsg->SetMsg("��ѡ���һ������!");
}
void CMainFrame::OnAmendHoleIncrement()
{
	CPPEView *pView=theApp.GetView();
	pView->AmendHoleIncrement();
}
void CMainFrame::OnUpdateAmendHoleIncrement(CCmdUI *pCmdUI)
{
#ifdef __PNC_
	pCmdUI->Enable(TRUE);
#else
	pCmdUI->Enable(g_pPartEditor->GetDrawMode() == IPEC::DRAW_MODE_NC
		&&GetCurPartType() == CProcessPart::TYPE_PLATE);
#endif
}