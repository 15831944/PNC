#pragma once


// CLeftTreeView ��ͼ

class CLeftTreeView : public CTreeView
{
	DECLARE_DYNCREATE(CLeftTreeView)
	HTREEITEM m_hAngleParentItem,m_hPlateParentItem;
protected:
	CLeftTreeView();           // ��̬������ʹ�õ��ܱ����Ĺ��캯��
	virtual ~CLeftTreeView();

public:
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	void InitTreeView(CStringArray &arrAngleFile,CStringArray &arrPlateFile);
	void InsertTreeItem(char* sText,BYTE cPartType);
	void UpdateSelectItem();
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnTvnSelchanged(NMHDR *pNMHDR, LRESULT *pResult);
};


