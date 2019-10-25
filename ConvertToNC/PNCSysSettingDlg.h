#pragma once

#include "resource.h"
#include "PropertyList.h"
#include "PropListItem.h"
#include "CADCallBackDlg.h"
#include "SuperGridCtrl.h"
class CPNCSysSettingDlg : public CCADCallBackDlg
{
	DECLARE_DYNAMIC(CPNCSysSettingDlg)
	void FinishSelectObjOper();				//���ѡ�����ĺ�������
public:
	CPNCSysSettingDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CPNCSysSettingDlg();

	afx_msg void OnSelchangeTabGroup(NMHDR* pNMHDR, LRESULT* pResult);
// �Ի�������
	enum { IDD = IDD_SYSTEM_SETTING_DLG };
	CPropertyList	m_propList;
	CTabCtrl	m_ctrlPropGroup;
	long m_idEventProp;		//��¼�����жϵ�����ID,�ָ�����ʱʹ��
public:
	void DisplaySystemSetting();
	void SelectEntObj(int nResultEnt=1);				//ѡ�����ڵ����
protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	void UpdateSave();
	void OnPNCSysDel();
	// DDX/DDV ֧��
	void OnPNCSysAdd();
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	afx_msg void OnOK();
	afx_msg void OnCancel();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnDefault();
	CSuperGridCtrl m_listCtrlSysSetting;
};
