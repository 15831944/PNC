#pragma once

#include "resource.h"
#include "PropertyList.h"
#include "PropListItem.h"
#include "CADCallBackDlg.h"
#include "SuperGridCtrl.h"

#ifdef __PNC_
class CPNCSysSettingDlg : public CCADCallBackDlg
#else
class CPNCSysSettingDlg : public CDialog
#endif
{
	DECLARE_DYNAMIC(CPNCSysSettingDlg)
public:
	CPNCSysSettingDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CPNCSysSettingDlg();

	afx_msg void OnSelchangeTabGroup(NMHDR* pNMHDR, LRESULT* pResult);
// �Ի�������
	enum { IDD = IDD_SYSTEM_SETTING_DLG };
	CPropertyList	m_propList;
	CTabCtrl	m_ctrlPropGroup;
	CHashStrList<CSuperGridCtrl::CTreeItem*> hashGroupByItemName;
	long m_idEventProp;		//��¼�����жϵ�����ID,�ָ�����ʱʹ��
public:
	void DisplaySystemSetting();
	void OnPNCSysDel();
#ifdef __PNC_
	void SelectEntObj(int nResultEnt=1);	//ѡ�����ڵ����
	void FinishSelectObjOper();				//���ѡ�����ĺ�������
#endif
protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	// DDX/DDV ֧��
	void OnPNCSysAdd();
	void OnPNCSysGroupDel();
	void OnPNCSysGroupAdd();
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	afx_msg void OnOK();
	afx_msg void OnCancel();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnDefault();
	CSuperGridCtrl m_listCtrlSysSetting;
};
