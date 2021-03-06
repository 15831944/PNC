#pragma once
#include "resource.h"
#include "supergridctrl.h"
#include "XhTreeCtrl.h"
#include "BomModel.h"
// CRevisionDlg 对话框
enum TREEITEM_TYPE{
	PROJECT_GROUP,		//工程塔形组
	PROJECT_ITEM,		
	BOM_GROUP,			//料单组
	BOM_ITEM,
	ANGLE_GROUP,		
	ANGLE_DWG_ITEM,
	PLATE_GROUP,
	PLATE_DWG_ITEM,
};
struct TREEITEM_INFO{
	TREEITEM_INFO(){;}
	TREEITEM_INFO(long type,DWORD dw){itemType=type;dwRefData=dw;}
	long itemType;
	DWORD dwRefData;
};
class CRevisionDlg : public CDialog
{
	DECLARE_DYNAMIC(CRevisionDlg)
	bool m_bEnableWindowMoveListen;
	int m_nScrLocationX;
	int m_nScrLocationY;
	int m_nRightMargin;
	int m_nBtmMargin;
	int m_iCompareMode;
public:
	CImageList m_imageList;
	ATOM_LIST<TREEITEM_INFO>itemInfoList;
	CString m_sCurFile;	// 当前显示文件
	CString m_sRecordNum;	// 显示记录数
public:
	CRevisionDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CRevisionDlg();
	// 对话框数据
	enum { IDD = IDD_REVISION_DLG };
	//控件变量
	CSuperGridCtrl m_xListReport;
	CXhTreeCtrl m_treeCtrl;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	//
	DECLARE_MESSAGE_MAP()
	afx_msg void OnMove(int x, int y);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnNMRClickTreeCtrl(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnSelchangedTreeContrl(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNewItem();
	afx_msg void OnLoadProjFile();
	afx_msg void OnExportProjFile();
	afx_msg void OnProjectProperty();
	afx_msg void OnImportBomFile();
	afx_msg void OnImportAngleDwg();
	afx_msg void OnImportPlateDwg();
	afx_msg void OnCompareData();
	afx_msg void OnExportCompResult();
	afx_msg void OnRefreshPartNum();
	afx_msg void OnModifyErpFile();
	afx_msg void OnModifyTmaFile();
	afx_msg void OnReviseThePlate();
	afx_msg void OnRetrievedAngles();
	afx_msg void OnRetrievedPlates();
	afx_msg void OnDeleteItem();
public:
	CXhTreeCtrl *GetTreeCtrl(){return &m_treeCtrl;}
	void ContextMenu(CWnd *pWnd, CPoint point);
	void RefreshTreeCtrl();
	void RefreshProjectItem(HTREEITEM hItem,CProjectTowerType* pProject);
	void RefreshListCtrl(HTREEITEM hItem,BOOL bCompared=FALSE);
	CProjectTowerType* GetProject(HTREEITEM hItem);
	HTREEITEM FindTreeItem(HTREEITEM hItem,CXhChar100 sName);
	//
	BOOL Create();
	void InitRevisionDlg();
};
extern CRevisionDlg *g_pRevisionDlg;
