#include "StdAfx.h"
#include "BomModel.h"
#include "DefCard.h"
#include "CadToolFunc.h"
#include "ArrayList.h"
#include "..\ConvertToNC\PNCCmd.h"

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
	pt=f3dPoint(g_xUbomModel.manager.fMinX,g_xUbomModel.manager.fMinY,0)+orig_pt;
	profileVertexList.append(pt);
	scope.VerifyVertex(pt);
	pt=f3dPoint(g_xUbomModel.manager.fMaxX,g_xUbomModel.manager.fMinY,0)+orig_pt;
	profileVertexList.append(pt);
	scope.VerifyVertex(pt);
	pt=f3dPoint(g_xUbomModel.manager.fMaxX,g_xUbomModel.manager.fMaxY,0)+orig_pt;
	profileVertexList.append(pt);
	scope.VerifyVertex(pt);
	pt=f3dPoint(g_xUbomModel.manager.fMinX,g_xUbomModel.manager.fMaxY,0)+orig_pt;
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
	if(data_type==ITEM_TYPE_PART_NO)	
	{	//�еļ��Ź������������ַŵ��˷����·����������󷽿������
		rect=g_xUbomModel.manager.part_no_rect;
		rect.bottomRight.y-=g_xUbomModel.manager.part_no_rect.Height();
	}
	else if(data_type==ITEM_TYPE_DES_MAT)
		rect=g_xUbomModel.manager.mat_rect;
	else if(data_type==ITEM_TYPE_DES_GUIGE)
		rect=g_xUbomModel.manager.guige_rect;
	else if(data_type==ITEM_TYPE_LENGTH)
		rect=g_xUbomModel.manager.length_rect;
	else if(data_type==ITEM_TYPE_PIECE_WEIGHT)
		rect=g_xUbomModel.manager.piece_weight_rect;
	else if(data_type==ITEM_TYPE_SUM_PART_NUM)
		rect=g_xUbomModel.manager.jiagong_num_rect;
	else if(data_type==ITEM_TYPE_PART_NUM)
		rect=g_xUbomModel.manager.danji_num_rect;
	else if(data_type==ITEM_TYPE_PART_NOTES)
		rect=g_xUbomModel.manager.note_rect;
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
void CAngleProcessInfo::InitAngleInfo(f3dPoint data_pos,const char* sValue,BOOL& bPartNum/*=FALSE*/)
{
	if(PtInDataRect(ITEM_TYPE_PART_NO,data_pos))	//����
		m_xAngle.SetPartNo(sValue);
	else if(PtInDataRect(ITEM_TYPE_DES_MAT,data_pos))	//����
		m_xAngle.cMaterial=CBomModel::QueryBriefMatMark(sValue);
	else if(PtInDataRect(ITEM_TYPE_DES_GUIGE,data_pos))	//���
	{	
		CXhChar50 sSpec(sValue);
		if(strstr(sSpec,"��"))
			sSpec.Replace("��","L");
		if(strstr(sSpec,"��"))
			sSpec.Replace("��","*");
		m_xAngle.SetSpec(sSpec);
		int nWidth=0,nThick=0;
		CBomModel::RestoreSpec(sSpec,&nWidth,&nThick);
		m_xAngle.m_fWidth=(float)nWidth;
		m_xAngle.m_fThick=(float)nThick;
	}
	else if(PtInDataRect(ITEM_TYPE_LENGTH,data_pos))	//����
		m_xAngle.m_wLength=atoi(sValue);
	else if(PtInDataRect(ITEM_TYPE_PIECE_WEIGHT,data_pos))	//����
		m_xAngle.m_fWeight=(float)atof(sValue);
	else if(PtInDataRect(ITEM_TYPE_PART_NUM,data_pos))	//������
		m_xAngle.m_nDanJiNum=atoi(sValue);
	else if(PtInDataRect(ITEM_TYPE_SUM_PART_NUM,data_pos))	//�ӹ���
	{
		m_xAngle.feature=atoi(sValue);
		bPartNum=TRUE;
	}
	else if(PtInDataRect(ITEM_TYPE_PART_NOTES,data_pos))	//��ע
		m_xAngle.SetNotes(sValue);
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
			g_xUbomModel.manager.fTextHigh,0,AcDb::kTextCenter,AcDb::kTextVertMid);
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
//////////////////////////////////////////////////////////////////////////
//CPlateProcessInfo
/*
CPlateProcessInfo::CPlateProcessInfo()
{
	partNumId=NULL;
	partNoId=NULL;
}
CPlateProcessInfo::~CPlateProcessInfo()
{

}
//���ɸְ�����
void CPlateProcessInfo::CreateRgn()
{
	if(vertexList.GetNodeNum()<=0)
	{	//�ְ���������ʧ�ܣ������ֱ�ע��Ϊ����
		AcDbEntity *pEnt=NULL;
		acdbOpenAcDbEntity(pEnt,partNoId,AcDb::kForRead);
		if(pEnt==NULL)
			return;
		pEnt->close();
		CXhChar50 sText;
		AcDbText* pText=(AcDbText*)pEnt;
#ifdef _ARX_2007
		sText.Copy(_bstr_t(pText->textString()));
#else
		sText.Copy(pText->textString());
#endif
		f3dPoint text_norm,dim_norm,dim_vec(1,0,0);
		Cpy_Pnt(text_norm,pText->normal());
		if(text_norm.IsZero())
			text_norm.Set(0,0,1);
		double height=pText->height();
		double angle=pText->rotation();
		double len=DrawTextLength(sText,height,pText->textStyle());
		dim_norm.Set(-sin(angle),cos(angle));
		RotateVectorAroundVector(dim_vec,angle,text_norm);
		//
		ARRAY_LIST<f3dPoint> vertexList;
		vertexList.append(f3dPoint(pn_dim_pos+dim_norm*height*1.5-dim_vec*len*2));
		vertexList.append(f3dPoint(pn_dim_pos-dim_norm*height*2-dim_vec*len*2));
		vertexList.append(f3dPoint(pn_dim_pos-dim_norm*height*2+dim_vec*len*2));
		vertexList.append(f3dPoint(pn_dim_pos+dim_norm*height*1.5+dim_vec*len*2));
		for(int i=0;i<vertexList.GetSize();i++)
			scope.VerifyVertex(vertexList[i]);
		region.CreatePolygonRgn(vertexList.m_pData,vertexList.GetSize());
	}
	else
	{	//��ʼ���ְ�����������
		ARRAY_LIST<f3dPoint> profileVertexList;
		for(PROFILE_VER* pVer=vertexList.GetFirst();pVer;pVer=vertexList.GetNext())
		{
			profileVertexList.append(pVer->vertex);
			scope.VerifyVertex(pVer->vertex);
		}
		region.CreatePolygonRgn(profileVertexList.m_pData,profileVertexList.GetSize());
	}
}
//�ж�������Ƿ��ڸְ���
bool CPlateProcessInfo::PtInPlateRgn(const double* poscoord)
{
	if(region.GetAxisZ().IsZero())
		return false;
	int nRetCode=region.PtInRgn(poscoord);
	return nRetCode==1;
}
//���Ƹְ�����������
void CPlateProcessInfo::DrawPlate()
{
	f3dPoint prePt,curPt;
	AcDbBlockTableRecord *pBlockTableRecord=GetBlockTableRecord();
	if(pBlockTableRecord==NULL)
		return;
	AcDbObjectId entId;
	int nNum=region.GetVertexCount();
	prePt=region.GetVertexAt(nNum-1);
	for(int i=0;i<nNum;i++)
	{
		curPt=region.GetVertexAt(i);
		AcDbLine* pLine=new AcDbLine(AcGePoint3d(prePt.x,prePt.y,prePt.z),AcGePoint3d(curPt.x,curPt.y,curPt.z));
		pLine->setColorIndex(2);
		pBlockTableRecord->appendAcDbEntity(entId,pLine);
		pLine->close();
		prePt=curPt;
	}
	pBlockTableRecord->close();
}
//������ѡͼԪ��ʼ���ְ�������
BOOL CPlateProcessInfo::InitProfileBySelEnts(CHashSet<AcDbObjectId>& selectedEntSet)
{	//1.����ѡ��ʵ���ʼ��ֱ���б�
	ATOM_LIST<LINE_OBJECT> objectLineList;
	AcDbEntity *pEnt=NULL;
	AcGePoint3d pt;
	for(AcDbObjectId objId=selectedEntSet.GetFirst();objId;objId=selectedEntSet.GetNext())
	{
		acdbOpenAcDbEntity(pEnt,objId,AcDb::kForRead);
		if(pEnt==NULL)
			continue;
		pEnt->close();
		if(pEnt->isKindOf(AcDbLine::desc()) || pEnt->isKindOf(AcDbArc::desc()) 
			|| pEnt->isKindOf(AcDbEllipse::desc()))
		{	//�ж������Ƿ�Ϊʵ��
			AcDbCurve* pCurve=(AcDbCurve*)pEnt;
			f3dPoint startPt,endPt;
			pCurve->getStartPoint(pt);
			Cpy_Pnt(startPt,pt);
			pCurve->getEndPoint(pt);
			Cpy_Pnt(endPt,pt);
			LINE_OBJECT* pLine=NULL;
			for(pLine=objectLineList.GetFirst();pLine;pLine=objectLineList.GetNext())
			{
				if((pLine->startPt.IsEqual(startPt,EPS2)&&pLine->endPt.IsEqual(endPt,EPS2))||
					(pLine->startPt.IsEqual(endPt,EPS2)&&pLine->endPt.IsEqual(startPt,EPS2)))
					break;
			}
			if(pLine==NULL)
			{
				pLine=objectLineList.append();
				pLine->startPt=startPt;
				pLine->endPt=endPt;
				pLine->midPt=0.5*(startPt+endPt);
			}
		}
	}
	if(objectLineList.GetNodeNum()<3)
		return FALSE;
	//2.��ʼ���ְ�������
	//2.1 ���ݸְ弸�����ģ������ߵķ���(���ڰ��㴦��׼)
	int nNum=0;
	f3dPoint geom_center;
	for(LINE_OBJECT* pLine=objectLineList.GetFirst();pLine;pLine=objectLineList.GetNext(),nNum++)
		geom_center+=pLine->midPt;
	geom_center/=nNum;
	for(LINE_OBJECT* pLine=objectLineList.GetFirst();pLine;pLine=objectLineList.GetNext())
	{
		f3dPoint startPt=pLine->startPt;
		f3dPoint endPt=pLine->endPt;
		if(DistOf2dPtLine(geom_center,startPt,endPt)<-eps)
		{
			pLine->startPt=endPt;
			pLine->endPt=startPt;
		}
	}
	//2.2 ���ݸְ�ıպ��ԣ���ȡ������
	vertexList.Empty();
	PROFILE_VER xVertex;
	xVertex.type=1;
	LINE_OBJECT* pFirLine=objectLineList.GetFirst();
	f3dPoint startPt,endPt;
	BOOL bFinish=FALSE;
	for(;;)
	{
		if(pFirLine&&pFirLine->bMatch==FALSE)
		{
			pFirLine->bMatch=TRUE;
			startPt=pFirLine->startPt;
			endPt=pFirLine->endPt;
			xVertex.vertex=endPt;
			vertexList.append(xVertex);
		}
		if(fabs(DISTANCE(startPt,endPt))<EPS2)
		{
			bFinish=TRUE;
			break;
		}
		LINE_OBJECT* pLine=NULL;
		for(pLine=objectLineList.GetFirst();pLine;pLine=objectLineList.GetNext())
		{
			if(pLine->bMatch)
				continue;
			if(fabs(DISTANCE(pLine->startPt,endPt))<EPS2)
			{
				endPt=pLine->endPt;
				pLine->bMatch=TRUE;
				xVertex.vertex=pLine->endPt;
				vertexList.append(xVertex);
				break;
			}
			else if(fabs(DISTANCE(pLine->endPt,endPt))<EPS2)
			{	//
				endPt=pLine->startPt;
				pLine->bMatch=TRUE;
				xVertex.vertex=pLine->startPt;
				vertexList.append(xVertex);
				break;
			}
		}
		if(pLine==NULL)
			break;
	}
	if(bFinish==FALSE)
		return FALSE;
	CreateRgn();
	if(!PtInPlateRgn(pn_dim_pos))
	{
		vertexList.Empty();
		return FALSE;
	}
	return TRUE;
}
//
void CPlateProcessInfo::UpdateVertexs(AcGeCircArc3d arc)
{
	f3dPoint startPt,endPt,centerPt,vertex,vec;
	startPt.Set(arc.startPoint().x,arc.startPoint().y,0);
	endPt.Set(arc.endPoint().x,arc.endPoint().y,0);
	centerPt.Set(arc.center().x,arc.center().y,0);
	vertex=(startPt+endPt)*0.5;
	vec=vertex-centerPt;
	normalize(vec);
	if(vec.IsZero())
		return;
	double angle=arc.endAng()-arc.startAng();
	if(angle>3.14)
		vertex=centerPt-vec*arc.radius();
	else
		vertex=centerPt+vec*arc.radius();
	BOOL bFind=FALSE;
	int i=0,iStart=-1,iEnd=-1;
	PROFILE_VER* pVer=NULL;
	for(pVer=vertexList.GetFirst();pVer;pVer=vertexList.GetNext())
	{
		if(pVer->vertex.IsEqual(startPt,0.1))
			iStart=i;
		else if(pVer->vertex.IsEqual(endPt,0.1))
			iEnd=i;
		if(iStart>-1&&iEnd>-1)
		{
			bFind=TRUE;
			break;
		}
		i++;		
	}
	if(bFind==FALSE)
		return;
	int nNum=vertexList.GetNodeNum();
	if(iStart>iEnd && (iStart+1)%nNum!=iEnd)
	{	//
		int index=iEnd;
		iEnd=iStart;
		iStart=index;
	}
	vertexList.insert(PROFILE_VER(vertex),iEnd);
}
//�����ظ���
static BOOL IsHasVertex(ATOM_LIST<PROFILE_VER> &tem_vertes,GEPOINT pt)
{
	for(int i=0;i<tem_vertes.GetNodeNum();i++)
	{
		PROFILE_VER* pVer=tem_vertes.GetByIndex(i);
		if(pVer->vertex.IsEqual(pt,EPS2))
			return TRUE;
	}
	return FALSE;
}
//����bpoly�����ȡ�ְ��������
void CPlateProcessInfo::InitProfileByBPolyCmd()
{
	ads_name seqent;
	AcDbObjectId initLastObjId,plineId;
	acdbEntLast(seqent);
	acdbGetObjectId(initLastObjId,seqent);
	f2dRect rect;
	rect.topLeft.Set(pn_dim_pos.x-10,pn_dim_pos.y+10);
	rect.bottomRight.Set(pn_dim_pos.x+10,pn_dim_pos.y-10);
	//�����ű�עλ�ý�������,�ҵ����ʵ����ű���,ͨ��boundary������ȡ������
	int externlen=2200;
	CString cmd_str;
	for(int i=0;i<=10;i++)
	{
		externlen-=200;
		ZoomAcadView(rect,externlen);	
		cmd_str.Format("-boundary %.2f,%.2f\n ",pn_dim_pos.x,pn_dim_pos.y);
		CBomModel::SendCommandToCad(cmd_str);	//ִ��boundary����
		acdbEntLast(seqent);
		acdbGetObjectId(plineId,seqent);
		if(initLastObjId!=plineId)
			break;
	}
	if(initLastObjId==plineId)
	{	//ִ��boundaryδ����ʵ��(��δ�ҵ���Ч�ķ������)
		CreateRgn();
		return;	
	}
	AcDbEntity *pEnt=NULL;
	acdbOpenAcDbEntity(pEnt,plineId,AcDb::kForWrite);
	if(pEnt==NULL||!pEnt->isKindOf(AcDbPolyline::desc()))
	{
		if(pEnt)
			pEnt->close();
		return;
	}
	//��ȡ�ְ�������
	AcDbPolyline *pPline=(AcDbPolyline*)pEnt;
	ATOM_LIST<PROFILE_VER> tem_vertes;
	PROFILE_VER *pVer=NULL;
	int nVertNum = pPline->numVerts();
	for(int iVertIndex = 0;iVertIndex<nVertNum;iVertIndex++)
	{
		AcGePoint3d location;
		pPline->getPointAt(iVertIndex,location);
		if(IsHasVertex(tem_vertes,GEPOINT(location.x,location.y,location.z)))
			continue;
		f3dPoint startPt,endPt;
		if(pPline->segType(iVertIndex)==AcDbPolyline::kLine)
		{
			AcGeLineSeg3d acgeLine;
			pPline->getLineSegAt(iVertIndex,acgeLine);
			Cpy_Pnt(startPt,acgeLine.startPoint());
			Cpy_Pnt(endPt,acgeLine.endPoint());
			pVer=tem_vertes.append();
			pVer->type=1;
			pVer->vertex.Set(location.x,location.y,location.z);
		}
		else if(pPline->segType(iVertIndex)==AcDbPolyline::kArc)
		{
			AcGeCircArc3d acgeArc;
			pPline->getArcSegAt(iVertIndex,acgeArc);
			f3dPoint center,norm;
			Cpy_Pnt(startPt,acgeArc.startPoint());
			Cpy_Pnt(endPt,acgeArc.endPoint());
			Cpy_Pnt(center,acgeArc.center());
			Cpy_Pnt(norm,acgeArc.normal());
			pVer=tem_vertes.append();
			pVer->vertex.Set(location.x,location.y,location.z);
			pVer->type=2;
			pVer->center=center;
			pVer->work_norm=norm;
			pVer->radius=acgeArc.radius();
		}
	}
	pPline->erase(Adesk::kTrue);	//ɾ��polyline����
	pPline->close();
	//
	for(int i=0;i<tem_vertes.GetNodeNum();i++)
	{
		PROFILE_VER* pCurVer=tem_vertes.GetByIndex(i);
		if(pCurVer->type==2&&ftoi(pCurVer->radius)==2)
		{
			PROFILE_VER* pNextVer=tem_vertes.GetByIndex((i+1)%nVertNum);
			if(pNextVer->type==1)
				pNextVer->vertex=pCurVer->center;
			else if(pNextVer->type==2&&ftoi(pNextVer->radius)!=2)
				pNextVer->vertex=pCurVer->center;
		}
	}
	for(pVer=tem_vertes.GetFirst();pVer;pVer=tem_vertes.GetNext())
	{
		if(pVer->type==2&&ftoi(pVer->radius)==2)
			tem_vertes.DeleteCursor();
	}
	tem_vertes.Clean();
	//���ְ��������
	vertexList.Empty();
	for(int i=0;i<tem_vertes.GetNodeNum();i++)
	{
		PROFILE_VER* pCur=tem_vertes.GetByIndex(i);
		pVer=vertexList.append(*pCur);
		if(pVer->type==2)
		{	//��Բ����Ϊ�����߶Σ���С�ְ������������
			pVer->type=1;
			PROFILE_VER* pNext=tem_vertes.GetByIndex((i+1)%tem_vertes.GetNodeNum());
			f3dPoint startPt=pCur->vertex,endPt=pNext->vertex;
			if(startPt.IsEqual(endPt,EPS2))
				continue;
			f3dArcLine arcLine;
			if(arcLine.CreateMethod3(startPt,endPt,pCur->work_norm,pCur->radius,pCur->center))
			{
				int nSlices=CalArcResolution(arcLine.Radius(),arcLine.SectorAngle(),1.0,5.0,18);
				double angle=arcLine.SectorAngle()/nSlices;
				for(int i=1;i<nSlices;i++)
				{
					PROFILE_VER *pProVer= vertexList.append();
					pProVer->vertex=arcLine.PositionInAngle(angle*i);
				}
			}
		}
	}
	CreateRgn();
}
//���¸ְ�ӹ���
void CPlateProcessInfo::RefreshPlateNum()
{
	if(partNumId==NULL)
		return;
	GetCurDwg()->setClayer(LayerTable::VisibleProfileLayer.layerId);
	acDocManager->lockDocument(curDoc(),AcAp::kWrite,NULL,NULL,true);
	AcDbEntity *pEnt=NULL;
	acdbOpenAcDbEntity(pEnt,partNumId,AcDb::kForWrite);
	AcDbText* pText=(AcDbText*)pEnt;
	CXhChar100 sValueG,sValueS,sValueM;
#ifdef _ARX_2007
	sValueG.Copy(_bstr_t(pText->textString()));
#else
	sValueG.Copy(pText->textString());
#endif
	for(char* sKey=strtok(sValueG," \t");sKey;sKey=strtok(NULL," \t"))
	{
		if(strstr(sKey,"Q"))
		{
			sValueM.Copy(sKey);
			break;
		}
	}
	sValueS.Printf("-%.0f %s %d��",m_xPlate.m_fThick,(char*)sValueM,m_xPlate.feature);
#ifdef _ARX_2007
	pText->setTextString(_bstr_t(sValueS));
#else
	pText->setTextString(sValueS);
#endif
	pEnt->close();
	acDocManager->unlockDocument(curDoc());
}
//
void CPlateProcessInfo::ClonePlateInfo(CPlateProcessInfo* pPlateInfo)
{
	m_xPlate.cMaterial=pPlateInfo->m_xPlate.cMaterial;
	m_xPlate.m_fThick=pPlateInfo->m_xPlate.m_fThick;
	m_xPlate.feature=pPlateInfo->m_xPlate.feature;
	partNumId=pPlateInfo->partNumId;
	//��ʼ���ְ�����
	ARRAY_LIST<f3dPoint> profileVertexList;
	for(PROFILE_VER* pVer=pPlateInfo->vertexList.GetFirst();pVer;pVer=pPlateInfo->vertexList.GetNext())
	{
		profileVertexList.append(pVer->vertex);
		scope.VerifyVertex(pVer->vertex);
	}
	region.CreatePolygonRgn(profileVertexList.m_pData,profileVertexList.GetSize());
}
*/
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
	if(m_bJgDwgFile)
		return RetrieveAngles();
	else
		return RetrievePlates();
}
//////////////////////////////////////////////////////////////////////////
//�ְ�DWG����
//////////////////////////////////////////////////////////////////////////
void CDwgFileInfo::CorrectPlates()
{
	//1.��ȥ�ظ�����
	/*
	CHashStrList<long> hashPlateKeyList;
	for(CPlateProcessInfo* pPlateInfo=m_xPncMode.EnumFirstPlate(FALSE);pPlateInfo;pPlateInfo= m_xPncMode.EnumFirstPlate(FALSE))
	{
		int nPush=m_xPncMode.PushPlateStack();
		CXhChar16 sPartNo=pPlateInfo->xPlate.GetPartNo();
		long *keyId=hashPlateKeyList.GetValue(sPartNo);
		if(keyId==NULL)
			hashPlateKeyList.SetValue(sPartNo,pPlateInfo->partNoId.handle());
		else
		{	//ȥ��
			if(pPlateInfo->xPlate.m_fThick>0)
			{
				m_hashPlateInfo.DeleteNode(*keyId);
				hashPlateKeyList.SetValue(sPartNo,pPlateInfo->partNoId.handle());
			}
			else
				m_hashPlateInfo.DeleteNode(pPlateInfo->partNoId.handle());
		}
		m_xPncMode.PopPlateStack();// m_hashPlateInfo.pop_stack(nPush);
	}
	m_hashPlateInfo.Clean();
	//2.�����Ϣ�������Ĺ���
	for(CPlateProcessInfo* pPlateInfo=m_hashPlateInfo.GetFirst();pPlateInfo;pPlateInfo=m_hashPlateInfo.GetNext())
	{
		if(pPlateInfo->xPlate.m_fThick<=0)
			logerr.Log("�ְ�%s��Ϣ��ȡʧ��!",(char*)pPlateInfo->xPlate.GetPartNo());
	}
	*/
}
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
		if (pLayerTableRecord)
		{
			pLayerTableRecord->close();
			linetypeId = pLayerTableRecord->linetypeObjectId();
		}
	}
	else if (stricmp(sLineTypeName, "ByBlock") == 0)
		linetypeId = m_idSolidLine;		//���ͼԪ����������ΪByBlock,�����;���ʵ��
	else
		linetypeId = pEnt->linetypeId();
	return linetypeId;
}
//��ȡ�ְ���������Ϣ
/*
void CDwgFileInfo::ExtractPlateProfile()
{
	acDocManager->lockDocument(curDoc(),AcAp::kWrite,NULL,NULL,true);
	//�����ض�ͼ�㣬����������ͼԪ�ƶ����ض�ͼ��
	AcDbEntity *pEnt=NULL;
	AcDbObjectId layerId;
	CXhChar50 sNewLayer("new_layer");
	CreateNewLayer(sNewLayer,"CONTINUOUS",AcDb::kLnWt013,1,layerId, m_idSolidLine);
	CHashSet<AcDbObjectId> allEntIdSet;
	GetDrawingVisibleEntSet(allEntIdSet);
	//��Բ������Բ������Ӹ���ͼԪ
	AcDbBlockTableRecord *pBlockTableRecord=GetBlockTableRecord();
	CHashSet<AcDbObjectId> xAssistCirSet;
	for(AcDbObjectId objId=allEntIdSet.GetFirst();objId;objId=allEntIdSet.GetNext())
	{
		acdbOpenAcDbEntity(pEnt,objId,AcDb::kForRead);
		if(pEnt==NULL)
			continue;
		pEnt->close();
		AcDbObjectId curLineId = GetEntLineTypeId(pEnt);
		if (curLineId != m_idSolidLine)
			continue;
		if(pEnt->isKindOf(AcDbEllipse::desc()))
		{
			AcDbEllipse* pEllipse=(AcDbEllipse*)pEnt;
			AcGePoint3d startPt,endPt;
			pEllipse->getStartPoint(startPt);
			pEllipse->getEndPoint(endPt);
			//
			AcDbObjectId lineId;
			AcDbLine *pLine=new AcDbLine(startPt,endPt);
			pBlockTableRecord->appendAcDbEntity(lineId,pLine);
			xAssistCirSet.SetValue(lineId.handle(),lineId);
			pLine->close();
		}
		else if(pEnt->isKindOf(AcDbArc::desc()))
		{
			AcDbArc* pArc=(AcDbArc*)pEnt;
			AcGePoint3d startPt,endPt;
			pArc->getStartPoint(startPt);
			pArc->getEndPoint(endPt);
			//��Բ��ʼ����Ӹ���СԲ
			AcDbObjectId circleId;
			AcGeVector3d norm(0,0,1);
			AcDbCircle *pCircle1=new AcDbCircle(startPt,norm,2);
			pBlockTableRecord->appendAcDbEntity(circleId,pCircle1);
			xAssistCirSet.SetValue(circleId.handle(),circleId);
			pCircle1->close();
			//�յ㴦��Ӹ���СԲ
			AcDbCircle *pCircle2=new AcDbCircle(endPt,norm,2);
			pBlockTableRecord->appendAcDbEntity(circleId,pCircle2);
			xAssistCirSet.SetValue(circleId.handle(),circleId);
			pCircle2->close();
		}
		else if(pEnt->isKindOf(AcDbSpline::desc()))
		{
			AcDbSpline* pSpline = (AcDbSpline*)pEnt;
			int n = pSpline->numControlPoints();
			if (n > 2)
			{
				AcGePoint3d startPt, endPt;
				pSpline->getControlPointAt(0, startPt);
				pSpline->getControlPointAt(n - 1, endPt);
				//
				AcDbObjectId lineId;
				AcDbLine *pLine = new AcDbLine(startPt, endPt);
				pBlockTableRecord->appendAcDbEntity(lineId, pLine);
				xAssistCirSet.SetValue(lineId.handle(), lineId);
				pLine->close();
			}
		}
	}
	pBlockTableRecord->close();
	//
	CHashList<CXhChar50> hashLayerList;
	for(AcDbObjectId objId=allEntIdSet.GetFirst();objId;objId=allEntIdSet.GetNext())
	{
		Acad::ErrorStatus errorStatus=acdbOpenAcDbEntity(pEnt,objId,AcDb::kForWrite);
		if(pEnt==NULL)
			continue;
#ifdef _ARX_2007
		hashLayerList.SetValue(objId.handle(),CXhChar50(_bstr_t(pEnt->layer())));
#else
		hashLayerList.SetValue(objId.handle(),pEnt->layer());
#endif
		AcDbObjectId curLineId=GetEntLineTypeId(pEnt);
		if(pEnt->isKindOf(AcDbLine::desc()))
		{	
			if(curLineId!= m_idSolidLine)
				pEnt->setLayer(layerId);
		}
		else if(pEnt->isKindOf(AcDbArc::desc()))
		{	
			if(curLineId!= m_idSolidLine)
				pEnt->setLayer(layerId);
		}
		else if(pEnt->isKindOf(AcDbRegion::desc()))
		{	//����	
			if(curLineId!= m_idSolidLine)
				pEnt->setLayer(layerId);
		}
		else if(pEnt->isKindOf(AcDbPolyline::desc()))
		{	//���߶�
			if(curLineId!= m_idSolidLine)
				pEnt->setLayer(layerId);
		}
		else
			pEnt->setLayer(layerId);
		pEnt->close();
	}
	//����ָ��ͼ��
	CBomModel::SendCommandToCad(CString("undo be "));
	CBomModel::SendCommandToCad(CString("-layer F new_layer\n "));
	CBomModel::SendCommandToCad(CString("undo e "));
	//��ʼ���ְ�������
	double fMaxEdge = 0, fMinEdge = 200000;
	for(CPlateProcessInfo* pInfo=m_hashPlateInfo.GetFirst();pInfo;pInfo=m_hashPlateInfo.GetNext())
		pInfo->InitProfileByBPolyCmd(fMinEdge,fMaxEdge);
	//ɾ������СԲ
	for(AcDbObjectId objId=xAssistCirSet.GetFirst();objId;objId=xAssistCirSet.GetNext())
	{
		acdbOpenAcDbEntity(pEnt,objId,AcDb::kForWrite);
		if(pEnt==NULL)
			continue;
		pEnt->erase(Adesk::kTrue);
		pEnt->close();
	}
	//��ԭͼԪ����ͼ��
	for(CXhChar50 *pLayer=hashLayerList.GetFirst();pLayer;pLayer=hashLayerList.GetNext())
	{
		long handle=hashLayerList.GetCursorKey();
		AcDbObjectId objId=allEntIdSet.GetValue(handle);
		acdbOpenAcDbEntity(pEnt,objId,AcDb::kForWrite);
		if(pEnt==NULL)
			continue;
#ifdef _ARX_2007
		pEnt->setLayer(_bstr_t(*pLayer));
#else
		pEnt->setLayer(*pLayer);
#endif
		pEnt->close();
	}
	CBomModel::SendCommandToCad(CString("undo be "));
	CBomModel::SendCommandToCad(CString("-layer T *\n "));
	CBomModel::SendCommandToCad(CString("undo e "));
	acDocManager->unlockDocument(curDoc());
}
*/
//��ȡ���������,ȷ���պ�����
BOOL CDwgFileInfo::RetrievePlates()
{
	SmartExtractPlate();
	/*
	//���ݼ������ֱ�עʶ��ְ�
	CAcModuleResourceOverride resOverride;
	AcDbEntity *pEnt=NULL;
	ads_name ent_sel_set,entname;
	CHashSet<AcDbObjectId> textIdHash;
#ifdef _ARX_2007
	acedSSGet(L"ALL",NULL,NULL,NULL,ent_sel_set);
#else
	acedSSGet("ALL",NULL,NULL,NULL,ent_sel_set);
#endif
	{	
		AcDbObjectId entId;
		long ll;
		acedSSLength(ent_sel_set,&ll);
		for(long i=0;i<ll;i++)
		{	//��������˵����ʼ���㼯��
			acedSSName(ent_sel_set,i,entname);
			acdbGetObjectId(entId,entname);
			acdbOpenAcDbEntity(pEnt,entId,AcDb::kForRead);
			if(pEnt==NULL)
				continue;
			pEnt->close();
			if(!pEnt->isKindOf(AcDbText::desc()))
				continue;
			textIdHash.SetValue(entId.handle(),entId);
			AcDbText* pText=(AcDbText*)pEnt;
			CXhChar50 sText;
#ifdef _ARX_2007
			sText.Copy(_bstr_t(pText->textString()));
#else
			sText.Copy(pText->textString());
#endif
			if(g_xUbomModel.manager.IsMatchPNRule(sText))
			{
				CXhChar16 sPartNo;
				f3dPoint dim_pos(pText->position().x,pText->position().y);
				g_xUbomModel.manager.ParsePartNoText(sText,sPartNo);
				if(strlen(sPartNo)<=0)
					continue;
				CPlateProcessInfo* pPlateProcess=NULL;
				for(pPlateProcess=m_hashPlateInfo.GetFirst();pPlateProcess;pPlateProcess=m_hashPlateInfo.GetNext())
				{	//���ι���һ�����ŵ����
					if(DISTANCE(dim_pos,pPlateProcess->dim_pos)<pText->height()*3)
						break;
				}
				if(pPlateProcess==NULL)
				{
					pPlateProcess=m_hashPlateInfo.Add(entId.handle());
					pPlateProcess->dim_pos =dim_pos;
					pPlateProcess->partNoId=entId;
					pPlateProcess->xPlate.cMaterial='S';
					pPlateProcess->xPlate.SetPartNo(sPartNo);
				}
				else
				{
					if (pPlateProcess->m_sRelatePartNo.GetLength() <= 0)
						pPlateProcess->m_sRelatePartNo.Copy(sPartNo);
					else
						pPlateProcess->m_sRelatePartNo.Append(CXhChar16(",%s", (char*)sPartNo));
				}
				pPlateProcess->pnTxtIdList.SetValue(entId.handle(), entId);
			}
		}
	}
	acedSSFree(ent_sel_set);
	if(m_hashPlateInfo.GetNodeNum()<=0)
	{
		logerr.Log("%s�ļ���ȡ�ְ�ʧ��",(char*)m_sFileName);
		return FALSE;
	}
	//��ȡ�ְ�������
	ExtractPlateProfile();
	//���ݸְ�����λ�û�ȡ�Ǹ���Ϣ
	for(AcDbObjectId objId=textIdHash.GetFirst();objId;objId=textIdHash.GetNext())
	{
		acdbOpenAcDbEntity(pEnt,objId,AcDb::kForRead);
		if(pEnt==NULL || !pEnt->isKindOf(AcDbText::desc()))
			continue;
		pEnt->close();
		CXhChar50 sValue;
		f3dPoint text_pos;
		AcDbText* pText=(AcDbText*)pEnt;
		text_pos.Set(pText->position().x,pText->position().y);
#ifdef _ARX_2007
		sValue.Copy(_bstr_t(pText->textString()));
#else
		sValue.Copy(pText->textString());
#endif
		CPlateProcessInfo* pPlateInfo=NULL;
		int nThick=0,nNum=0;
		char cMat;
		if(g_xUbomModel.manager.IsMatchThickRule(sValue))
		{
			pPlateInfo=FindPlateByPt(text_pos);
			if(pPlateInfo)
			{
				g_xUbomModel.manager.ParseThickText(sValue,nThick);
				pPlateInfo->xPlate.m_fThick=(float)nThick;
			}
		}
		if(g_xUbomModel.manager.IsMatchMatRule(sValue))
		{
			pPlateInfo=FindPlateByPt(text_pos);
			if(pPlateInfo)
			{
				g_xUbomModel.manager.ParseMatText(sValue,cMat);
				pPlateInfo->xPlate.cMaterial=cMat;
			}
		}
		if(g_xUbomModel.manager.IsMatchNumRule(sValue))
		{
			pPlateInfo=FindPlateByPt(text_pos);
			if(pPlateInfo)
			{
				g_xUbomModel.manager.ParseNumText(sValue,nNum);
				pPlateInfo->xPlate.feature=nNum;
				pPlateInfo->partNumId=objId;
			}
		}
	}
	//����ȡ�ĽǸ���Ϣ���к����Լ��
	CorrectPlates();
	if(m_hashPlateInfo.GetNodeNum()<=0)
	{	
		logerr.Log("%s�ļ���ȡ�ְ�ʧ��",(char*)m_sFileName);
		return FALSE;
	}
	//����һ����ŵļӵ���ϣ����
	CPlateProcessInfo* pPlateInfo=NULL,*pNewPlateInfo=NULL;
	for(pPlateInfo=m_hashPlateInfo.GetFirst();pPlateInfo;pPlateInfo=m_hashPlateInfo.GetNext())
	{
		int nPush=m_hashPlateInfo.push_stack();
		if(pPlateInfo->sSecPartNo.GetLength()>0)
		{
			pNewPlateInfo=m_hashPlateInfo.Add(pPlateInfo->secPnId.handle());
			pNewPlateInfo->partNoId=pPlateInfo->secPnId;
			pNewPlateInfo->xPlate.SetPartNo(pPlateInfo->sSecPartNo);
			pNewPlateInfo->ClonePlateInfo(pPlateInfo);
		}
		m_hashPlateInfo.pop_stack(nPush);
	}*/
	//�û����Բ鿴��ȡʧ�ܵĸְ�
	/*acDocManager->lockDocument(curDoc(),AcAp::kWrite,NULL,NULL,true);
	for(CPlateProcessInfo* pPlateInfo=m_hashPlateInfo.GetFirst();pPlateInfo;pPlateInfo=m_hashPlateInfo.GetNext())
		pPlateInfo->DrawPlate();	
	acDocManager->unlockDocument(curDoc());*/
	return TRUE;
}

BOOL CDwgFileInfo::ReviseThePlate(const char* sPartNo)
{
	ads_name ent_sel_set,entname;
	CHashSet<AcDbObjectId> selectedEntList;
#ifdef _ARX_2007
	acedSSGet(L"",NULL,NULL,NULL,ent_sel_set);
#else
	acedSSGet("",NULL,NULL,NULL,ent_sel_set);
#endif	
	AcDbObjectId entId;
	AcDbEntity* pEnt=NULL;
	long ll;
	acedSSLength(ent_sel_set,&ll);
	for(long i=0;i<ll;i++)
	{	//��������˵����ʼ���㼯��
		acedSSName(ent_sel_set,i,entname);
		acdbGetObjectId(entId,entname);
		acdbOpenAcDbEntity(pEnt,entId,AcDb::kForRead);
		if(pEnt==NULL)
			continue;
		selectedEntList.SetValue(entId.handle(),entId);
		pEnt->close();
	}
	acedSSFree(ent_sel_set);
	//����ѡ�񼯺���ȡ�ְ��������
	CPlateProcessInfo* pPlateInfo=FindPlateByPartNo(sPartNo);
	if(pPlateInfo==NULL || !pPlateInfo->InitProfileBySelEnts(selectedEntList))
	{
		logerr.Log("ѡ������������󣬹�������Ч�պ�����");
		return FALSE;
	}
	//�������������򣬻�ȡѡ��,��ʼ���ְ���Ϣ
	struct resbuf* pList=NULL,*pPoly=NULL;
	for(CPlateObject::VERTEX* pVer=pPlateInfo->vertexList.GetFirst();pVer;pVer=pPlateInfo->vertexList.GetNext())
	{
		if(pList==NULL)
			pPoly=pList=acutNewRb(RTPOINT);
		else
		{
			pList->rbnext=acutNewRb(RTPOINT);
			pList=pList->rbnext;
		}
		pList->restype=RTPOINT;
		pList->resval.rpoint[X]=pVer->pos.x;
		pList->resval.rpoint[Y]= pVer->pos.y;
		pList->resval.rpoint[Z]=0;
	}
	pList->rbnext=NULL;	
	//��ʼ�������ڸְ��ͼԪ����
#ifdef _ARX_2007
	acedSSGet(L"cp",pPoly,NULL,NULL,ent_sel_set);
#else
	acedSSGet("cp",pPoly,NULL,NULL,ent_sel_set);
#endif
	acutRelRb(pPoly);
	acedSSLength(ent_sel_set,&ll);
	for(long i=0;i<ll;i++)
	{	
		AcDbEntity *pEnt=NULL;
		acedSSName(ent_sel_set,i,entname);
		acdbGetObjectId(entId,entname);
		acdbOpenAcDbEntity(pEnt,entId,AcDb::kForRead);
		if(pEnt==NULL)
			continue;
		pEnt->close();
		if(!pEnt->isKindOf(AcDbText::desc()))
			continue;
		CXhChar50 sValue;
		AcDbText* pText=(AcDbText*)pEnt;
#ifdef _ARX_2007
		sValue.Copy(_bstr_t(pText->textString()));
#else
		sValue.Copy(pText->textString());
#endif
		if(!g_xUbomModel.manager.IsMatchThickRule(sValue))
			continue;
		int nThick=0,nNum=0;
		char cMat;
		//model.manager.ParseThickText(sValue,nThick,nNum,cMat);
		pPlateInfo->xPlate.m_fThick=(float)nThick;
		pPlateInfo->xPlate.feature=nNum;
		pPlateInfo->xPlate.cMaterial=cMat;
		pPlateInfo->partNumId=entId;
	}
	acedSSFree(ent_sel_set);
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
		pJgInfo->RefreshAngleNum();
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
			orig_pt=g_xUbomModel.manager.GetJgCardOrigin(f3dPoint(pText->position().x,pText->position().y,0));
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
			orig_pt=g_xUbomModel.manager.GetJgCardOrigin(f3dPoint(pMText->location().x,pMText->location().y,0));
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
		BOOL bPartNumData=FALSE;
		CAngleProcessInfo* pJgInfo=FindAngleByPt(text_pos);
		if(pJgInfo)
			pJgInfo->InitAngleInfo(text_pos,sValue,bPartNumData);
		if(bPartNumData)
			pJgInfo->partNumId=objId;
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