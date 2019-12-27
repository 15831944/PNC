#include "StdAfx.h"
#include "BomModel.h"
#include "DefCard.h"
#include "CadToolFunc.h"
#include "ArrayList.h"
#include "PNCCmd.h"
#include "PNCSysPara.h"

#if defined(__UBOM_) || defined(__UBOM_ONLY_)
//////////////////////////////////////////////////////////////////////////
//CAngleProcessInfo
CAngleProcessInfo::CAngleProcessInfo()
{
	keyId=NULL;
	partNumId=NULL;
}
CAngleProcessInfo::~CAngleProcessInfo()
{

}
//���ɽǸֹ��տ�����
void CAngleProcessInfo::CreateRgn()
{
	f3dPoint pt;
	ARRAY_LIST<f3dPoint> profileVertexList;
	pt=f3dPoint(g_pncSysPara.fMinX, g_pncSysPara.fMinY,0)+orig_pt;
	profileVertexList.append(pt);
	scope.VerifyVertex(pt);
	pt=f3dPoint(g_pncSysPara.fMaxX, g_pncSysPara.fMinY,0)+orig_pt;
	profileVertexList.append(pt);
	scope.VerifyVertex(pt);
	pt=f3dPoint(g_pncSysPara.fMaxX, g_pncSysPara.fMaxY,0)+orig_pt;
	profileVertexList.append(pt);
	scope.VerifyVertex(pt);
	pt=f3dPoint(g_pncSysPara.fMinX, g_pncSysPara.fMaxY,0)+orig_pt;
	profileVertexList.append(pt);
	scope.VerifyVertex(pt);
	region.CreatePolygonRgn(profileVertexList.m_pData,profileVertexList.GetSize());
}
//�ж�������Ƿ��ڽǸֹ��տ���
bool CAngleProcessInfo::PtInAngleRgn(const double* poscoord)
{
	if(region.GetAxisZ().IsZero())
		return false;
	int nRetCode=region.PtInRgn(poscoord);
	return nRetCode==1;
}
//�������ݵ����ͻ�ȡ������������
f2dRect CAngleProcessInfo::GetAngleDataRect(BYTE data_type)
{
	f2dRect rect;
	if (data_type == ITEM_TYPE_PART_NO)
	{	//�еļ��Ź������������ַŵ��˷����·����������󷽿������
		rect = g_pncSysPara.part_no_rect;
		rect.bottomRight.y -= g_pncSysPara.part_no_rect.Height();
	}
	else if (data_type == ITEM_TYPE_DES_MAT)
		rect = g_pncSysPara.mat_rect;
	else if (data_type == ITEM_TYPE_DES_GUIGE)
		rect = g_pncSysPara.guige_rect;
	else if (data_type == ITEM_TYPE_LENGTH)
		rect = g_pncSysPara.length_rect;
	else if (data_type == ITEM_TYPE_PIECE_WEIGHT)
		rect = g_pncSysPara.piece_weight_rect;
	else if (data_type == ITEM_TYPE_SUM_PART_NUM)
		rect = g_pncSysPara.jiagong_num_rect;
	else if (data_type == ITEM_TYPE_PART_NUM)
		rect = g_pncSysPara.danji_num_rect;
	else if (data_type == ITEM_TYPE_PART_NOTES)
		rect = g_pncSysPara.note_rect;
	else if (data_type == ITEM_TYPE_SUM_WEIGHT)
		rect = g_pncSysPara.sum_weight_rect;
	rect.topLeft.x+=orig_pt.x;
	rect.topLeft.y+=orig_pt.y;
	rect.bottomRight.x+=orig_pt.x;
	rect.bottomRight.y+=orig_pt.y;
	return rect;
}
//�ж�������Ƿ���ָ�����͵����ݿ���
bool CAngleProcessInfo::PtInDataRect(BYTE data_type,f3dPoint pos)
{
	f2dRect rect=GetAngleDataRect(data_type);
	f2dPoint pt(pos.x,pos.y);
	if(rect.PtInRect(pt))
		return true;
	else
		return false;
}
//��ʼ���Ǹ���Ϣ
BYTE CAngleProcessInfo::InitAngleInfo(f3dPoint data_pos,const char* sValue)
{
	BYTE cType = 0;
	if (PtInDataRect(ITEM_TYPE_PART_NO, data_pos))	//����
	{
		m_xAngle.SetPartNo(sValue);
		cType = ITEM_TYPE_PART_NO;
	}
	else if (PtInDataRect(ITEM_TYPE_DES_MAT, data_pos))	//����
	{
		m_xAngle.cMaterial = CProcessPart::QueryBriefMatMark(sValue);
		if (strlen(sValue) == 5)	//��ʼ�������ȼ� wht 19-09-15
			m_xAngle.cQuality = toupper(sValue[4]);
		cType = ITEM_TYPE_DES_MAT;
	}
	else if(PtInDataRect(ITEM_TYPE_DES_GUIGE,data_pos))	//���
	{	
		CXhChar50 sSpec(sValue);
		if(strstr(sSpec,"��"))
			sSpec.Replace("��","L");
		if(strstr(sSpec,"��"))
			sSpec.Replace("��","*");
		m_xAngle.SetSpec(sSpec);
		int nWidth=0,nThick=0;
		//�ӹ������ȡ���� wht 19-08-05
		CXhChar16 sMaterial;
		CProcessPart::RestoreSpec(sSpec,&nWidth,&nThick,sMaterial);
		if (sMaterial.GetLength() > 0)
		{
			m_xAngle.cMaterial = CProcessPart::QueryBriefMatMark(sMaterial);
			m_xAngle.cQuality = CProcessPart::QueryBriefQuality(sMaterial);
		}
		m_xAngle.m_fWidth=(float)nWidth;
		m_xAngle.m_fThick=(float)nThick;
		cType = ITEM_TYPE_DES_GUIGE;
	}
	else if (PtInDataRect(ITEM_TYPE_LENGTH, data_pos))	//����
	{
		m_xAngle.m_wLength = atoi(sValue);
		cType = ITEM_TYPE_LENGTH;
	}
	else if (PtInDataRect(ITEM_TYPE_PIECE_WEIGHT, data_pos))	//����
	{
		m_xAngle.m_fWeight = (float)atof(sValue);
		cType = ITEM_TYPE_PIECE_WEIGHT;
	}
	else if (PtInDataRect(ITEM_TYPE_PART_NUM, data_pos))	//������
	{
		m_xAngle.m_nDanJiNum = atoi(sValue);
		cType = ITEM_TYPE_PART_NUM;
	}
	else if(PtInDataRect(ITEM_TYPE_SUM_PART_NUM,data_pos))	//�ӹ���
	{
		m_xAngle.feature=atoi(sValue);
		cType = ITEM_TYPE_SUM_PART_NUM;
	}
	else if (PtInDataRect(ITEM_TYPE_PART_NOTES, data_pos))	//��ע
	{
		m_xAngle.SetNotes(sValue);
		cType = ITEM_TYPE_PART_NOTES;
	}
	else if (PtInDataRect(ITEM_TYPE_SUM_WEIGHT, data_pos))	//����
	{
		m_xAngle.m_fSumWeight = (float)atof(sValue);
		cType = ITEM_TYPE_SUM_WEIGHT;
	}
	return cType;
}
//��ȡ�Ǹ����ݵ�����
f3dPoint CAngleProcessInfo::GetAngleDataPos(BYTE data_type)
{
	f2dRect rect=GetAngleDataRect(data_type);
	double fx=(rect.topLeft.x+rect.bottomRight.x)*0.5;
	double fy=(rect.topLeft.y+rect.bottomRight.y)*0.5;
	return f3dPoint(fx,fy,0);
}
//���½Ǹֵļӹ�����
void CAngleProcessInfo::RefreshAngleNum()
{
	GetCurDwg()->setClayer(LayerTable::VisibleProfileLayer.layerId);
	f3dPoint data_pt=GetAngleDataPos(ITEM_TYPE_SUM_PART_NUM);
	CXhChar16 sPartNum("%d",m_xAngle.feature);
	if(partNumId==NULL)
	{	//��ӽǸּӹ���
		acDocManager->lockDocument(curDoc(),AcAp::kWrite,NULL,NULL,true);
		AcDbBlockTableRecord *pBlockTableRecord=GetBlockTableRecord();
		if(pBlockTableRecord==NULL)
		{
			logerr.Log("����ʧ��");
			return;
		}
		DimText(pBlockTableRecord,data_pt,sPartNum,TextStyleTable::hzfs.textStyleId,
			g_pncSysPara.fTextHigh,0,AcDb::kTextCenter,AcDb::kTextVertMid);
		pBlockTableRecord->close();//�رտ��
		acDocManager->unlockDocument(curDoc());
	}
	else
	{	//��д�Ǹּӹ���
		acDocManager->lockDocument(curDoc(),AcAp::kWrite,NULL,NULL,true);
		AcDbEntity *pEnt=NULL;
		acdbOpenAcDbEntity(pEnt,partNumId,AcDb::kForWrite);
		if(pEnt->isKindOf(AcDbText::desc()))
		{
			AcDbText* pText=(AcDbText*)pEnt;
#ifdef _ARX_2007
			pText->setTextString(_bstr_t(sPartNum));
#else
			pText->setTextString(sPartNum);
#endif
		}
		else
		{
			AcDbMText* pMText=(AcDbMText*)pEnt;
#ifdef _ARX_2007
			pMText->setContents(_bstr_t(sPartNum));
#else
			pMText->setContents(sPartNum);
#endif
		}
		pEnt->close();
		acDocManager->unlockDocument(curDoc());
	}
}
void CAngleProcessInfo::RefreshAngleSumWeight()
{
	GetCurDwg()->setClayer(LayerTable::VisibleProfileLayer.layerId);
	f3dPoint data_pt = GetAngleDataPos(ITEM_TYPE_SUM_WEIGHT);
	CXhChar16 sSumWeight("%.f", m_xAngle.m_fSumWeight);
	CLockDocumentLife lockDoc;
	if (sumWeightId == NULL)
	{	//��ӽǸ�����
		AcDbBlockTableRecord *pBlockTableRecord = GetBlockTableRecord();
		if (pBlockTableRecord == NULL)
		{
			logerr.Log("����ʧ��");
			return;
		}
		DimText(pBlockTableRecord, data_pt, sSumWeight, TextStyleTable::hzfs.textStyleId,
			g_pncSysPara.fTextHigh, 0, AcDb::kTextCenter, AcDb::kTextVertMid);
		pBlockTableRecord->close();//�رտ��
	}
	else
	{	//��д�Ǹּӹ���
		AcDbEntity *pEnt = NULL;
		acdbOpenAcDbEntity(pEnt, sumWeightId, AcDb::kForWrite);
		if (pEnt->isKindOf(AcDbText::desc()))
		{
			AcDbText* pText = (AcDbText*)pEnt;
#ifdef _ARX_2007
			pText->setTextString(_bstr_t(sSumWeight));
#else
			pText->setTextString(sSumWeight);
#endif
		}
		else
		{
			AcDbMText* pMText = (AcDbMText*)pEnt;
#ifdef _ARX_2007
			pMText->setContents(_bstr_t(sSumWeight));
#else
			pMText->setContents(sSumWeight);
#endif
		}
		pEnt->close();
	}
}
//////////////////////////////////////////////////////////////////////////
//CDwgFileInfo
CDwgFileInfo::CDwgFileInfo()
{
	m_bJgDwgFile=FALSE;
}
CDwgFileInfo::~CDwgFileInfo()
{

}
//��ʼ��DWG�ļ���Ϣ
BOOL CDwgFileInfo::InitDwgInfo(const char* sFileName,BOOL bJgDxf)
{
	if(strlen(sFileName)<=0)
		return FALSE;
	m_bJgDwgFile=bJgDxf;
	m_sFileName.Copy(sFileName);
	if (m_bJgDwgFile)
		return RetrieveAngles();
	else
		return RetrievePlates();
}
//////////////////////////////////////////////////////////////////////////
//�ְ�DWG����
//////////////////////////////////////////////////////////////////////////
//�������ݵ�������Ҷ�Ӧ�ĸְ�
CPlateProcessInfo* CDwgFileInfo::FindPlateByPt(f3dPoint text_pos)
{
	CPlateProcessInfo* pPlateInfo=NULL;
	for(pPlateInfo=m_xPncMode.EnumFirstPlate(FALSE);pPlateInfo;pPlateInfo=m_xPncMode.EnumNextPlate(FALSE))
	{
		if(pPlateInfo->IsInPlate(text_pos))
			break;
	}
	return pPlateInfo;
}
//���ݼ��Ų��Ҷ�Ӧ�ĸְ�
CPlateProcessInfo* CDwgFileInfo::FindPlateByPartNo(const char* sPartNo)
{
	return m_xPncMode.PartFromPartNo(sPartNo);
}
//���¸ְ�ӹ�����
void CDwgFileInfo::ModifyPlateDwgPartNum()
{
	if(m_xPncMode.GetPlateNum()<=0)
		return;
	CPlateProcessInfo* pInfo=NULL;
	CProcessPlate* pProcessPlate=NULL;
	BOOL bFinish=TRUE;
	for(pInfo=EnumFirstPlate();pInfo;pInfo=EnumNextPlate())
	{
		CXhChar16 sPartNo=pInfo->xPlate.GetPartNo();
		pProcessPlate=(CProcessPlate*)m_pProject->m_xLoftBom.FindPart(sPartNo);
		if(pProcessPlate==NULL)
		{
			bFinish=FALSE;
			logerr.Log("TMA�������ϱ���û��%s�ְ�",(char*)sPartNo);
			continue;
		}
		if(pInfo->partNumId==NULL)
		{
			bFinish=FALSE;
			logerr.Log("%s�ְ�����޸�ʧ��!",(char*)sPartNo);
			continue;
		}
		if(pInfo->xPlate.feature!=pProcessPlate->feature)
		{	//�ӹ�����ͬ�����޸�
			pInfo->xPlate.feature=pProcessPlate->feature;	//�ӹ���
			pInfo->RefreshPlateNum();
		}
	}
	if(bFinish)
		AfxMessageBox("�ְ�ӹ����޸����!");
}
//�õ���ʾͼԪ����
int CDwgFileInfo::GetDrawingVisibleEntSet(CHashSet<AcDbObjectId> &entSet)
{
	entSet.Empty();
	long ll=0;
	AcDbEntity *pEnt=NULL;
	AcDbObjectId entId;
	ads_name ent_sel_set,entname;
#ifdef _ARX_2007
	acedSSGet(L"ALL",NULL,NULL,NULL,ent_sel_set);
#else
	acedSSGet("ALL",NULL,NULL,NULL,ent_sel_set);
#endif
	acedSSLength(ent_sel_set,&ll);
	for(long i=0;i<ll;i++)
	{	//��ʼ��ʵ�弯��
		acedSSName(ent_sel_set,i,entname);
		acdbGetObjectId(entId,entname);
		acdbOpenAcDbEntity(pEnt,entId,AcDb::kForRead);
		if(pEnt==NULL)
			continue;
		entSet.SetValue(entId.handle(),entId);
		pEnt->close();
	}
	acedSSFree(ent_sel_set);
	return entSet.GetNodeNum();
}
//��ȡʵ�������ID
AcDbObjectId CDwgFileInfo::GetEntLineTypeId(AcDbEntity *pEnt)
{
	if (pEnt == NULL)
		return NULL;
	CXhChar50 sLineTypeName;
#ifdef _ARX_2007
	ACHAR* sValue = new ACHAR[50];
	sValue = pEnt->linetype();
	sLineTypeName.Copy((char*)_bstr_t(sValue));
	delete[] sValue;
#else
	char *sValue = new char[50];
	sValue = pEnt->linetype();
	sLineTypeName.Copy(sValue);
	delete[] sValue;
#endif
	AcDbObjectId linetypeId;
	if (stricmp(sLineTypeName, "ByLayer") == 0)
	{	//�������
		AcDbLayerTableRecord *pLayerTableRecord;
		acdbOpenObject(pLayerTableRecord, pEnt->layerId(), AcDb::kForRead);
		pLayerTableRecord->close();
		linetypeId = pLayerTableRecord->linetypeObjectId();
	}
	else if (stricmp(sLineTypeName, "ByBlock") == 0)
		linetypeId = m_idSolidLine;		//���ͼԪ����������ΪByBlock,�����;���ʵ��
	else
		linetypeId = pEnt->linetypeId();
	return linetypeId;
}

//��ȡ���������,ȷ���պ�����
BOOL CDwgFileInfo::RetrievePlates()
{
	//SmartExtractPlate();
	return TRUE;
}

BOOL CDwgFileInfo::ReviseThePlate(const char* sPartNo)
{
	return TRUE;
}
//////////////////////////////////////////////////////////////////////////
//�Ǹ�DWG�ļ�����
//////////////////////////////////////////////////////////////////////////
void CDwgFileInfo::CorrectAngles()
{
	CAngleProcessInfo* pJgInfo=NULL;
	for(pJgInfo=m_hashJgInfo.GetFirst();pJgInfo;pJgInfo=m_hashJgInfo.GetNext())
	{
		CXhChar16 sPartNo=pJgInfo->m_xAngle.GetPartNo();
		if(sPartNo.GetLength()<=0)
			m_hashJgInfo.DeleteNode(pJgInfo->keyId.handle());
		if (pJgInfo->m_xAngle.cMaterial == 0)
			pJgInfo->m_xAngle.cMaterial = 'S';
	}
	m_hashJgInfo.Clean();
}
//�������ݵ������������Ӧ�Ǹ�
CAngleProcessInfo* CDwgFileInfo::FindAngleByPt(f3dPoint data_pos)
{
	CAngleProcessInfo* pJgInfo=NULL;
	for(pJgInfo=m_hashJgInfo.GetFirst();pJgInfo;pJgInfo=m_hashJgInfo.GetNext())
	{
		if(pJgInfo->PtInAngleRgn(data_pos))
			break;
	}
	return pJgInfo;
}
//���ݼ��Ų��Ҷ�Ӧ�ĽǸ�
CAngleProcessInfo* CDwgFileInfo::FindAngleByPartNo(const char* sPartNo)
{
	CAngleProcessInfo* pJgInfo=NULL;
	for(pJgInfo=m_hashJgInfo.GetFirst();pJgInfo;pJgInfo=m_hashJgInfo.GetNext())
	{
		if(stricmp(pJgInfo->m_xAngle.GetPartNo(),sPartNo)==0)
			break;
	}
	return pJgInfo;
}
//���½Ǹּӹ���
void CDwgFileInfo::ModifyAngleDwgPartNum()
{
	if(m_hashJgInfo.GetNodeNum()<=0)
		return;
	CAngleProcessInfo* pJgInfo=NULL;
	CProcessAngle* pProcessJg=NULL;
	BOOL bFinish=TRUE;
	for(pJgInfo=EnumFirstJg();pJgInfo;pJgInfo=EnumNextJg())
	{
		CXhChar16 sPartNo=pJgInfo->m_xAngle.GetPartNo();
		pProcessJg=(CProcessAngle*)m_pProject->m_xLoftBom.FindPart(sPartNo);
		if(pProcessJg==NULL)
		{	
			bFinish=FALSE;
			logerr.Log("TMA���ϱ���û��%s�Ǹ�",(char*)sPartNo);
			continue;
		}
		pJgInfo->m_xAngle.feature=pProcessJg->feature;	//�ӹ���
		pJgInfo->m_xAngle.m_fSumWeight = pProcessJg->m_fSumWeight;
		pJgInfo->RefreshAngleNum();
		pJgInfo->RefreshAngleSumWeight();
	}
	if(bFinish)
		AfxMessageBox("�Ǹּӹ����޸����!");
}
//��ȡ�Ǹֲ���
BOOL CDwgFileInfo::RetrieveAngles()
{
	CAcModuleResourceOverride resOverride;
	ads_name ent_sel_set,entname;
	CHashSet<AcDbObjectId> textIdHash;
	//���ݹ��տ���ʶ��Ǹ�
#ifdef _ARX_2007
	acedSSGet(L"ALL",NULL,NULL,NULL,ent_sel_set);
#else
	acedSSGet("ALL",NULL,NULL,NULL,ent_sel_set);
#endif
	AcDbEntity *pEnt=NULL;
	AcDbObjectId entId,blockId;
	long ll;
	f3dPoint orig_pt;
	acedSSLength(ent_sel_set,&ll);
	for(long i=0;i<ll;i++)
	{	
		acedSSName(ent_sel_set,i,entname);
		acdbGetObjectId(entId,entname);
		acdbOpenAcDbEntity(pEnt,entId,AcDb::kForRead);
		if(pEnt==NULL)
			continue;
		pEnt->close();
		CXhChar50 sText;
		if(pEnt->isKindOf(AcDbText::desc()))
		{	//�Ǹֹ��տ��ǿ飬����"����"������ȡ�Ǹ���Ϣ
			textIdHash.SetValue(entId.handle(),entId);
			AcDbText* pText=(AcDbText*)pEnt;
#ifdef _ARX_2007
			sText.Copy(_bstr_t(pText->textString()));
#else
			sText.Copy(pText->textString());
#endif
			if((strstr(sText,"��")==NULL&&strstr(sText,"��")==NULL) || strstr(sText,"��")==NULL)
				continue;
			if (strstr(sText, "ͼ��") != NULL || strstr(sText, "�ļ����") != NULL)
				continue;
			orig_pt= g_pncSysPara.GetJgCardOrigin(f3dPoint(pText->position().x,pText->position().y,0));
		}
		else if(pEnt->isKindOf(AcDbMText::desc()))
		{	//�Ǹֹ��տ��ǿ飬����"����"������ȡ�Ǹ���Ϣ
			textIdHash.SetValue(entId.handle(),entId);
			AcDbMText* pMText=(AcDbMText*)pEnt;
#ifdef _ARX_2007
			sText.Copy(_bstr_t(pMText->contents()));
#else
			sText.Copy(pMText->contents());
#endif
			if ((strstr(sText, "��") == NULL && strstr(sText, "��") == NULL) || strstr(sText, "��") == NULL)
				continue;
			if (strstr(sText, "ͼ��") != NULL || strstr(sText, "�ļ����") != NULL)
				continue;
			orig_pt= g_pncSysPara.GetJgCardOrigin(f3dPoint(pMText->location().x,pMText->location().y,0));
		}
		else if(pEnt->isKindOf(AcDbBlockReference::desc()))
		{	//���ݽǸֹ��տ�����ȡ�Ǹ���Ϣ
			AcDbBlockTableRecord *pTempBlockTableRecord=NULL;
			AcDbBlockReference* pReference=(AcDbBlockReference*)pEnt;
			blockId=pReference->blockTableRecord();
			acdbOpenObject(pTempBlockTableRecord,blockId,AcDb::kForRead);
			if(pTempBlockTableRecord==NULL)
				continue;
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
			if(strcmp(sName,"JgCard")!=0)
				continue;
			orig_pt.Set(pReference->position().x,pReference->position().y,0);
		}
		else
			continue;
		//��ӽǸּ�¼
		CAngleProcessInfo* pJgInfo=NULL;
		pJgInfo=m_hashJgInfo.Add(entId.handle());
		pJgInfo->keyId=entId;
		pJgInfo->m_xAngle.cMaterial='S';
		pJgInfo->SetOrig(orig_pt);
		pJgInfo->CreateRgn();
	}
	acedSSFree(ent_sel_set);
	if(m_hashJgInfo.GetNodeNum()<=0)
	{	
		logerr.Log("%s�ļ���ȡ�Ǹ�ʧ��",(char*)m_sFileName);
		return FALSE;
	}
	//���ݽǸ�����λ�û�ȡ�Ǹ���Ϣ
	for(AcDbObjectId objId=textIdHash.GetFirst();objId;objId=textIdHash.GetNext())
	{
		acdbOpenAcDbEntity(pEnt,objId,AcDb::kForRead);
		if(pEnt==NULL)
			continue;
		pEnt->close();
		CXhChar50 sValue;
		f3dPoint text_pos;
		if(pEnt->isKindOf(AcDbText::desc()))
		{
			AcDbText* pText=(AcDbText*)pEnt;
			text_pos.Set(pText->alignmentPoint().x,pText->alignmentPoint().y,pText->alignmentPoint().z);
			if(text_pos.IsZero())
				text_pos.Set(pText->position().x,pText->position().y,pText->position().z);
#ifdef _ARX_2007
			sValue.Copy(_bstr_t(pText->textString()));
#else
			sValue.Copy(pText->textString());
#endif
		}
		else
		{
			AcDbMText* pMText=(AcDbMText*)pEnt;
			text_pos.Set(pMText->location().x,pMText->location().y,pMText->location().z);//contents
#ifdef _ARX_2007
			sValue.Copy(_bstr_t(pMText->contents()));
#else
			sValue.Copy(pMText->contents());
#endif
		}
		if(strlen(sValue)<=0)	//���˿��ַ�
			continue;
		CAngleProcessInfo* pJgInfo=FindAngleByPt(text_pos);
		if (pJgInfo)
		{
			BYTE cType = pJgInfo->InitAngleInfo(text_pos, sValue);
			if (cType == ITEM_TYPE_SUM_PART_NUM)
				pJgInfo->partNumId = objId;
			else if (cType == ITEM_TYPE_SUM_WEIGHT)
				pJgInfo->sumWeightId = objId;
		}
	}
	//����ȡ�ĽǸ���Ϣ���к����Լ��
	CorrectAngles();
	if(m_hashJgInfo.GetNodeNum()<=0)
	{	
		logerr.Log("%s�ļ���ȡ�Ǹ�ʧ��",(char*)m_sFileName);
		return FALSE;
	}
	return TRUE;
}
#endif