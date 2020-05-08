#ifndef __PNC_SYS_PARAMETER_
#define __PNC_SYS_PARAMETER_
#include "HashTable.h"
#include "XhCharString.h"
#include "PropertyList.h"
#include "PropListItem.h"
#include "ProcessPart.h"
#include "XeroExtractor.h"
#include "AcUiDialogPanel.h"
#include "PartListDlg.h"
#include "SteelSealReactor.h"
#include "DockBarManager.h"
//////////////////////////////////////////////////////////////////////////
//
class CPNCSysPara : public CPlateExtractor , public CJgCardExtractor
{
public:
	struct LAYER_ITEM{
		CXhChar50 m_sLayer;
		BOOL m_bMark;
		LAYER_ITEM(){m_bMark=FALSE;}
		LAYER_ITEM(const char* sName,BOOL bFilter)
		{
			m_sLayer.Copy(sName);
			m_bMark=bFilter;
		}
	};
private:
	CHashStrList<LAYER_ITEM> m_xHashDefaultFilterLayers;
	CHashStrList<LAYER_ITEM> m_xHashEdgeKeepLayers;
public:
	int m_iPPiMode;			//PPI�ļ�����ģʽ 0.һ��һ�� 1.һ����
	bool m_bIncDeformed;	//�Ƿ����˻�������
	BOOL m_bMKPos;			//�Ƿ��ȡ��ӡ��λ��
	BOOL m_bReplaceSH;		//�Ƿ���������״���
	int m_nReplaceHD;		//���׿׾�
	int m_nMkRectLen;		//��ӡ�ֺг���
	int m_nMkRectWidth;		//��ӡ�ֺп��

	static const BYTE LAYOUT_NONE	= 0;
	static const BYTE LAYOUT_PRINT	= 1;
	static const BYTE LAYOUT_SEG	= 2;
	BOOL m_bAutoLayout;		//�Ƿ��Զ��Ű� 0.���Ű� 1.��ӡ�Ű� 2.�ֶ��Ű�
	int m_nMapLength;		//ͼֽ���� 0��ʾ������ֽ�ų���
	int m_nMapWidth;		//ͼֽ���
	int m_nMinDistance;		//��С���
	double m_fMapScale;		//
	int m_iLayerMode;		//ͼ�㴦��ʽ 0.ָ��������ͼ�� 1.����Ĭ��ͼ��
	int m_iAxisXCalType;	//�ӹ�����ϵX����㷽ʽ 0.��� 1.��˨����ƽ�б� 2.���ӱ�
	int m_nBoltPadDimSearchScope;	//��˨����ע������Χ��Ĭ��Ϊ50 wht 19-02-01
	static const BYTE BOLT_RECOG_DEFAULT = 0;
	static const BYTE BOLT_RECOG_NO_FILTER_PARTNO_CIR = 1;
	BYTE m_ciBoltRecogMode;	//0.Ĭ�� 1.�رռ��Ź���
	static const BYTE FILTER_BY_LINETYPE = 0;
	static const BYTE FILTER_BY_LAYER	 = 1;
	static const BYTE FILTER_BY_COLOR	 = 2;
	static const BYTE FILTER_BY_PIXEL	 = 3;
	BYTE m_ciRecogMode;		//0.��ͼ��&����ʶ�� 1.����ɫʶ��
	BYTE m_ciProfileColorIndex;
	BYTE m_ciBendLineColorIndex;
	CXhChar16 m_sProfileLineType;
	double m_fPixelScale;
	//
	CXhChar100 m_sJgCadName;		//�Ǹֹ��տ�
	CXhChar16 m_sPartLabelTitle;	//���ű���
	CXhChar50 m_sJgCardBlockName;	//�Ǹֹ��տ������� wht 19-09-24
	double m_fMaxLenErr;		//����������ֵ
public:
	CPNCSysPara();
	~CPNCSysPara();
	//
	LAYER_ITEM* EnumFirst();
	LAYER_ITEM* EnumNext();
	LAYER_ITEM* AppendSpecItem(const char* sLayer){return m_xHashEdgeKeepLayers.Add(sLayer);}
	LAYER_ITEM* GetEdgeLayerItem(const char* sLayer){return m_xHashEdgeKeepLayers.GetValue(sLayer);}
	void EmptyEdgeLayerHash(){m_xHashEdgeKeepLayers.Empty();}
	void Init();
	RECOG_SCHEMA* InsertRecogSchema(const char* name, int dimStyle, const char* partNoKey,
		const char* matKey, const char* thickKey, const char* partCountKey = "",
		const char* frontBendKey = "", const char* reverseBendKey = "", BOOL bEditable = FALSE);
	void ActiveRecogSchema(RECOG_SCHEMA *pSchema);
	//
	BOOL RecogMkRect(AcDbEntity* pEnt,f3dPoint* ptArr,int nNum);
	BOOL IsNeedFilterLayer(const char* sLayer);
	BOOL IsBendLine(AcDbLine* pAcDbLine,ISymbolRecognizer* pRecognizer=NULL);
	BOOL IsProfileEnt(AcDbEntity* pEnt);
	//
	DECLARE_PROP_FUNC(CPNCSysPara);
	int GetPropValueStr(long id, char *valueStr,UINT nMaxStrBufLen=100,CPropTreeItem *pItem=NULL);//ͨ������Id��ȡ����ֵ
	BOOL IsPartLabelTitle(const char* sText);
	BOOL IsJgCardBlockName(const char* sBlockName);
};
extern CPNCSysPara g_pncSysPara;
extern CSteelSealReactor *g_pSteelSealReactor;	

//
void PNCSysSetImportDefault();
void PNCSysSetExportDefault();
#endif