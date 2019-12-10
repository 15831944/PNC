#include "stdafx.h"
#include "PNCSysPara.h"
#include "CadToolFunc.h"
#include "XeroExtractor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPNCSysPara g_pncSysPara;
CSteelSealReactor *g_pSteelSealReactor = NULL;
const int HASHTABLESIZE=500;
const int STATUSHASHTABLESIZE=500;
//////////////////////////////////////////////////////////////////////////
//CDimRuleManager
CPNCSysPara::CPNCSysPara()
{
	Init();
}
RECOG_SCHEMA* CPNCSysPara::InsertRecogSchema(const char* name,int dimStyle,const char* partNoKey,
										     const char* matKey, const char* thickKey,const char* partCountKey/*=""*/,
											 const char* frontBendKey/*=""*/,const char* reverseBendKey/*=""*/,BOOL bEditable/*=FALSE*/)
{
	RECOG_SCHEMA *pSchema1 = g_pncSysPara.m_recogSchemaList.append();
	pSchema1->m_bEditable = bEditable;
	pSchema1->m_iDimStyle = dimStyle;
	pSchema1->m_bEnable = FALSE;
	if(name!=NULL)
		pSchema1->m_sSchemaName.Copy(name);
	if(partCountKey!=NULL)
		pSchema1->m_sPnKey.Copy(partNoKey);
	if(thickKey!=NULL)
		pSchema1->m_sThickKey.Copy(thickKey);
	if(matKey!=NULL)
		pSchema1->m_sMatKey.Copy(matKey);
	if(partCountKey!=NULL)
		pSchema1->m_sPnNumKey.Copy(partCountKey);
	if(frontBendKey!=NULL)
		pSchema1->m_sFrontBendKey.Copy(frontBendKey);
	if(reverseBendKey!=NULL)
		pSchema1->m_sReverseBendKey.Copy(reverseBendKey);
	return pSchema1;
}
void CPNCSysPara::Init()
{
	CPlateExtractor::Init();
	m_iPPiMode = 1;
	m_bIncDeformed = true;
	m_iAxisXCalType = 0;
	m_bReplaceSH = FALSE;
	m_nReplaceHD = 20;
	//�Զ��Ű�����
	m_bAutoLayout = FALSE;
	m_nMapWidth = 1500;
	m_nMapLength = 0;
	m_nMinDistance = 0;
	m_bMKPos = 0;
	m_nMkRectWidth = 30;
	m_nMkRectLen = 60;
	//ͼֽ��������
	m_fMapScale = 1;
	m_iLayerMode = 0;
	m_ciRecogMode = 0;
	m_ciBoltRecogMode = BOLT_RECOG_DEFAULT;
	m_ciProfileColorIndex = 1;		//��ɫ
	m_ciBendLineColorIndex = 190;	//��ɫ
	m_sProfileLineType.Copy("CONTINUOUS");
	//Ĭ�Ϲ���ͼ����
	//Ĭ�ϼ�������ʶ������
	g_pncSysPara.m_recogSchemaList.Empty();
	RECOG_SCHEMA *pSchema = InsertRecogSchema("����1", 0, "#", "Q", "-");
	if (pSchema)
		pSchema->m_bEnable = TRUE;
	InsertRecogSchema("����2", 0, "#", "Q", "-", "��");
	InsertRecogSchema("����3", 0, "#", "Q", "-", "��", "����", "����");
	InsertRecogSchema("����4", 0, "#", "Q", "-", "��", "����", "����");
	InsertRecogSchema("����1", 1, "#", "Q", "-");
	InsertRecogSchema("����2", 1, "#", "Q", "-", "��");
	InsertRecogSchema("����3", 1, "#", "Q", "-", "��", "����", "����");
	InsertRecogSchema("����4", 1, "#", "Q", "-", "��", "����", "����");
	InsertRecogSchema("����5", 1, "����:", "����:", "���:");
	InsertRecogSchema("����6", 1, "����:", "����:", "���:", "����");
	InsertRecogSchema("����7", 1, "����:", "����:", "���:", "����", "����", "����");
	InsertRecogSchema("����8", 1, "����:", "����:", "���:", "����", "����", "����");
	
#ifdef __PNC_
	InitDrawingEnv();
	m_xHashDefaultFilterLayers.SetValue(LayerTable::UnvisibleProfileLayer.layerName, LAYER_ITEM(LayerTable::UnvisibleProfileLayer.layerName, 1));//���ɼ�������
	m_xHashDefaultFilterLayers.SetValue(LayerTable::BoltSymbolLayer.layerName, LAYER_ITEM(LayerTable::BoltSymbolLayer.layerName, 1));		//��˨ͼ��
	m_xHashDefaultFilterLayers.SetValue(LayerTable::BendLineLayer.layerName, LAYER_ITEM(LayerTable::BendLineLayer.layerName, 1));		//�Ǹֻ������ְ����
	m_xHashDefaultFilterLayers.SetValue(LayerTable::BreakLineLayer.layerName, LAYER_ITEM(LayerTable::BreakLineLayer.layerName, 1));		//�Ͽ�����
	m_xHashDefaultFilterLayers.SetValue(LayerTable::DimTextLayer.layerName, LAYER_ITEM(LayerTable::DimTextLayer.layerName, 1));		//���ֱ�עͼ��
	m_xHashDefaultFilterLayers.SetValue(LayerTable::BoltLineLayer.layerName, LAYER_ITEM(LayerTable::BoltLineLayer.layerName, 1));		//��˨��
	m_xHashDefaultFilterLayers.SetValue(LayerTable::DamagedSymbolLine.layerName, LAYER_ITEM(LayerTable::DamagedSymbolLine.layerName, 1));	//�����������
	m_xHashDefaultFilterLayers.SetValue(LayerTable::CommonLayer.layerName, LAYER_ITEM(LayerTable::CommonLayer.layerName, 1));			//����
#endif
}
CPNCSysPara::~CPNCSysPara()
{
	hashBoltDList.Empty();
	m_xHashDefaultFilterLayers.Empty();
	m_xHashEdgeKeepLayers.Empty();
}
CPNCSysPara::LAYER_ITEM* CPNCSysPara::EnumFirst()
{
	if(m_iLayerMode==1)
		return m_xHashDefaultFilterLayers.GetFirst();
	else
		return m_xHashEdgeKeepLayers.GetFirst();
}
CPNCSysPara::LAYER_ITEM* CPNCSysPara::EnumNext()
{
	if(m_iLayerMode==1)
		return m_xHashDefaultFilterLayers.GetNext();
	else
		return m_xHashEdgeKeepLayers.GetNext();
}
IMPLEMENT_PROP_FUNC(CPNCSysPara);
void CPNCSysPara::InitPropHashtable()
{
	int id=1;
	propHashtable.SetHashTableGrowSize(HASHTABLESIZE);
	propStatusHashtable.CreateHashTable(STATUSHASHTABLESIZE);
	//��������
	AddPropItem("general_set",PROPLIST_ITEM(id++,"��������","��������"));
	AddPropItem("m_bIncDeformed",PROPLIST_ITEM(id++,"�ѿ��ǻ�������","����ȡ�ĸְ�ͼ���ѿ��ǻ���������","��|��"));
	AddPropItem("m_bReplaceSH", PROPLIST_ITEM(id++, "��������״���", "�����⹦�����͵Ŀ׽��д��״���", "��|��"));
	AddPropItem("m_nReplaceHD", PROPLIST_ITEM(id++, "����ֱ��", "���д��׵���˨ֱ��", "12|16|20|24"));
	AddPropItem("m_iPPiMode",PROPLIST_ITEM(id++,"�ļ�����ģ��","PPI�ļ�����ģʽ","0.һ��һ��|1.һ����"));
	AddPropItem("m_bAutoLayout2", PROPLIST_ITEM(id++, "����Ԥ��", "��������Ԥ��ʱ���ְ���ȡ֮�󰴶��Ų���֧���趨�ӹ�����ϵ��֧���趨��ӡ��λ�á�", "��|��"));
	AddPropItem("CDrawDamBoard::BOARD_HEIGHT", PROPLIST_ITEM(id++, "����߶�", "����߶�"));
	AddPropItem("CDrawDamBoard::m_bDrawAllBamBoard", PROPLIST_ITEM(id++, "������ʾģʽ", "������ʾģʽ", "0.����ʾѡ�иְ嵵��|1.��ʾ���е���"));
	AddPropItem("m_nMkRectLen", PROPLIST_ITEM(id++, "��ӡ�ֺг���", "��ӡ�ֺп��"));
	AddPropItem("m_nMkRectWidth", PROPLIST_ITEM(id++, "��ӡ�ֺп��", "��ӡ�ֺп��"));
	AddPropItem("m_bAutoLayout1",PROPLIST_ITEM(id++,"��ӡ�Ű�","��ӡ�Ű�","��|��"));
	AddPropItem("m_nMapWidth",PROPLIST_ITEM(id++,"ͼֽ���","ͼֽ���"));
	AddPropItem("m_nMapLength",PROPLIST_ITEM(id++,"ͼֽ����","ͼֽ����"));
	AddPropItem("m_nMinDistance",PROPLIST_ITEM(id++,"��С���","ͼ��֮�����С���"));
	AddPropItem("m_bMKPos",PROPLIST_ITEM(id++,"��ȡ��ӡλ��","��ȡ��ӡλ��","��|��"));
	AddPropItem("AxisXCalType",PROPLIST_ITEM(id++,"X����㷽ʽ","�ӹ�����ϵX��ļ��㷽ʽ","0.�������|1.��˨ƽ�б�����|2.���ӱ�����"));
	AddPropItem("m_ciBoltRecogMode", PROPLIST_ITEM(id++, "��˨ʶ��ģʽ", "��˨ʶ��ģʽ", "0.Ĭ��|1.�����˼���ԲȦ"));
	//ͼ������
	AddPropItem("layer_set",PROPLIST_ITEM(id++,"ʶ��ģʽ","ʶ��ģʽ"));
	AddPropItem("m_iRecogMode",PROPLIST_ITEM(id++,"ʶ��ģʽ","�ְ�ʶ��ģʽ","0.������ʶ��|1.��ͼ��ʶ��|2.����ɫʶ��"));
	AddPropItem("m_iProfileLineTypeName", PROPLIST_ITEM(id++, "����������", "�ְ�������������", "CONTINUOUS|HIDDEN|DASHDOT2X|DIVIDE|ZIGZAG"));
	AddPropItem("m_iProfileColorIndex",PROPLIST_ITEM(id++,"��������ɫ","�ְ�����������ɫ"));
	AddPropItem("m_iBendLineColorIndex",PROPLIST_ITEM(id++,"��������ɫ","�ְ���������ɫ"));
	AddPropItem("layer_mode",PROPLIST_ITEM(id++,"ͼ�㴦��ʽ","������ͼ�㴦��ʽ","0.ָ��������ͼ��|1.����Ĭ��ͼ��"));
	//����ʶ������
	AddPropItem("text_dim_set",PROPLIST_ITEM(id++,"����ʶ��","���ֱ�עʶ������"));
	AddPropItem("m_iDimStyle",PROPLIST_ITEM(id++,"��ע����","���ֱ�ע����","0.���б�ע|1.���б�ע"));
	AddPropItem("m_nMember",PROPLIST_ITEM(id++,"��ע��Ա","��ע��Ա"));
	AddPropItem("m_sPnKey",PROPLIST_ITEM(id++,"���ű�ʶ��","���ű�ʶ��","#|����:|���ţ�|"));
	AddPropItem("m_sThickKey",PROPLIST_ITEM(id++,"��ȱ�ʶ��","��ȱ�ʶ��","-|���:|���|���:|��ȣ�|"));
	AddPropItem("m_sMatKey",PROPLIST_ITEM(id++,"���ʱ�ʶ��","���ʱ�ʶ��","Q|����:|���ʣ�|"));
	AddPropItem("m_sPnNumKey",PROPLIST_ITEM(id++,"������ʶ��","������ʶ��","����:|������|����:|������|"));
	//��˨ֱ������
	AddPropItem("bolt_dim_set",PROPLIST_ITEM(id++,"��˨ʶ��","��˨ʶ������"));
	AddPropItem("block_name",PROPLIST_ITEM(id++,"ͼ������","ͼ������"));
	AddPropItem("bolt_d",PROPLIST_ITEM(id++,"��˨ֱ��","��˨ֱ��"));
	AddPropItem("hole_d",PROPLIST_ITEM(id++,"�׾�","��˨ֱ��Ϊ0ʱ��ʹ�ÿ׾���ֱ���׾���Ϊ0ʱ����ֱ����ע��ȡ�׾���"));
	//ͼֽ��������
	AddPropItem("map_scale_set",PROPLIST_ITEM(id++,"����ʶ��","ͼֽ��������"));
	AddPropItem("m_fMapScale",PROPLIST_ITEM(id++,"���ű���","��ͼ���ű���"));
}
int CPNCSysPara::GetPropValueStr(long id,char* valueStr,UINT nMaxStrBufLen/*=100*/,CPropTreeItem *pItem/*=NULL*/)
{
	CXhChar100 sText;
	if (GetPropID("m_bIncDeformed") == id)
	{
		if (g_pncSysPara.m_bIncDeformed)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if (GetPropID("m_bReplaceSH") == id)
	{
		if (g_pncSysPara.m_bReplaceSH)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if (GetPropID("m_nReplaceHD") == id)
		sText.Printf("%d", g_pncSysPara.m_nReplaceHD);
	else if(GetPropID("m_iPPiMode")==id)
	{
		if(g_pncSysPara.m_iPPiMode==0)
			sText.Copy("0.һ��һ��");
		else if(g_pncSysPara.m_iPPiMode==1)
			sText.Copy("1.һ����");
	}
	else if(GetPropID("AxisXCalType")==id)
	{
		if(g_pncSysPara.m_iAxisXCalType==0)
			sText.Copy("0.�������");
		else if(g_pncSysPara.m_iAxisXCalType==1)
			sText.Copy("1.��˨ƽ�б�����");
		else if(g_pncSysPara.m_iAxisXCalType==2)
			sText.Copy("2.���ӱ�����");
	}
	else if(GetPropID("m_iDimStyle")==id)
	{
		if(g_pncSysPara.m_iDimStyle==0)
			sText.Copy("0.���б�ע");
		else
			sText.Copy("1.���б�ע");
	}
	else if(GetPropID("m_nMember")==id)
		sText.Printf("%d",g_pncSysPara.GetKeyMemberNum());
	else if(GetPropID("m_sPnKey")==id)
		sText.Copy(g_pncSysPara.m_sPnKey);
	else if(GetPropID("m_sThickKey")==id)
		sText.Copy(g_pncSysPara.m_sThickKey);
	else if(GetPropID("m_sMatKey")==id)
		sText.Copy(g_pncSysPara.m_sMatKey);
	else if(GetPropID("m_sPnNumKey")==id)
		sText.Copy(g_pncSysPara.m_sPnNumKey);
	else if(GetPropID("m_bMKPos")==id)
	{
		if(g_pncSysPara.m_bMKPos)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if(GetPropID("m_fMapScale")==id)
		sText.Printf("%.f",g_pncSysPara.m_fMapScale);
	else if(GetPropID("block_name")==id)
	{
		if(pItem&&pItem->m_pParent)
		{
			BOLT_BLOCK *pBoltD=(BOLT_BLOCK*)pItem->m_pParent->m_idProp;
			if(pBoltD)
				sText.Copy(pBoltD->sBlockName);
		}
	}
	else if(GetPropID("bolt_d")==id)
	{
		if(pItem&&pItem->m_pParent)
		{
			BOLT_BLOCK *pBoltD=(BOLT_BLOCK*)pItem->m_pParent->m_idProp;
			if(pBoltD)
				sText.Printf("%d",pBoltD->diameter);
		}
	}
	else if(GetPropID("hole_d")==id)
	{
		if(pItem&&pItem->m_pParent)
		{
			BOLT_BLOCK *pBoltD=(BOLT_BLOCK*)pItem->m_pParent->m_idProp;
			if(pBoltD)
				sText.Printf("%.1f",pBoltD->hole_d);
		}
	}
	else if(GetPropID("m_bAutoLayout1")==id)
	{
		if(m_bAutoLayout==CPNCSysPara::LAYOUT_PRINT)
			sText.Copy("��");
		else 
			sText.Copy("��");
	}
	else if (GetPropID("m_bAutoLayout2") == id)
	{
		if (m_bAutoLayout == CPNCSysPara::LAYOUT_SEG)
			sText.Copy("��");
		else
			sText.Copy("��");
	}
	else if(GetPropID("m_nMapLength")==id)
		sText.Printf("%d",g_pncSysPara.m_nMapLength);
	else if(GetPropID("m_nMapWidth")==id)
		sText.Printf("%d",g_pncSysPara.m_nMapWidth);
	else if(GetPropID("m_nMinDistance")==id)
		sText.Printf("%d",g_pncSysPara.m_nMinDistance);
	else if(GetPropID("CDrawDamBoard::m_bDrawAllBamBoard")==id)
	{
		if(CDrawDamBoard::m_bDrawAllBamBoard)
			sText.Copy("1.��ʾ���е���");
		else 
			sText.Copy("0.����ʾѡ�иְ嵵��");
	}
	else if(GetPropID("CDrawDamBoard::BOARD_HEIGHT")==id)
		sText.Printf("%d",CDrawDamBoard::BOARD_HEIGHT);
	else if (GetPropID("m_nMkRectWidth") == id)
		sText.Printf("%d", g_pncSysPara.m_nMkRectWidth);
	else if (GetPropID("m_nMkRectLen") == id)
		sText.Printf("%d", g_pncSysPara.m_nMkRectLen);
	else if(GetPropID("layer_mode")==id)
	{
		if(m_iLayerMode==0)
			sText.Copy("0.ָ��������ͼ��");
		else if(m_iLayerMode==1)
			sText.Copy("1.����Ĭ��ͼ��");
	}
	else if(GetPropID("m_iRecogMode")==id)
	{
		if(m_ciRecogMode==FILTER_BY_LINETYPE)
			sText.Copy("0.������ʶ��");
		else if (m_ciRecogMode == FILTER_BY_LAYER)
			sText.Copy("1.��ͼ��ʶ��");
		else if(m_ciRecogMode==FILTER_BY_COLOR)
			sText.Copy("2.����ɫʶ��");
	}
	else if (GetPropID("m_ciBoltRecogMode") == id)
	{
	if (m_ciBoltRecogMode == BOLT_RECOG_NO_FILTER_PARTNO_CIR)
		sText.Copy("1.�����˼���ԲȦ");
	else //if (m_ciBoltRecogMode == BOLT_RECOG_DEFAULT)
		sText.Copy("0.Ĭ��");
	}
	else if(GetPropID("m_iProfileColorIndex")==id)
		sText.Printf("RGB%X",GetColorFromIndex(m_ciProfileColorIndex));
	else if(GetPropID("m_iBendLineColorIndex")==id)
		sText.Printf("RGB%X",GetColorFromIndex(m_ciBendLineColorIndex));
	else if(GetPropID("m_iProfileLineTypeName")==id)
		sText.Copy(m_sProfileLineType);
	if(valueStr)
		StrCopy(valueStr,sText,nMaxStrBufLen);
	return strlen(sText);
}

BOOL CPNCSysPara::IsNeedFilterLayer(const char* sLayer)
{
	if(m_ciRecogMode==FILTER_BY_LAYER)
	{
		if(m_iLayerMode==0&&m_xHashEdgeKeepLayers.GetNodeNum()>0)
		{	//�û�ָ������������ͼ��
			LAYER_ITEM* pItem=GetEdgeLayerItem(sLayer);
			if(pItem && pItem->m_bMark)
				return FALSE;
			else
				return TRUE;
		}
		else if(m_iLayerMode==1&&m_xHashDefaultFilterLayers.GetValue(sLayer))
			return TRUE;
	}
	return FALSE;
}

BOOL CPNCSysPara::IsBendLine(AcDbLine* pAcDbLine,ISymbolRecognizer* pRecognizer/*=NULL*/)
{
	BOOL bRet=CPlateExtractor::IsBendLine(pAcDbLine,pRecognizer);
	if(!bRet)
	{	
		if(m_ciRecogMode==FILTER_BY_COLOR)
			bRet=(GetEntColorIndex(pAcDbLine)==m_ciBendLineColorIndex);
	}
	return bRet;
}

BOOL CPNCSysPara::RecogMkRect(AcDbEntity* pEnt,f3dPoint* ptArr,int nNum)
{
	if(m_bMKPos==FALSE)	//����Ҫ��ȡ��ӡ��
		return FALSE;
	if(pEnt->isKindOf(AcDbText::desc()))
	{	//��ȡ��ӡ��
		AcDbText* pText=(AcDbText*)pEnt;
#ifdef _ARX_2007
		CXhChar50 sText(_bstr_t(pText->textString()));
#else
		CXhChar50 sText(pText->textString());
#endif
		if(!sText.Equal("��ӡ��"))
			return FALSE;
		double len=DrawTextLength(sText,pText->height(),pText->textStyle());
		f3dPoint dim_vec(cos(pText->rotation()),sin(pText->rotation()));
		f3dPoint origin(pText->position().x,pText->position().y,pText->position().z);
		origin+=dim_vec*len*0.5;
		f2dRect rect;
		rect.topLeft.Set(origin.x-10,origin.y+10);
		rect.bottomRight.Set(origin.x+10,origin.y-10);
		ZoomAcadView(rect,200);		//�Ը�ӡ�������ʵ�����
		ads_name seqent;
		AcDbObjectId initLastObjId,plineId;
		acdbEntLast(seqent);
		acdbGetObjectId(initLastObjId,seqent);
		ads_point base_pnt;
		base_pnt[X]=origin.x;
		base_pnt[Y]=origin.y;
		base_pnt[Z]=origin.z;
#ifdef _ARX_2007
		int resCode=acedCommand(RTSTR,L"-boundary",RTSTR,L"a",RTSTR,L"i",RTSTR,L"n",RTSTR,L"",RTSTR,L"",RTPOINT,base_pnt,RTSTR,L"",RTNONE);
#else
		int resCode=acedCommand(RTSTR,"-boundary",RTSTR,"a",RTSTR,"i",RTSTR,"n",RTSTR,"",RTSTR,"",RTPOINT,base_pnt,RTSTR,"",RTNONE);
#endif		
		if(resCode!=RTNORM)
			return FALSE;
		acdbEntLast(seqent);
		acdbGetObjectId(plineId,seqent);
		if(initLastObjId==plineId)
			return FALSE;
		AcDbEntity *pEnt=NULL;
		acdbOpenAcDbEntity(pEnt,plineId,AcDb::kForWrite);
		AcDbPolyline *pPline=(AcDbPolyline*)pEnt;
		if(pPline==NULL||pPline->numVerts()!=nNum)
		{
			if(pPline)
			{
				pPline->erase(Adesk::kTrue);
				pPline->close();
			}
			return FALSE;
		}
		AcGePoint3d location;
		for(int iVertIndex=0;iVertIndex<nNum;iVertIndex++)
		{
			pPline->getPointAt(iVertIndex,location);
			ptArr[iVertIndex].Set(location.x,location.y,location.z);
		}
		pPline->erase(Adesk::kTrue);	//ɾ��polyline����
		pPline->close();
	}
	else if(pEnt->isKindOf(AcDbBlockReference::desc()))
	{	//��ӡ��ͼ��
		AcDbBlockReference* pReference=(AcDbBlockReference*)pEnt;
		AcDbObjectId blockId=pReference->blockTableRecord();
		AcDbBlockTableRecord *pTempBlockTableRecord=NULL;
		acdbOpenObject(pTempBlockTableRecord,blockId,AcDb::kForRead);
		if(pTempBlockTableRecord==NULL)
			return FALSE;
		pTempBlockTableRecord->close();
		CXhChar50 sName;
#ifdef _ARX_2007
		ACHAR* sValue=new ACHAR[50];
		pTempBlockTableRecord->getName(sValue);
		sName.Copy((char*)_bstr_t(sValue));
		delete[] sValue;
#else
		char *sValue=new char[50];
		pTempBlockTableRecord->getName(sValue);
		sName.Copy(sValue);
		delete[] sValue;
#endif
		if(!sName.Equal("MK"))
			return FALSE;
		double rot_angle=pReference->rotation();
		f3dPoint orig(pReference->position().x,pReference->position().y,0);
		AcDbBlockTableRecordIterator *pIterator=NULL;
		pTempBlockTableRecord->newIterator( pIterator);
		for(;!pIterator->done();pIterator->step())
		{
			pIterator->getEntity(pEnt,AcDb::kForRead);
			CAcDbObjLife entObj(pEnt);
			if(pEnt->isKindOf(AcDbPolyline::desc()))
			{
				AcGePoint3d location;
				AcDbPolyline* pPolyLine=(AcDbPolyline*)pEnt;
				for(int iVertIndex=0;iVertIndex<nNum;iVertIndex++)
				{
					pPolyLine->getPointAt(iVertIndex,location);
					ptArr[iVertIndex].Set(location.x,location.y,location.z);
				}
				break;
			}
		}
		pTempBlockTableRecord->close();
		//���¸�ӡ��ʵ������
		for(int i=0;i<nNum;i++)
		{
			if(fabs(rot_angle)>0)	//ͼ������ת�Ƕ�
				rotate_point_around_axis(ptArr[i],rot_angle,f3dPoint(),100*f3dPoint(0,0,1));
			ptArr[i]+=orig;
		}
	}
	else
		return FALSE;
	return TRUE;
}
//////////////////////////////////////////////////////////////////////////
//

void PNCSysSetImportDefault()
{
	FILE *fp;
	char file_name[MAX_PATH], line_txt[MAX_PATH], key_word[100];
#ifdef __PNC_
	GetAppPath(file_name);
#else
	strcpy(file_name, "D:\\");
#endif
	strcat(file_name, "rule.set");
	if ((fp = fopen(file_name, "rt")) == NULL)
		return;
	int nTemp = 0;
	g_pncSysPara.hashBoltDList.Empty();
	g_pncSysPara.m_recogSchemaList.Empty();
	while (!feof(fp))
	{
		if (fgets(line_txt, MAX_PATH, fp) == NULL)
			break;
		char sText[MAX_PATH];
		strcpy(sText, line_txt);
		CString sLine(line_txt);
		sLine.Replace('=', ' ');
		sprintf(line_txt, "%s", sLine);
		char *skey = strtok((char*)sText, "=,;");
		strncpy(key_word, skey, 100);
		//��������
		if (_stricmp(key_word, "DimStyle") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_iDimStyle);
		else if (_stricmp(key_word, "PnKey") == 0)
		{
			skey = strtok(NULL, "=,;");
			sprintf(g_pncSysPara.m_sPnKey, "%s", skey);
			g_pncSysPara.m_sPnKey.Replace(" ", "");
		}
		else if (_stricmp(key_word, "ThickKey") == 0)
		{
			skey = strtok(NULL, "=,;");
			sprintf(g_pncSysPara.m_sThickKey, "%s", skey);
			g_pncSysPara.m_sThickKey.Replace(" ", "");
		}
		else if (_stricmp(key_word, "MatKey") == 0)
		{
			skey = strtok(NULL, "=,;");
			sprintf(g_pncSysPara.m_sMatKey, "%s", skey);
			g_pncSysPara.m_sMatKey.Replace(" ", "");
		}
		else if (_stricmp(key_word, "PnNumKey") == 0)
		{
			skey = strtok(NULL, "=,;");
			sprintf(g_pncSysPara.m_sPnNumKey, "%s", skey);
			g_pncSysPara.m_sPnNumKey.Replace(" ", "");
		}
		else if (_stricmp(key_word, "MKPos") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_bMKPos);
		else if (_stricmp(key_word, "MapScale") == 0)
			sscanf(line_txt, "%s%f", key_word, &g_pncSysPara.m_fMapScale);
		else if (_stricmp(key_word, "bIncFilterLayer") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_iLayerMode);
		else if (_stricmp(key_word, "RecogMode") == 0)
		{	//ʹ��%d��ȡBYTE�ͱ�����һ�θ���4���ֽ�Ӱ����������ȡֵ
			//��ʹ��%hhu��ȡBYTE������������Ч���ȶ�ȡ����ʱ�����У��ٸ�ֵ����Ӧ������ wht 19-10-05
			sscanf(line_txt, "%s%d", key_word, &nTemp);
			g_pncSysPara.m_ciRecogMode = nTemp;
		}
		else if (_stricmp(key_word, "BoltRecogMode") == 0)
		{
			sscanf(line_txt, "%s%d", key_word, &nTemp);
			g_pncSysPara.m_ciBoltRecogMode = nTemp;
			//sscanf(line_txt, "%s%hhu", key_word, &g_pncSysPara.m_ciBoltRecogMode);
		}
		else if (_stricmp(key_word, "BendLineColorIndex") == 0)
		{
			sscanf(line_txt, "%s%d", key_word, &nTemp);
			g_pncSysPara.m_ciBendLineColorIndex = nTemp;
			//sscanf(line_txt, "%s%hhu", key_word, &g_pncSysPara.m_ciBendLineColorIndex);
		}
		else if (_stricmp(key_word, "ProfileColorIndex") == 0)
		{
			sscanf(line_txt, "%s%d", key_word, &nTemp);
			g_pncSysPara.m_ciProfileColorIndex = nTemp;
			//sscanf(line_txt, "%s%hhu", key_word, &g_pncSysPara.m_ciProfileColorIndex);
		}
		else if (_stricmp(key_word, "ProfileLineType") == 0)
			sscanf(line_txt, "%s%s", key_word, &g_pncSysPara.m_sProfileLineType);
		else if (_stricmp(key_word, "BoltDKey") == 0)
		{
			skey = strtok(NULL, "=,;");
			CString skeyName(skey);
			if (strlen(skey) > 0)
			{
				BOLT_BLOCK *pBoltD = g_pncSysPara.hashBoltDList.Add(skey);
				fgets(line_txt, MAX_PATH, fp);
				skey = strtok(line_txt, ";");
				if (!strcmp(skeyName, skey))
				{
					pBoltD->sGroupName.Printf(" ");
					pBoltD->sBlockName.Printf("%s", skey);
				}
				else
				{
					pBoltD->sGroupName.Printf("%s", skey);
					skey = strtok(NULL, ";");
					pBoltD->sBlockName.Printf("%s", skey);
				}
				skey = strtok(NULL, ";");
				pBoltD->diameter = atoi(skey);
				skey = strtok(NULL, ";");
				if (skey != NULL)
					pBoltD->hole_d = atof(skey);
			}
		}
		else if (_stricmp(key_word, "m_iDimStyle") == 0)
		{
			skey = strtok(NULL, ",;");
			if (strlen(skey) > 0)
			{
				RECOG_SCHEMA *pSchema = g_pncSysPara.m_recogSchemaList.append();
				pSchema->m_iDimStyle = atoi(skey);
				skey = strtok(line_txt, ";");
				skey = strtok(NULL, ";");
				pSchema->m_sSchemaName = skey;
				pSchema->m_sSchemaName.Replace(" ", "");
				skey = strtok(NULL, ";");
				pSchema->m_sPnKey = skey;
				pSchema->m_sPnKey.Replace(" ", "");
				skey = strtok(NULL, ";");
				pSchema->m_sThickKey = skey;
				pSchema->m_sThickKey.Replace(" ", "");
				skey = strtok(NULL, ";");
				pSchema->m_sMatKey = skey;
				pSchema->m_sMatKey.Replace(" ", "");
				skey = strtok(NULL, ";");
				pSchema->m_sPnNumKey = skey;
				pSchema->m_sPnNumKey.Replace(" ", "");
				skey = strtok(NULL, ";");
				pSchema->m_sFrontBendKey = skey;
				pSchema->m_sFrontBendKey.Replace(" ", "");
				skey = strtok(NULL, ";");
				pSchema->m_sReverseBendKey = skey;
				pSchema->m_sReverseBendKey.Replace(" ", "");
				skey = strtok(NULL, ";");
				pSchema->m_bEditable = atoi(skey);
				skey = strtok(NULL, ";");
				pSchema->m_bEnable = atoi(skey);
			}
		}
		else if (_stricmp(key_word, "bIncDeformed") == 0)
		{
			CXhChar16 sText;
			sscanf(line_txt, "%s%s", key_word, (char*)sText);
			if (sText.Equal("��"))
				g_pncSysPara.m_bIncDeformed = true;
			else
				g_pncSysPara.m_bIncDeformed = false;
		}
		else if (_stricmp(key_word, "PPIMode") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_iPPiMode);
		else if (_stricmp(key_word, "AutoLayout") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_bAutoLayout);
		else if (_stricmp(key_word, "MapLength") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_nMapLength);
		else if (_stricmp(key_word, "MapWidth") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_nMapWidth);
		else if (_stricmp(key_word, "MinDistance") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_nMinDistance);
		else if (_stricmp(key_word, "AxisXCalType") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_iAxisXCalType);
		else if (_stricmp(key_word, "m_nMkRectWidth") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_nMkRectWidth);
		else if (_stricmp(key_word, "m_nMkRectLen") == 0)
			sscanf(line_txt, "%s%d", key_word, &g_pncSysPara.m_nMkRectLen);
#ifdef __PNC_
		else if (_stricmp(key_word, "CDrawDamBoard::BOARD_HEIGHT") == 0)
			sscanf(line_txt, "%s%d", key_word, &CDrawDamBoard::BOARD_HEIGHT);
		else if (_stricmp(key_word, "CDrawDamBoard::m_bDrawAllBamBoard") == 0)
			sscanf(line_txt, "%s%d", key_word, &CDrawDamBoard::m_bDrawAllBamBoard);
#endif
	}
	fclose(fp);
	//���������ļ��󼤻ǰʶ��ģ�� wht 19-10-30
	for (RECOG_SCHEMA *pSchema = g_pncSysPara.m_recogSchemaList.GetFirst(); pSchema; pSchema = g_pncSysPara.m_recogSchemaList.GetNext())
	{
		if (pSchema->m_bEnable)
		{
			g_pncSysPara.ActiveRecogSchema(pSchema);
			break;
		}
	}
}
void PNCSysSetExportDefault()
{
	char file_name[MAX_PATH];
#ifdef __PNC_
	GetAppPath(file_name);
#else
	strcpy(file_name, "D:\\");
#endif
	strcat(file_name, "rule.set");
	FILE *fp = fopen(file_name, "wt");
	if (fp == NULL)
	{
		AfxMessageBox("�򲻿�ָ���������ļ�!");
		return;
	}
	fprintf(fp, "��������\n");
	fprintf(fp, "bIncDeformed=%s ;���ǻ���������\n", g_pncSysPara.m_bIncDeformed ? "��" : "��");
	fprintf(fp, "PPIMode=%d ;PPI�ļ�ģʽ\n", g_pncSysPara.m_iPPiMode);
	fprintf(fp, "AutoLayout=%d ;�Զ��Ű�\n", g_pncSysPara.m_bAutoLayout);
	fprintf(fp, "MapWidth=%d ;ͼֽ���\n", g_pncSysPara.m_nMapWidth);
	fprintf(fp, "MapLength=%d ;ͼֽ����\n", g_pncSysPara.m_nMapLength);
	fprintf(fp, "MinDistance=%d ;��С���\n", g_pncSysPara.m_nMinDistance);
	fprintf(fp, "m_nMkRectWidth=%d ;�ֺп��\n", g_pncSysPara.m_nMkRectWidth);
	fprintf(fp, "m_nMkRectLen=%d ;�ֺг���\n", g_pncSysPara.m_nMkRectLen);
#ifdef __PNC_
	fprintf(fp, "CDrawDamBoard::BOARD_HEIGHT=%d ;����߶�\n", CDrawDamBoard::BOARD_HEIGHT);
	fprintf(fp, "CDrawDamBoard::m_bDrawAllBamBoard=%d ;�������е���\n", CDrawDamBoard::m_bDrawAllBamBoard);
#endif
	fprintf(fp, "ͼ������\n");
	fprintf(fp, "bIncFilterLayer=%d ;���ù���Ĭ��ͼ��\n", g_pncSysPara.m_iLayerMode);
	fprintf(fp, "RecogMode=%d ;ʶ��ģʽ\n", g_pncSysPara.m_ciRecogMode);
	fprintf(fp, "BoltRecogMode=%d ;��˨ʶ��ģʽ\n", g_pncSysPara.m_ciBoltRecogMode);
	fprintf(fp, "ProfileColorIndex=%d ;��������ɫ\n", g_pncSysPara.m_ciProfileColorIndex);
	fprintf(fp, "BendLineColorIndex=%d ;��������ɫ\n", g_pncSysPara.m_ciBendLineColorIndex);
	fprintf(fp, "ProfileLineType=%s ;����������\n", (char*)g_pncSysPara.m_sProfileLineType);
	fprintf(fp, "����ʶ������\n");
	fprintf(fp, "DimStyle=%d ;���ֱ�ע����\n", g_pncSysPara.m_iDimStyle);
	fprintf(fp, "PnKey=%s ;���ű�ʶ��\n", (char*)g_pncSysPara.m_sPnKey);
	fprintf(fp, "ThickKey=%s ;��ȱ�ʶ��\n", (char*)g_pncSysPara.m_sThickKey);
	fprintf(fp, "MatKey=%s ;���ʱ�ʶ��\n", (char*)g_pncSysPara.m_sMatKey);
	fprintf(fp, "PnNumKey=%s ;������ʶ��\n", (char*)g_pncSysPara.m_sPnNumKey);
	fprintf(fp, "MKPos=%d ;��ȡ��ӡλ��\n", (char*)g_pncSysPara.m_bMKPos);
	fprintf(fp, "AxisXCalType=%d ;X����㷽ʽ\n", (char*)g_pncSysPara.m_iAxisXCalType);
	fprintf(fp, "��˨ʶ������\n");
	for (BOLT_BLOCK *pBoltBlock = g_pncSysPara.hashBoltDList.GetFirst(); pBoltBlock; pBoltBlock = g_pncSysPara.hashBoltDList.GetNext())
	{
		fprintf(fp, "BoltDKey=%s;ͼ������;��˨ֱ��\n", g_pncSysPara.hashBoltDList.GetCursorKey());
		if (!strcmp(pBoltBlock->sGroupName, ""))
			fprintf(fp, "%s;", " ");
		else
			fprintf(fp, "%s;", (char*)pBoltBlock->sGroupName);
		fprintf(fp, "%s;", (char*)pBoltBlock->sBlockName);
		fprintf(fp, "%d;", pBoltBlock->diameter);
		fprintf(fp, "%.1f\n", pBoltBlock->hole_d);
	}
	fprintf(fp, "����ʶ������\n");
	for (RECOG_SCHEMA *pSchema = g_pncSysPara.m_recogSchemaList.GetFirst(); pSchema; pSchema = g_pncSysPara.m_recogSchemaList.GetNext())
	{
		fprintf(fp, "m_iDimStyle=%d;", pSchema->m_iDimStyle);
		fprintf(fp, " %s;", (char*)pSchema->m_sSchemaName);
		fprintf(fp, " %s;", (char*)pSchema->m_sPnKey);
		fprintf(fp, " %s;", (char*)pSchema->m_sThickKey);
		fprintf(fp, " %s;", (char*)pSchema->m_sMatKey);
		fprintf(fp, " %s;", (char*)pSchema->m_sPnNumKey);
		fprintf(fp, " %s;", (char*)pSchema->m_sFrontBendKey);
		fprintf(fp, " %s;", (char*)pSchema->m_sReverseBendKey);
		fprintf(fp, "%d;", pSchema->m_bEditable ? 1 : 0);
		fprintf(fp, "%d\n", pSchema->m_bEnable ? 1 : 0);
	}
	fprintf(fp, "����ʶ��\n");
	fprintf(fp, "MapScale=%.f ;���ű���\n", g_pncSysPara.m_fMapScale);
	fclose(fp);
}

void CPNCSysPara::ActiveRecogSchema(RECOG_SCHEMA *pSchema)
{
	if (pSchema != NULL)
	{
		m_sPnKey.Copy(pSchema->m_sPnKey);
		m_sMatKey.Copy(pSchema->m_sMatKey);
		m_sThickKey.Copy(pSchema->m_sThickKey);
		m_sPnNumKey.Copy(pSchema->m_sPnKey);
		m_sFrontBendKey.Copy(pSchema->m_sFrontBendKey);
		m_sReverseBendKey.Copy(pSchema->m_sReverseBendKey);
		m_iDimStyle = pSchema->m_iDimStyle;
	}
}