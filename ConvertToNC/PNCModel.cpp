#include "stdafx.h"
#include "PNCModel.h"
#include "TSPAlgorithm.h"
#include "PNCSysPara.h"
#include "XeroExtractor.h"
#include "DrawUnit.h"
#include "DragEntSet.h"
#include "CadToolFunc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define  ASSIST_RADIUS	2

CPNCModel model;
//////////////////////////////////////////////////////////////////////////
//CPlateProcessInfo
AcDbObjectId MkCadObjId(unsigned long idOld){ return AcDbObjectId((AcDbStub*)idOld); }
CPlateProcessInfo::CPlateProcessInfo()
{
	m_fZoomScale=100;
	boltList.Empty();
	m_bIslandDetection=FALSE;
	plateInfoBlockRefId=0;
	partNoId=0;
	partNumId=0;
	m_bHasInnerDimPos=FALSE;
	m_bEnableReactor = TRUE;
	m_bNeedExtract = FALSE;
}
CPlateObject::CAD_ENTITY* CPlateProcessInfo::AppendRelaEntity(AcDbEntity *pEnt)
{
	if(pEnt==NULL)
		return NULL;
	long entId = pEnt->id().asOldId();
	long objectId = pEnt->objectId().asOldId();
	CAD_ENTITY* pRelaEnt=m_xHashRelaEntIdList.GetValue(pEnt->id().asOldId());
	if(pRelaEnt==NULL)
	{
		pRelaEnt=m_xHashRelaEntIdList.Add(pEnt->id().asOldId());
		pRelaEnt->idCadEnt=pEnt->id().asOldId();
		if (pEnt->isKindOf(AcDbLine::desc()))
			pRelaEnt->ciEntType = RELA_ACADENTITY::TYPE_LINE;
		else if (pEnt->isKindOf(AcDbArc::desc()))
			pRelaEnt->ciEntType = RELA_ACADENTITY::TYPE_ARC;
		else if (pEnt->isKindOf(AcDbCircle::desc()))
		{
			AcDbCircle *pCir = (AcDbCircle*)pEnt;
			AcGePoint3d center = pCir->center();
			//��¼Բ�뾶��λ�� wht 19-07-20
			pRelaEnt->ciEntType = RELA_ACADENTITY::TYPE_CIRCLE;
			pRelaEnt->m_fSize = pCir->radius() * 2;
			pRelaEnt->pos.Set(center.x, center.y, center.z);
		}
		else if (pEnt->isKindOf(AcDbSpline::desc()))
			pRelaEnt->ciEntType = RELA_ACADENTITY::TYPE_SPLINE;
		else if (pEnt->isKindOf(AcDbEllipse::desc()))
			pRelaEnt->ciEntType = RELA_ACADENTITY::TYPE_ELLIPSE;
		else if (pEnt->isKindOf(AcDbText::desc()))
		{
			pRelaEnt->ciEntType = RELA_ACADENTITY::TYPE_TEXT;
			AcDbText* pText = (AcDbText*)pEnt;
#ifdef _ARX_2007
			strncpy(pRelaEnt->sText, _bstr_t(pText->textString()), 99);
#else
			strncpy(pRelaEnt->sText, pText->textString(), 99);
#endif
			//ȡ��������λ��Ϊ��Ϊ����λ�����꣬�������ж������Ƿ���Բ��ʱ�����󽫳ߴ��עʶ��Ϊ���ű�ע wht 19-08-14
			Cpy_Pnt(pRelaEnt->pos, pText->position());
			GEPOINT pos;
			if (GetCadTextEntPos(pText, pos,false))
				pRelaEnt->pos.Set(pos.x, pos.y, pos.z);
		}
		else if (pEnt->isKindOf(AcDbMText::desc()))
			pRelaEnt->ciEntType = RELA_ACADENTITY::TYPE_MTEXT;
		else if (pEnt->isKindOf(AcDbBlockReference::desc()))
			pRelaEnt->ciEntType = RELA_ACADENTITY::TYPE_BLOCKREF;
		else
			pRelaEnt->ciEntType=RELA_ACADENTITY::TYPE_OTHER;
	}
	else
	{
		int a = 10;
	}
	return pRelaEnt;
}
int CPlateProcessInfo::GetRelaEntIdList(ARRAY_LIST<AcDbObjectId> &entIdList)
{
	entIdList.Empty();
	for(CPlateObject::CAD_ENTITY *pCadEnt=EnumFirstEnt();pCadEnt;pCadEnt=EnumNextEnt())
		entIdList.append(MkCadObjId(pCadEnt->idCadEnt));
	return (int)entIdList.Size();
}
//���ݸְ���������ʼ�������ְ��ͼԪ����
void CPlateProcessInfo::InternalExtractPlateRelaEnts()
{
	m_xHashRelaEntIdList.Empty();
	m_xHashRelaEntIdList.SetValue(partNoId.asOldId(),CAD_ENTITY(partNoId.asOldId()));
	if(vertexList.GetNodeNum()<3)
		return;
	//��ȡ�ְ�Ⱦ�����������㼯��
	double minDistance=0.5;
	ATOM_LIST<VERTEX> list;
	CalEquidistantShape(minDistance,&list);
	struct resbuf* pList=NULL,*pPoly=NULL;
	for(VERTEX* pVer=list.GetFirst();pVer;pVer=list.GetNext())
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
		pList->resval.rpoint[Y]=pVer->pos.y;
		pList->resval.rpoint[Z]=0;
	}
	pList->rbnext=NULL;	
	//��ʼ�������ڸְ��ͼԪ����
	if(!dim_pos.IsZero())
	{	//���ݱ�עλ�ý�������
		f2dRect rect=GetPnDimRect();
		ZoomAcadView(rect,m_fZoomScale);
	}
	AcDbObjectId entId;
	ads_name ent_sel_set,entname;
#ifdef _ARX_2007
	int retCode=acedSSGet(L"cp",pPoly,NULL,NULL,ent_sel_set);
#else
	int retCode=acedSSGet("cp",pPoly,NULL,NULL,ent_sel_set);
#endif
	acutRelRb(pPoly);
	if(retCode==RTNORM)
	{
		long ll=0;
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
			if(pEnt->isKindOf(AcDbLine::desc()))
			{	//���˲��������ڵ�ֱ��
				AcDbLine* pLine=(AcDbLine*)pEnt;
				f3dPoint startPt,endPt;
				Cpy_Pnt(startPt,pLine->startPoint());
				Cpy_Pnt(endPt,pLine->endPoint());
				startPt.z=endPt.z=0;
				if(!IsInPlate(startPt,endPt))
					continue;
			}
			AppendRelaEntity(pEnt);
		}
	}
	acedSSFree(ent_sel_set);
	//
	if(m_bHasInnerDimPos)
	{	//����Χѡ���ı�
		ATOM_LIST<VERTEX> list;
		CalEquidistantShape(50,&list);
		struct resbuf* pList=NULL,*pPoly=NULL;
		for(VERTEX* pVer=list.GetFirst();pVer;pVer=list.GetNext())
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
			pList->resval.rpoint[Y]=pVer->pos.y;
			pList->resval.rpoint[Z]=0;
		}
		pList->rbnext=NULL;	
		//��ʼ�������ڸְ��ͼԪ����
		AcDbObjectId entId;
		ads_name ent_sel_set,entname;
#ifdef _ARX_2007
		int retCode=acedSSGet(L"cp",pPoly,NULL,NULL,ent_sel_set);
#else
		int retCode=acedSSGet("cp",pPoly,NULL,NULL,ent_sel_set);
#endif
		acutRelRb(pPoly);
		if(retCode==RTNORM)
		{
			long ll=0;
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
				if(pEnt->isKindOf(AcDbText::desc()))
					AppendRelaEntity(pEnt);
			}
		}
		acedSSFree(ent_sel_set);
	}
}

void CPlateProcessInfo::ExtractPlateRelaEnts()
{
	InternalExtractPlateRelaEnts();
	//�жϸְ�ʵ�弯�����Ƿ�����Բ��
		//������Բ��ʱ��Ҫ������Բ�����¸ְ�������������ȡ�ְ����ʵ�壬������ܵ���©�� wht 19-08-14
	CPlateObject::CAD_ENTITY *pCadEnt = NULL;
	BOOL bHasEllipse = FALSE;
	for (pCadEnt = EnumFirstEnt(); pCadEnt; pCadEnt = EnumNextEnt())
	{
		if (pCadEnt->ciEntType == RELA_ACADENTITY::TYPE_ELLIPSE)
		{
			bHasEllipse = TRUE;
			break;
		}
	}
	if (bHasEllipse)
	{
		AcDbEntity *pEnt = NULL;
		for (pCadEnt = EnumFirstEnt(); pCadEnt; pCadEnt = EnumNextEnt())
		{
			acdbOpenAcDbEntity(pEnt, MkCadObjId(pCadEnt->idCadEnt), AcDb::kForRead);
			if (pEnt == NULL)
				continue;
			CAcDbObjLife entLife(pEnt);
			if (pEnt->isKindOf(AcDbEllipse::desc()))
			{	//��ȡԲ����������Ϣ
				BYTE ciEdgeType = 0;
				f3dArcLine arcLine;
				if (g_pncSysPara.RecogArcEdge(pEnt, arcLine, ciEdgeType))
					UpdateVertexPropByArc(arcLine, ciEdgeType);
			}
		}
		//������Բ�������ְ����������������ȡ�ְ����ʵ�� wht 19-08-14
		InternalExtractPlateRelaEnts();
	}
}

void CPlateProcessInfo::PreprocessorBoltEnt(CHashSet<CAD_ENTITY*> &hashInvalidBoltCirPtrSet, int *piInvalidCirCountForText)
{	//�ϲ�Բ����ͬ��Բ��ͬһԲ��ֻ����ֱ������Բ
	CHashStrList<CAD_ENTITY*> hashEntPtrByCenterStr;	//��¼Բ�Ķ�Ӧ��ֱ��������˨ʵ��
	hashInvalidBoltCirPtrSet.Empty();
	for (CAD_ENTITY *pEnt = m_xHashRelaEntIdList.GetFirst(); pEnt; pEnt = m_xHashRelaEntIdList.GetNext())
	{
		if (pEnt->ciEntType != RELA_ACADENTITY::TYPE_CIRCLE)//&&pEnt->ciEntType != CAD_ENT::TYPE_POLYLINE_ARC)
			continue;
		CXhChar50 sKey("%.2f,%.2f,%.2f", pEnt->pos.x, pEnt->pos.y, pEnt->pos.z);
		CAD_ENTITY **ppEntPtr = hashEntPtrByCenterStr.GetValue(sKey);
		if (ppEntPtr)
		{
			if ((*ppEntPtr)->m_fSize > pEnt->m_fSize)
			{	//pEntֱ����СΪ��˨��Բ��Ҫ����
				hashInvalidBoltCirPtrSet.SetValue((DWORD)pEnt, pEnt);
			}
			else
			{	//**ppEntPtr��˨ֱ����СΪ��˨��Բ��Ҫ����
				hashInvalidBoltCirPtrSet.SetValue((DWORD)(*ppEntPtr), *ppEntPtr);
				hashEntPtrByCenterStr.DeleteNode(sKey);
				hashEntPtrByCenterStr.SetValue(sKey, pEnt);
			}
		}
		else
			hashEntPtrByCenterStr.SetValue(sKey, pEnt);
	}
	if (g_pncSysPara.m_ciBoltRecogMode == CPNCSysPara::BOLT_RECOG_DEFAULT)
	{
		int nInvalidCount = 0;
		for (CAD_ENTITY *pEnt = m_xHashRelaEntIdList.GetFirst(); pEnt; pEnt = m_xHashRelaEntIdList.GetNext())
		{
			if (pEnt->ciEntType != RELA_ACADENTITY::TYPE_TEXT && pEnt->ciEntType != RELA_ACADENTITY::TYPE_MTEXT)
				continue;
			//��ע�ַ����а���"��׻���"��
			if (strstr(pEnt->sText, "��") != NULL || strstr(pEnt->sText, "��") != NULL ||
				strstr(pEnt->sText, "��") != NULL || strstr(pEnt->sText, "%%C") != NULL ||
				strstr(pEnt->sText, "��") != NULL)
				continue;
			//�ų����ű�עԲȦ
			SCOPE_STRU scope;
			for (CAD_ENTITY **ppCirEnt = hashEntPtrByCenterStr.GetFirst(); ppCirEnt; ppCirEnt = hashEntPtrByCenterStr.GetNext())
			{
				CAD_ENTITY *pCirEnt = *ppCirEnt;
				scope.ClearScope();
				double r = pCirEnt->m_fSize*0.5;
				scope.fMinX = pCirEnt->pos.x - r;
				scope.fMaxX = pCirEnt->pos.x + r;
				scope.fMinY = pCirEnt->pos.y - r;
				scope.fMaxY = pCirEnt->pos.y + r;
				scope.fMinZ = scope.fMaxZ = 0;
				if (scope.IsIncludePoint(pEnt->pos))
				{
					hashInvalidBoltCirPtrSet.SetValue((DWORD)pCirEnt, pCirEnt);
					hashEntPtrByCenterStr.DeleteCursor();
					nInvalidCount++;
				}
			}
		}
		if (piInvalidCirCountForText)
			*piInvalidCirCountForText = nInvalidCount;
	}
}
//���ݸְ���ص�ͼԪ���ϸ��»�����Ϣ����˨��Ϣ������(����)��Ϣ
BOOL CPlateProcessInfo::UpdatePlateInfo(BOOL bRelatePN/*=FALSE*/)
{
	if(vertexList.GetNodeNum()<3)
		return FALSE;
	//����Spline��ȡ�����߱����Ϣ(�ֶ��߶μ���)
	CSymbolRecoginzer symbols;
	AcDbEntity *pEnt=NULL;
	PRESET_ARRAY8<CAD_ENTITY*> bendDimTextArr;
	for(CAD_ENTITY *pRelaObj=m_xHashRelaEntIdList.GetFirst();pRelaObj;pRelaObj=m_xHashRelaEntIdList.GetNext())
	{
		if( pRelaObj->ciEntType!=RELA_ACADENTITY::TYPE_SPLINE&&
			pRelaObj->ciEntType!=RELA_ACADENTITY::TYPE_TEXT)
			continue;
		acdbOpenAcDbEntity(pEnt,MkCadObjId(pRelaObj->idCadEnt),AcDb::kForRead);
		if(pEnt==NULL)
			continue;
		CAcDbObjLife objLife(pEnt);
		if(pRelaObj->ciEntType==RELA_ACADENTITY::TYPE_SPLINE)
		{
			AcGePoint3d pt;
			AcDbSpline* pSpline=(AcDbSpline*)pEnt;
			CSymbolEntity* pSymbol=symbols.listSymbols.AttachObject();
			for(int i=0;i<pSpline->numFitPoints()-1;i++)
			{
				CSymbolEntity::LINE* pLine=pSymbol->listSonlines.AttachObject();
				pSymbol->ciSymbolType=1;	//��Ĭ��Ϊ������S�ͷ���
				pSpline->getFitPointAt(i,pt);
				Cpy_Pnt(pLine->start,pt);
				pSpline->getFitPointAt(i+1,pt);
				Cpy_Pnt(pLine->end,pt);
			}
		}
		else if(pRelaObj->ciEntType==RELA_ACADENTITY::TYPE_TEXT)
		{
			if(g_pncSysPara.IsMatchBendRule(pRelaObj->sText))
				bendDimTextArr.Append(pRelaObj);
		}
	}
	//
	boltList.Empty();
	xPlate.m_cFaceN=1;
	DeleteAssisstPts();
	CHashSet<CAD_ENTITY*> hashInvalidBoltCirPtrSet;
	int nInvalidCirCountForText = 0;
	PreprocessorBoltEnt(hashInvalidBoltCirPtrSet, &nInvalidCirCountForText);
	if (nInvalidCirCountForText > 0)
		logerr.Log("%s#�ְ壬�ѹ���%d������Ϊ���ű�ע��Բ����ȷ�ϣ�",(char*)xPlate.GetPartNo(),nInvalidCirCountForText);
	//baseInfoӦ������Forѭ���⣬������ж����ȡ�ᵼ��֮ǰ����ȡ���Ľ������� wht 19-10-22
	BASIC_INFO baseInfo;
	for (CAD_ENTITY *pRelaObj = m_xHashRelaEntIdList.GetFirst(); pRelaObj; pRelaObj = m_xHashRelaEntIdList.GetNext())
	{
		if (hashInvalidBoltCirPtrSet.GetValue((DWORD)pRelaObj))
			continue;	//���˵���Ч����˨ʵ�壨��Բ�ڵ�СԲ�����ű�ע�� wht 19-07-20
		acdbOpenAcDbEntity(pEnt,MkCadObjId(pRelaObj->idCadEnt),AcDb::kForRead);
		if(pEnt==NULL)
			continue;
		pEnt->close();
		//��ȡ������Ϣ
		if(plateInfoBlockRefId==NULL&&g_pncSysPara.RecogBasicInfo(pEnt,baseInfo))
		{	//plateInfoBlockRefId��Чʱ�Ѵӿ�����ȡ���˻������� wht 19-01-04
			if(baseInfo.m_cMat>0)
				xPlate.cMaterial=baseInfo.m_cMat;
			if(baseInfo.m_nThick>0)
				xPlate.m_fThick=(float)baseInfo.m_nThick;
			if (baseInfo.m_nNum > 0)
				xPlate.feature = baseInfo.m_nNum;
			if (baseInfo.m_idCadEntNum != 0)
				partNumId = MkCadObjId(baseInfo.m_idCadEntNum);
			if(baseInfo.m_sPartNo.GetLength()>0&&bRelatePN)
			{
				if(xPlate.GetPartNo().Length<=0)
					xPlate.SetPartNo(baseInfo.m_sPartNo);
				else if(m_sRelatePartNo.Length<=0)
					m_sRelatePartNo.Copy(baseInfo.m_sPartNo);
				else
					m_sRelatePartNo.Append(CXhChar16(",%s",(char*)baseInfo.m_sPartNo));
				pnTxtIdList.SetValue(pRelaObj->idCadEnt,MkCadObjId(pRelaObj->idCadEnt));
			}
			continue;
		}
		//��ȡ��˨��Ϣ
		BOLT_HOLE boltInfo;
		if(g_pncSysPara.RecogBoltHole(pEnt,boltInfo))
		{
			BOLT_INFO *pBoltInfo=boltList.append();
			pBoltInfo->posX=boltInfo.posX;
			pBoltInfo->posY=boltInfo.posY;
			pBoltInfo->bolt_d=boltInfo.d;
			pBoltInfo->hole_d_increment=boltInfo.increment;
			pBoltInfo->cFuncType=(boltInfo.ciSymbolType==1)?2:0;
			//����˨����Χ�����׾���ע��ʶ��׾� wht 18-12-30
			if(pBoltInfo->bolt_d==0)
			{	
				int nPush=m_xHashRelaEntIdList.push_stack();
				for(CAD_ENTITY *pRelaObj=m_xHashRelaEntIdList.GetFirst();pRelaObj;pRelaObj=m_xHashRelaEntIdList.GetNext())
				{
					if(pRelaObj->ciEntType!=RELA_ACADENTITY::TYPE_TEXT)
						continue;
					if(strlen(pRelaObj->sText)<=0||(strstr(pRelaObj->sText,"%%C")==NULL&&strstr(pRelaObj->sText,"%%c")==NULL))
						continue;
					if(DISTANCE(pRelaObj->pos,GEPOINT(boltInfo.posX,boltInfo.posY))<50)
					{
						CString ss(pRelaObj->sText);
						if(ss.Find("��")>=0)
							pBoltInfo->cFlag=1;
						ss.Replace("%%C","");
						ss.Replace("%%c","");
						ss.Replace("��","");
						ss.Replace("��","");
						ss.Replace("��","");
						double hole_d=atof(ss);
						if(ss.Find(".")>0)
						{
							pBoltInfo->bolt_d=(int)(hole_d-1.5);
							pBoltInfo->hole_d_increment=1.5;
						}
						else 
						{
							pBoltInfo->bolt_d=(int)hole_d;
							pBoltInfo->hole_d_increment=0;
						}
						pBoltInfo->cFuncType=2;
						break;
					}
				}
				m_xHashRelaEntIdList.pop_stack(nPush);
			}
			//��������׽��д��״���
			if (g_pncSysPara.m_bReplaceSH && pBoltInfo->cFuncType == 2)
			{
				pBoltInfo->bolt_d = g_pncSysPara.m_nReplaceHD;
				pBoltInfo->hole_d_increment = 1.5;
			}
			continue;
		}
		//��ȡԲ����������Ϣ
		BYTE ciEdgeType=0;
		f3dArcLine arcLine;
		if(g_pncSysPara.RecogArcEdge(pEnt,arcLine,ciEdgeType))
		{
			UpdateVertexPropByArc(arcLine,ciEdgeType);
			continue;
		}
		//��ȡ��ӡ��
		f3dPoint ptArr[4];
		if(g_pncSysPara.RecogMkRect(pEnt,ptArr,4))
		{
			xPlate.mkpos.Set((ptArr[0].x+ptArr[2].x)/2,(ptArr[0].y+ptArr[2].y)/2,(ptArr[0].z+ptArr[2].z)/2);
			if(DISTANCE(ptArr[0],ptArr[1])>DISTANCE(ptArr[1],ptArr[2]))
				xPlate.mkVec=ptArr[0]-ptArr[1];
			else
				xPlate.mkVec=ptArr[1]-ptArr[2];
			normalize(xPlate.mkVec);
		}
		//
		if(pEnt->isKindOf(AcDbLine::desc()))
		{	//��ʼ���ְ�Ļ�������Ϣ
			if(xPlate.m_cFaceN>=3)
				continue;
			AcDbLine* pAcDbLine=(AcDbLine*)pEnt;
			double dist=0,min_dist=100000;
			f3dPoint startPt,endPt,perp;
			Cpy_Pnt(startPt,pAcDbLine->startPoint());
			Cpy_Pnt(endPt,pAcDbLine->endPoint());
			if(g_pncSysPara.IsBendLine((AcDbLine*)pEnt,&symbols))
			{
				int iFace=0;
				xPlate.m_cFaceN+=1;
				if(xPlate.m_cFaceN==2)
				{
					xPlate.HuoQuLine[0].startPt=startPt;
					xPlate.HuoQuLine[0].endPt=endPt;
				}
				else
				{
					xPlate.HuoQuLine[1].startPt=startPt;
					xPlate.HuoQuLine[1].endPt=endPt;
					iFace=1;
				}
				int iMatch=-1;
				CAD_ENTITY *pMatchEnt=NULL;
				for(DWORD i=0;i<bendDimTextArr.Size();i++)
				{
					CAD_ENTITY *pEnt=bendDimTextArr.At(i);
					if(pEnt==NULL)
						continue;
					if(SnapPerp(&perp,startPt,endPt,f3dPoint(pEnt->pos),&dist)&&dist<min_dist)
					{
						pMatchEnt=pEnt;
						iMatch=i;
					}
				}
				//TODO���������߸����������ֱ�ע���������ֱ�עʶ���������������ʼ�������淨�߷��� wht 19-02-13
				/*if(iMatch>=0&&pMatchEnt!=NULL)
				{
					double fDegree=0;
					BOOL bFrontBend=FALSE;
					g_pncSysPara.ParseBendText(pMatchEnt->sText,fDegree,bFrontBend);
					fDegree*=RADTODEG_COEF;
					f3dPoint axis=endPt-startPt;
					normalize(axis);
					RotateVectorAroundVector(xPlate.HuoQuFaceNorm[iFace],sin(fDegree),cos(fDegree),axis);
					//f3dPoint extend_vec=xPlate.HuoQuFaceNorm[iFace];
					//RotateVectorAroundVector(xPlate.HuoQuFaceNorm[iFace],1,0,axis);
				}*/
			}
			else if(g_pncSysPara.IsSlopeLine((AcDbLine*)pEnt))
			{
				RecogWeldLine(f3dLine(startPt,endPt));
				continue;
			}
		}
	}
	return m_xHashRelaEntIdList.GetNodeNum()>1;
}
f2dRect CPlateProcessInfo::GetPnDimRect()
{
	f2dRect rect;
	AcDbEntity* pEnt=NULL;
	acdbOpenAcDbEntity(pEnt,partNoId,AcDb::kForRead);
	CAcDbObjLife objLife(pEnt);
	f3dPoint origin=dim_pos;
	if(pEnt&&pEnt->isKindOf(AcDbText::desc()))
	{
		AcDbText* pText=(AcDbText*)pEnt;
		CXhChar100 sText;
#ifdef _ARX_2007
		sText.Copy(_bstr_t(pText->textString()));
#else
		sText.Copy(pText->textString());
#endif
		double heigh=pText->height();
		double angle=pText->rotation();
		double len=DrawTextLength(sText,heigh,pText->textStyle());
		f3dPoint dim_norm(-sin(angle),cos(angle));
		origin+=dim_vec*len*0.5;
		origin+=dim_norm*heigh*0.5;
	}
	rect.topLeft.Set(origin.x-10,origin.y+10);
	rect.bottomRight.Set(origin.x+10,origin.y-10);
	return rect;
}
//�����ظ���
static BOOL IsHasVertex(ATOM_LIST<CPlateObject::VERTEX>& tem_vertes,GEPOINT pt)
{
	for(int i=0;i<tem_vertes.GetNodeNum();i++)
	{
		CPlateObject::VERTEX* pVer=tem_vertes.GetByIndex(i);
		if(pVer->pos.IsEqual(pt,EPS2))
			return TRUE;
	}
	return FALSE;
}
//ͨ��bpoly������ȡ�ְ�������
void CALLBACK EXPORT CloseBoundaryPopupWnd(HWND hWnd, UINT nMsg, UINT nIDEvent, DWORD dwTime)
{
	HWND hMainWnd = adsw_acadMainWnd();
	HWND hPopupWnd=GetLastActivePopup(hMainWnd);
	if(hPopupWnd!=NULL&&hPopupWnd!=hMainWnd)
	{
		::SendMessage(hPopupWnd ,WM_CLOSE,0,0);
		KillTimer(hMainWnd,1);
	}
}

void CPlateProcessInfo::InitProfileByBPolyCmd(double fMinExtern,double fMaxExtern)
{
	if (!m_bNeedExtract)
		return;
	ads_name seqent;
	AcDbObjectId initLastObjId,plineId;
	acdbEntLast(seqent);
	acdbGetObjectId(initLastObjId,seqent);
	//�����ű�עλ�ý�������,�ҵ����ʵ����ű���,ͨ��boundary������ȡ������
	f2dRect rect=GetPnDimRect();
	double fExternLen=fMaxExtern*3;
	fExternLen=(fExternLen>2200)?2200:fExternLen;
	double fScale=(fExternLen-fMinExtern)/10;
	//
	HWND hMainWnd=adsw_acadMainWnd();
	for(int j=0;j<2;j++)
	{
		f3dPoint cur_dim_pos=dim_pos;
		if(j==1)
		{	//����ʹ��inner_dim_pos��ȡ wht 19-02-01
			if(m_bHasInnerDimPos)
			{
				cur_dim_pos=inner_dim_pos;
				rect.topLeft.Set(cur_dim_pos.x-150,cur_dim_pos.y+10);
				rect.bottomRight.Set(cur_dim_pos.x+10,cur_dim_pos.y-10);
			}
			else
				break;
		}
		for(int i=0;i<=10;i++)
		{
			if(i>0)
				fExternLen-=fScale;
			m_fZoomScale=fExternLen/g_pncSysPara.m_fMapScale;
			ZoomAcadView(rect,m_fZoomScale);
			ads_point base_pnt;
			base_pnt[X]=cur_dim_pos.x;
			base_pnt[Y]=cur_dim_pos.y;
			base_pnt[Z]=cur_dim_pos.z;
			int nTimer=SetTimer(hMainWnd,1,100,CloseBoundaryPopupWnd);
			int resCode=0;
	#ifdef _ARX_2007
			if(m_bIslandDetection)
				resCode=acedCommand(RTSTR,L"-boundary",RTPOINT,base_pnt,RTSTR,L"",RTNONE);
			else
				resCode=acedCommand(RTSTR,L"-boundary",RTSTR,L"a",RTSTR,L"i",RTSTR,L"n",RTSTR,L"",RTSTR,L"",RTPOINT,base_pnt,RTSTR,L"",RTNONE);
	#else
			if(m_bIslandDetection)
				resCode=acedCommand(RTSTR,"-boundary",RTPOINT,base_pnt,RTSTR,"",RTNONE);
			else
				resCode=acedCommand(RTSTR,"-boundary",RTSTR,"a",RTSTR,"i",RTSTR,"n",RTSTR,"",RTSTR,"",RTPOINT,base_pnt,RTSTR,"",RTNONE);
	#endif		
			if(nTimer==1)
				KillTimer(hMainWnd,nTimer);
			if(resCode!=RTNORM)
				return;
			acdbEntLast(seqent);
			acdbGetObjectId(plineId,seqent);
			if(initLastObjId!=plineId)
				break;
		}
		if(initLastObjId!=plineId)
		{
			m_bHasInnerDimPos=(j==1);
			break;
		}
	}

	if (initLastObjId == plineId)
	{	//ִ�п�������(��������س�)�������ظ�ִ����һ������ wxc-2019.6.13
		acedCommand(RTSTR, "");
		return;	//ִ��boundaryδ����ʵ��(��δ�ҵ���Ч�ķ������)
	}
	AcDbEntity *pEnt = NULL;
	acdbOpenAcDbEntity(pEnt,plineId,AcDb::kForWrite);
	if(pEnt==NULL||!pEnt->isKindOf(AcDbPolyline::desc()))
	{
		if(pEnt)
			pEnt->close();
		//ִ�п�������(��������س�)�������ظ�ִ����һ������ wxc-2019.6.13
		acedCommand(RTSTR, "");
		return;
	}
	//
	AcDbPolyline *pPline=(AcDbPolyline*)pEnt;
	int nVertNum = pPline->numVerts();
	ATOM_LIST<VERTEX> tem_vertes;
	VERTEX* pVer=NULL;
	for(int iVertIndex=0;iVertIndex<nVertNum;iVertIndex++)
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
			pVer->pos.Set(location.x,location.y,location.z);
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
			pVer->pos.Set(location.x,location.y,location.z);
			pVer->ciEdgeType=2;
			pVer->arc.center=center;
			pVer->arc.work_norm=norm;
			pVer->arc.radius=acgeArc.radius();
		}
	}
	pPline->erase(Adesk::kTrue);	//ɾ��polyline����
	pPline->close();
	//
	for(int i=0;i<tem_vertes.GetNodeNum();i++)
	{
		VERTEX* pCurVer=tem_vertes.GetByIndex(i);
		if(pCurVer->ciEdgeType==2&&ftoi(pCurVer->arc.radius)==ASSIST_RADIUS)
		{
			VERTEX* pNextVer=tem_vertes.GetByIndex((i+1)%nVertNum);
			if(pNextVer->ciEdgeType==1)
				pNextVer->pos=pCurVer->arc.center;
			else if(pNextVer->ciEdgeType==2&&ftoi(pNextVer->arc.radius)!=ASSIST_RADIUS)
				pNextVer->pos=pCurVer->arc.center;
		}
	}
	for(pVer=tem_vertes.GetFirst();pVer;pVer=tem_vertes.GetNext())
	{
		if(pVer->ciEdgeType==2&&ftoi(pVer->arc.radius)==ASSIST_RADIUS)
			tem_vertes.DeleteCursor();
	}
	tem_vertes.Clean();
	//���ְ��������
	vertexList.Empty();
	for(int i=0;i<tem_vertes.GetNodeNum();i++)
	{
		VERTEX* pCur=tem_vertes.GetByIndex(i);
		pVer=vertexList.append(*pCur);
		if(pVer->ciEdgeType==2)
		{	//��Բ����Ϊ�����߶Σ���С�ְ������������
			pVer->ciEdgeType=1;
			VERTEX* pNext=tem_vertes.GetByIndex((i+1)%tem_vertes.GetNodeNum());
			f3dPoint startPt=pCur->pos,endPt=pNext->pos;
			double len = 0.5*DISTANCE(startPt,endPt);
			if(len<EPS2)
				continue;	//Բ����ʼ�㡢��ֹ������������������CreateMethod3��Ľ��� wht 19-01-02
			f3dArcLine arcLine;
			if(arcLine.CreateMethod3(startPt,endPt,pCur->arc.work_norm,pCur->arc.radius,pCur->arc.center))
			{
				int nSlices=CalArcResolution(arcLine.Radius(),arcLine.SectorAngle(),1.0,5.0,18);
				double angle=arcLine.SectorAngle()/nSlices;
				for(int i=1;i<nSlices;i++)
				{
					VERTEX *pProVer= vertexList.append();
					pProVer->pos=arcLine.PositionInAngle(angle*i);
					pProVer->tag.lParam=-1;
				}
			}
		}
	}
	//����������к����Լ��(�Ƿ���ʱ������)
	if(!IsValidVertexs())
		ReverseVertexs();
	CreateRgn();
}
struct LINE_OBJECT
{
	static const BYTE STRAIGHT	=0x01;
	static const BYTE ARCLINE	=0x02;
	static const BYTE ELLIPSE	=0x03;
	char ciEdgeType;
	AcDbObjectId idCadEnt;
	f3dPoint startPt;
	f3dPoint endPt;
	BOOL bMatch;
	LINE_OBJECT(){bMatch=FALSE;}
};
BOOL CPlateProcessInfo::InitProfileBySelEnts(CHashSet<AcDbObjectId>& selectedEntSet)
{	//1.����ѡ��ʵ���ʼ��ֱ���б�
	ATOM_LIST<LINE_OBJECT> objectLineList;
	for(AcDbObjectId objId=selectedEntSet.GetFirst();objId;objId=selectedEntSet.GetNext())
	{
		AcDbEntity *pEnt=NULL;
		acdbOpenAcDbEntity(pEnt,objId,AcDb::kForRead);
		if(pEnt==NULL)
			continue;
		pEnt->close();
		if(!pEnt->isKindOf(AcDbLine::desc())&&!pEnt->isKindOf(AcDbArc::desc())&&!pEnt->isKindOf(AcDbEllipse::desc()))
			continue;
		AcDbCurve* pCurve=(AcDbCurve*)pEnt;
		f3dPoint startPt,endPt;
		AcGePoint3d pt;
		pCurve->getStartPoint(pt);
		Cpy_Pnt(startPt,pt);
		pCurve->getEndPoint(pt);
		Cpy_Pnt(endPt,pt);
		//
		LINE_OBJECT* pLine=NULL;
		for(pLine=objectLineList.GetFirst();pLine;pLine=objectLineList.GetNext())
		{	//����ʼ�յ��غϵ��߶�
			if((pLine->startPt.IsEqual(startPt,EPS2)&&pLine->endPt.IsEqual(endPt,EPS2))||
				(pLine->startPt.IsEqual(endPt,EPS2)&&pLine->endPt.IsEqual(startPt,EPS2)))
				break;
		}
		if(pLine==NULL)
		{
			pLine=objectLineList.append();
			pLine->startPt=startPt;
			pLine->endPt=endPt;
			if(pEnt->isKindOf(AcDbLine::desc()))
				pLine->ciEdgeType=LINE_OBJECT::STRAIGHT;
			else if(pEnt->isKindOf(AcDbArc::desc()))
				pLine->ciEdgeType=LINE_OBJECT::ARCLINE;
			else if(pEnt->isKindOf(AcDbEllipse::desc()))
				pLine->ciEdgeType=LINE_OBJECT::ELLIPSE;
		}
	}
	if(objectLineList.GetNodeNum()<3)
		return FALSE;
	//2.��ʼ���ְ�������(��ȡ����������㣩
	vertexList.Empty();
	VERTEX xVertex;
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
			xVertex.pos=(double*)endPt;
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
				xVertex.pos=(double*)pLine->endPt;
				vertexList.append(xVertex);
				break;
			}
			else if(fabs(DISTANCE(pLine->endPt,endPt))<EPS2)
			{	//
				endPt=pLine->startPt;
				pLine->bMatch=TRUE;
				xVertex.pos=(double*)pLine->startPt;
				vertexList.append(xVertex);
				break;
			}
		}
		if(pLine==NULL)
			break;
	}
	if(bFinish==FALSE)
		return FALSE;
	//3.����������к����Լ��(�Ƿ���ʱ������)������ʼ���ְ�����
	if(!IsValidVertexs())
		ReverseVertexs();
	CreateRgn();
	return TRUE;
}
void CPlateProcessInfo::InitMkPos(GEPOINT &mk_pos,GEPOINT &mk_vec)
{
	int i=0;
	AcDbEntity *pEnt=NULL;
	for(AcDbObjectId objId=pnTxtIdList.GetFirst();objId;objId=pnTxtIdList.GetNext())
	{
		acdbOpenAcDbEntity(pEnt,objId,AcDb::kForRead);
		if(pEnt==NULL)
			continue;
		pEnt->close();
		if(!pEnt->isKindOf(AcDbText::desc())&&!pEnt->isKindOf(AcDbMText::desc()))
			continue;
		CXhChar100 sText;
		double heigh=0;
		AcDbObjectId styleId;
		if(pEnt->isKindOf(AcDbText::desc()))
		{
			AcDbText* pText=(AcDbText*)pEnt;
#ifdef _ARX_2007
			sText.Copy(_bstr_t(pText->textString()));
#else
			sText.Copy(pText->textString());
#endif
			heigh=pText->height();
			styleId=pText->textStyle();
		}
		else if(pEnt->isKindOf(AcDbMText::desc()))
		{
			AcDbMText* pMText=(AcDbMText*)pEnt;
#ifdef _ARX_2007
			sText.Copy(_bstr_t(pMText->contents()));
#else
			sText.Copy(pMText->contents());
#endif
			heigh=pMText->textHeight();
			styleId=pMText->textStyle();
		}
		double len=DrawTextLength(sText,heigh,styleId);
		f3dPoint origin=dim_pos+dim_vec*len*0.5;
		if(mk_vec.IsZero())
			mk_vec=dim_vec;
		if(i==0)
			mk_pos=origin;
		else
			mk_pos=(mk_pos+origin)*0.5;
		i++;
	}
}
//����ppi�����ļ�
void CPlateProcessInfo::CreatePPiFile(const char* file_path)
{
	//����ְ�ĳ����
	f2dRect rect=GetMinWrapRect();
	xPlate.m_wLength=(rect.Width()>rect.Height())?ftoi(rect.Width()):ftoi(rect.Height());
	xPlate.m_fWidth =(rect.Width()>rect.Height())?(float)ftoi(rect.Height()): (float)ftoi(rect.Width());
	//��ʼ���ְ�Ĺ�����Ϣ
	VERTEX* pVer=NULL;
	PROFILE_VER* pNewVer=NULL;
	xPlate.vertex_list.Empty();
	for(pVer=vertexList.GetFirst();pVer;pVer=vertexList.GetNext())
	{
		pNewVer=xPlate.vertex_list.Add(0);
		pNewVer->type=pVer->ciEdgeType;
		pNewVer->vertex=pVer->pos;
		if(pVer->ciEdgeType>1)
		{
			pNewVer->radius=pVer->arc.radius;
			pNewVer->sector_angle=pVer->arc.fSectAngle;
			pNewVer->center=pVer->arc.center;
			pNewVer->work_norm=pVer->arc.work_norm;
			pNewVer->column_norm=pVer->arc.column_norm;
		}
		pNewVer->m_bWeldEdge=pVer->m_bWeldEdge;
		pNewVer->m_bRollEdge=pVer->m_bRollEdge;
		pNewVer->manu_space=pVer->manu_space;
		//
		pNewVer->vertex*=g_pncSysPara.m_fMapScale;
		pNewVer->center*=g_pncSysPara.m_fMapScale;
	}
	BOLT_INFO* pBolt=NULL,*pNewBolt=NULL;
	xPlate.m_xBoltInfoList.Empty();
	for(pBolt=boltList.GetFirst();pBolt;pBolt=boltList.GetNext())
	{
		pNewBolt=xPlate.m_xBoltInfoList.Add(0);
		pNewBolt->CloneBolt(pBolt);
		pNewBolt->posX*=(float)g_pncSysPara.m_fMapScale;
		pNewBolt->posY*=(float)g_pncSysPara.m_fMapScale;
	}
	if(xPlate.mkpos.IsZero())
		InitMkPos(xPlate.mkpos,xPlate.mkVec);
	xPlate.mkpos*=g_pncSysPara.m_fMapScale;
	xPlate.m_bIncDeformed=g_pncSysPara.m_bIncDeformed;
	if(xPlate.mcsFlg.ciBottomEdge==(BYTE)-1)
		InitBtmEdgeIndex();
	CXhChar100 sAllRelPart(xPlate.GetPartNo());
	if(m_sRelatePartNo.Length>0)
		sAllRelPart.Printf("%s,%s",(char*)xPlate.GetPartNo(),(char*)m_sRelatePartNo);
	//���õ�ǰ����·��
	SetCurrentDirectory(file_path);
	if(g_pncSysPara.m_iPPiMode==0)
	{//һ��һ��ģʽ��һ��PPI�ļ�����һ������
		for(char* sKey=strtok(sAllRelPart,",");sKey;sKey=strtok(NULL,","))
		{
			if(strlen(sKey)<=0)
				continue;
			xPlate.SetPartNo(sKey);
			CBuffer buffer;
			xPlate.ToPPIBuffer(buffer);
			CString sFilePath;
			//ʹ��SetCurrentDirectory���ù���·����ʹ�����·��д�ļ�
			//���Ա����ļ����д�"."�޷��������·����̫���޷���������� wht 19-04-24
			//sFilePath.Format("%s%s.ppi",file_path,(char*)xPlate.GetPartNo());
			sFilePath.Format(".\\%s.ppi", (char*)xPlate.GetPartNo());	
			FILE* fp=fopen(sFilePath,"wb");
			if(fp)
			{
				long file_len=buffer.GetLength();
				fwrite(&file_len,sizeof(long),1,fp);
				fwrite(buffer.GetBufferPtr(),buffer.GetLength(),1,fp);
				fclose(fp);
			}
			else
			{
				DWORD nRetCode = GetLastError();
				logerr.Log("%s#�ְ�ppi�ļ�����ʧ��,�����#%d��·����%s%s", (char*)xPlate.GetPartNo(), nRetCode, file_path,sFilePath);
			}
		}
	}
	else if(g_pncSysPara.m_iPPiMode==1)
	{//һ����ģʽ��һ��PPI�ļ������������
		if(m_sRelatePartNo.Length>0)
			xPlate.m_sRelatePartNo.Copy(m_sRelatePartNo);
		sAllRelPart.Replace(","," ");
		CBuffer buffer;
		xPlate.ToPPIBuffer(buffer);
		CString sFilePath;
		//ʹ��SetCurrentDirectory���ù���·����ʹ�����·��д�ļ�
		//���Ա����ļ����д�"."�޷��������·����̫���޷���������� wht 19-04-24
		//sFilePath.Format("%s%s.ppi",file_path,(char*)sAllRelPart);
		sFilePath.Format(".\\%s.ppi", (char*)sAllRelPart);
		FILE* fp=fopen(sFilePath,"wb");
		if (fp)
		{
			long file_len = buffer.GetLength();
			fwrite(&file_len, sizeof(long), 1, fp);
			fwrite(buffer.GetBufferPtr(), buffer.GetLength(), 1, fp);
			fclose(fp);
		}
		else
		{
			DWORD nRetCode = GetLastError();
			logerr.Log("%s#�ְ�ppi�ļ�����ʧ��,�����#%d��·����%s%s", (char*)xPlate.GetPartNo(), nRetCode,file_path,sFilePath);
		}
	}
}
//��ʼ���ְ�ӹ�����ϵX�����ڵ�������
void CPlateProcessInfo::InitBtmEdgeIndex()
{
	int i=0,n=xPlate.vertex_list.GetNodeNum(),bottom_edge=-1,weld_edge=-1;
	DYN_ARRAY<f3dPoint> vertex_arr(n);
	for(PROFILE_VER* pVertex=xPlate.vertex_list.GetFirst();pVertex;pVertex=xPlate.vertex_list.GetNext(),i++)
	{
		vertex_arr[i]=pVertex->vertex;
		if(pVertex->type>1)
			vertex_arr[i].feature=-1;
		if(pVertex->m_bWeldEdge)
			weld_edge=i;
	}
	if(g_pncSysPara.m_iAxisXCalType==1)
	{	//���ȸ�����˨ȷ��X������������
		for(i=0;i<n;i++)
		{
			if(vertex_arr[i].feature==-1)
				continue;
			f3dLine edge_line(vertex_arr[i],vertex_arr[(i+1)%n]);
			CHashList<VERTEX_TAG> xVerTagHash;
			for(BOLT_INFO* pLs=xPlate.m_xBoltInfoList.GetFirst();pLs;pLs=xPlate.m_xBoltInfoList.GetNext())
			{
				f3dPoint ls_pt(pLs->posX,pLs->posY,0),perp;
				double fDist=0;
				if(SnapPerp(&perp,edge_line,ls_pt,&fDist)==FALSE||edge_line.PtInLine(perp,1)!=2)
					continue;	//��˨ͶӰ�㲻����������
				VERTEX_TAG* pTag=xVerTagHash.GetValue(ftoi(fDist));
				if(pTag==NULL)
					pTag=xVerTagHash.Add(ftoi(fDist));
				pTag->nLsNum+=1;
			}
			for(VERTEX_TAG* pTag=xVerTagHash.GetFirst();pTag;pTag=xVerTagHash.GetNext())
			{
				if(pTag->nLsNum>1 && pTag->nLsNum>vertex_arr[i].feature)
					vertex_arr[i].feature=pTag->nLsNum;
			}
		}
		int fMaxLs=0;
		for(i=0;i<n;i++)
		{
			if(vertex_arr[i].feature>fMaxLs)
			{
				bottom_edge=i;
				fMaxLs=vertex_arr[i].feature;
			}
		}
	}
	else if(g_pncSysPara.m_iAxisXCalType==2)
	{	//�Ժ��ӱ���Ϊ�ӹ�����ϵ��X������
		bottom_edge=weld_edge;
	}
	if(bottom_edge>-1)
		xPlate.mcsFlg.ciBottomEdge=(BYTE)bottom_edge;
	else
	{	//�������Ϊ�ӹ�����ϵ��X������
		GEPOINT vertice,prev_vec,edge_vec;
		double prev_edge_dist=0, edge_dist = 0, max_edge = 0;
		for(i=0;i<n;i++)
		{
			if(vertex_arr[i].feature==-1)
				continue;
			edge_vec=vertex_arr[(i+1)%n]-vertex_arr[i];
			edge_dist = edge_vec.mod();
			edge_vec/=edge_dist;	//��λ����ʸ��
			if(i>0&&prev_vec*edge_vec>EPS_COS)	//�������߱�����
				edge_dist+=edge_dist+prev_edge_dist;
			if(edge_dist>max_edge)
			{
				max_edge = edge_dist;
				bottom_edge=i;
			}
			prev_edge_dist=edge_dist;
			prev_vec=edge_vec;
		}
		xPlate.mcsFlg.ciBottomEdge=(BYTE)bottom_edge;
	}
}
//��ȡ�ְ������ļӹ�����
void CPlateProcessInfo::BuildPlateUcs()
{
	int i=0,n=xPlate.vertex_list.GetNodeNum();
	if(n<=0)
		return;
	DYN_ARRAY<GEPOINT> vertex_arr(n);
	for(PROFILE_VER* pVertex=xPlate.vertex_list.GetFirst();pVertex;pVertex=xPlate.vertex_list.GetNext(),i++)
		vertex_arr[i]=pVertex->vertex;
	if(xPlate.mcsFlg.ciBottomEdge==-1||xPlate.mcsFlg.ciBottomEdge>=n)
	{	//��ʼ���ӹ�����ϵ
		WORD bottom_edge;
		GEPOINT vertice,prev_vec,edge_vec;
		double prev_edge_dist=0, edge_dist = 0, max_edge = 0;
		for(i=0;i<n;i++)
		{
			edge_vec=vertex_arr[(i+1)%n]-vertex_arr[i];
			edge_dist = edge_vec.mod();
			edge_vec/=edge_dist;	//��λ����ʸ��
			if(!dim_vec.IsZero()&&fabs(dim_vec*edge_vec)>EPS_COS)
			{	//���Ȳ��������ֱ�ע������ͬ�ı���Ϊ��׼��
				bottom_edge=i;
				break;
			}
			if(i>0&&prev_vec*edge_vec>EPS_COS)	//�������߱�����
				edge_dist+=edge_dist+prev_edge_dist;
			if(edge_dist>max_edge)
			{
				max_edge = edge_dist;
				bottom_edge=i;
			}
			prev_edge_dist=edge_dist;
			prev_vec=edge_vec;
		}
		//
		xPlate.mcsFlg.ciBottomEdge=(BYTE)bottom_edge;
	}
	//����bottom_edge_i����ӹ�����ϵ
	int iEdge=xPlate.mcsFlg.ciBottomEdge;
	f3dPoint prev_pt = vertex_arr[(iEdge - 1 + n) % n];
	f3dPoint cur_pt=vertex_arr[iEdge];
	f3dPoint next_pt=vertex_arr[(iEdge+1)%n];
	f3dPoint next_next_pt = vertex_arr[(iEdge + 2) % n];
	ucs.origin = cur_pt;
	ucs.axis_x = next_pt - cur_pt;
	if (DistOf2dPtLine(next_pt.x, next_pt.y, cur_pt.x, cur_pt.y, next_next_pt.x, next_next_pt.y) > 0)
	{	//next_pt��Ϊ���㣬��next_next_ptΪ��׼����ӳֱ� wht 19-04-09
		ucs.axis_x = next_next_pt - cur_pt;
	}
	else if (DistOf2dPtLine(cur_pt.x, cur_pt.y, prev_pt.x, prev_pt.y, next_pt.x, next_pt.y) > 0)
	{	//cur_pt��λ���㣬��prev_pt��next_pt����λΪ��׼����ӳֱ� wht 19-04-09
		ucs.axis_x = next_pt - prev_pt;
	}
	normalize(ucs.axis_x);
	ucs.axis_y.Set(-ucs.axis_x.y,ucs.axis_x.x);
	ucs.axis_z=ucs.axis_x^ucs.axis_y;
	//��������ϵԭ��
	SCOPE_STRU scope=GetPlateScope(TRUE);
	ucs.origin+=scope.fMinX*ucs.axis_x;
	//����ְ���з�ת�����¸ְ�����ϵ
	if(xPlate.mcsFlg.ciOverturn==TRUE)
	{
		ucs.origin+=((scope.fMaxX-scope.fMinX)*ucs.axis_x);
		ucs.axis_x*=-1.0;
		ucs.axis_z*=-1.0;
	}
}
SCOPE_STRU CPlateProcessInfo::GetPlateScope(BOOL bVertexOnly,BOOL bDisplayMK/*=TRUE*/)
{
	SCOPE_STRU scope;
	scope.ClearScope();
	if(bVertexOnly)
	{
		PROFILE_VER* pPreVertex=xPlate.vertex_list.GetTail();
		for(PROFILE_VER *pVertex=xPlate.vertex_list.GetFirst();pVertex;pVertex=xPlate.vertex_list.GetNext())
		{
			f3dPoint pt=pPreVertex->vertex;
			coord_trans(pt,ucs,FALSE);
			scope.VerifyVertex(pt);
			if(pPreVertex->type==2)
			{
				f3dPoint axis_len(ucs.axis_x);
				f3dArcLine arcLine;
				arcLine.CreateMethod2(pPreVertex->vertex,pVertex->vertex,pPreVertex->work_norm,pPreVertex->sector_angle);
				f3dPoint startPt=f3dPoint(arcLine.Center())+axis_len*arcLine.Radius();
				f3dPoint endPt=f3dPoint(arcLine.Center())-axis_len*arcLine.Radius();
				coord_trans(startPt,ucs,FALSE);
				coord_trans(endPt,ucs,FALSE);
				scope.VerifyVertex(startPt);
				scope.VerifyVertex(endPt);
			}
			pPreVertex=pVertex;
		}
	}
	else
	{
		for(PROFILE_VER* pVertex=xPlate.vertex_list.GetFirst();pVertex;pVertex=xPlate.vertex_list.GetNext())
			scope.VerifyVertex(f3dPoint(pVertex->vertex.x,pVertex->vertex.y));
		for(BOLT_INFO *pHole=xPlate.m_xBoltInfoList.GetFirst();pHole;pHole=xPlate.m_xBoltInfoList.GetNext())
		{
			double radius=0.5*pHole->bolt_d;
			scope.VerifyVertex(f3dPoint(pHole->posX-radius,pHole->posY-radius));
			scope.VerifyVertex(f3dPoint(pHole->posX+radius,pHole->posY+radius));
		}
		if(xPlate.IsDisplayMK() && bDisplayMK)
			scope.VerifyVertex(f3dPoint(xPlate.mkpos.x,xPlate.mkpos.y));
	}
	return scope;
}
void CPlateProcessInfo::DrawPlate(f3dPoint *pOrgion/*=NULL*/,BOOL bCreateDimPos/*=FALSE*/,BOOL bDrawAsBlock/*=FALSE*/, GEPOINT *pPlateCenter /*= NULL*/)
{	
	AcDbBlockTableRecord *pBlockTableRecord=GetBlockTableRecord();
	if(pBlockTableRecord==NULL)
	{
		AfxMessageBox("��ȡ����¼ʧ��!");
		return;
	}
	AcGeMatrix3d moveMat,rotationMat;
	if(pOrgion)
	{	//����ƽ�ƻ�׼�㼰��ת�Ƕ�
		ads_point ptFrom,ptTo;
		ptFrom[X]=datumStartVertex.srcPos.x;
		ptFrom[Y]=datumStartVertex.srcPos.y;
		ptFrom[Z]=0;
		ptTo[X]=pOrgion->x+datumStartVertex.offsetPos.x;
		ptTo[Y]=pOrgion->y+datumStartVertex.offsetPos.y;
		ptTo[Z]=0;
		moveMat.setToTranslation(AcGeVector3d(ptTo[X]-ptFrom[X],ptTo[Y]-ptFrom[Y],ptTo[Z]-ptFrom[Z]));
		//
		GEPOINT src_vec=datumEndVertex.srcPos-datumStartVertex.srcPos;
		GEPOINT dest_vec=datumEndVertex.offsetPos-datumStartVertex.offsetPos;
		double fDegAngle=Cal2dLineAng(0,0,dest_vec.x,dest_vec.y)-Cal2dLineAng(0,0,src_vec.x,src_vec.y);
		rotationMat.setToRotation(fDegAngle,AcGeVector3d::kZAxis,AcGePoint3d(ptTo[X],ptTo[Y],ptTo[Z]));
	}
	AcGeMatrix3d blockMoveMat;
	if (pPlateCenter)
	{
		ads_point ptFrom, ptTo;
		ptFrom[X] = pPlateCenter->x;
		ptFrom[Y] = pPlateCenter->y;
		ptFrom[Z] = 0;
		ptTo[X] = 0;
		ptTo[Y] = 0;
		ptTo[Z] = 0;
		blockMoveMat.setToTranslation(AcGeVector3d(ptTo[X] - ptFrom[X], ptTo[Y] - ptFrom[Y], ptTo[Z] - ptFrom[Z]));
	}
	BOOL bOnlyIncTextEnt=TRUE;
	AcDbEntity *pEnt=NULL;
	AcDbObjectId entId;
	AcDbText *pPartNoText=NULL;
	m_cloneEntIdList.Empty();
	m_hashColneEntIdBySrcId.Empty();
	AcDbBlockTableRecord *pCurBlockTblRec = pBlockTableRecord;
	if (bDrawAsBlock&&pPlateCenter&&DRAGSET.BeginBlockRecord())
		pCurBlockTblRec = DRAGSET.RecordingBlockTableRecord();
	for(CAD_ENTITY *pRelaObj=m_xHashRelaEntIdList.GetFirst();pRelaObj;pRelaObj=m_xHashRelaEntIdList.GetNext())
	{
		acdbOpenAcDbEntity(pEnt,MkCadObjId(pRelaObj->idCadEnt),AcDb::kForRead);
		if(pEnt==NULL)
			continue;
		pEnt->close();
		AcDbEntity *pClone = (AcDbEntity *)pEnt->clone();
		if(pClone)
		{
			if(vertexList.GetNodeNum()<3)
				pClone->setColorIndex(1);	//
			if(pOrgion)
			{
				pClone->transformBy(moveMat);		//ƽ��
				pClone->transformBy(rotationMat);	//��ת
			}
			if (pPlateCenter&&bDrawAsBlock)
				pClone->transformBy(blockMoveMat);	//��Ϊ�����ʱ���ƶ���ԭ��λ�ã����ÿ�����Ϊ�������ĵ� wht 19-07-25
			DRAGSET.AppendAcDbEntity(pCurBlockTblRec,entId,pClone);
			m_cloneEntIdList.append(entId.asOldId());
			m_hashColneEntIdBySrcId.SetValue(pRelaObj->idCadEnt,entId.asOldId());
			pClone->close();
		}
		if(pEnt->isKindOf(AcDbText::desc()))
		{	
			AcDbText* pText=(AcDbText*)pEnt;
			CXhChar100 sText;
#ifdef _ARX_2007
			sText.Copy(_bstr_t(pText->textString()));
#else
			sText.Copy(pText->textString());
#endif
			if(pPartNoText==NULL&&strstr(sText,GetPartNo()))
				pPartNoText=pText;
		}
		else 
			bOnlyIncTextEnt=FALSE;
	}
	if(bCreateDimPos)
	{	//���ɱ�ע��
		AcDbObjectId pointId;
		AcDbPoint *pPoint=new AcDbPoint(AcGePoint3d(dim_pos.x,dim_pos.y,dim_pos.z));
		if(DRAGSET.AppendAcDbEntity(pCurBlockTblRec,pointId,pPoint))
		{
			pPoint->transformBy(moveMat);		//ƽ��
			pPoint->transformBy(rotationMat);	//��ת
			if (pPlateCenter&&bDrawAsBlock)
				pPoint->transformBy(blockMoveMat);	//��Ϊ�����ʱ���ƶ���ԭ��λ�ã����ÿ�����Ϊ�������ĵ� wht 19-07-25
			m_xMkDimPoint.idCadEnt=pointId.asOldId();
			AcGePoint3d pos=pPoint->position();
			m_xMkDimPoint.pos.Set(pos.x,pos.y,0);
		}
		pPoint->close();
	}
	//������ȡʧ�ܵĸְ������⴦��(���hashEntIdList��ֻ���ı���עĬ����ȡʧ��)
	if(pPartNoText&&bOnlyIncTextEnt)
	{
		CXhChar100 sText;
#ifdef _ARX_2007
		sText.Copy(_bstr_t(pPartNoText->textString()));
#else
		sText.Copy(pPartNoText->textString());
#endif
		double heigh=pPartNoText->height();
		double angle=pPartNoText->rotation();
		double len=DrawTextLength(sText,heigh,pPartNoText->textStyle());
		f3dPoint dim_norm(-sin(angle),cos(angle));
		f3dPoint origin=dim_pos;
		origin+=dim_vec*len*0.5;
		origin+=dim_norm*heigh*0.5;
		AcGeVector3d norm(0,0,1);
		AcGePoint3d acad_centre;
		Cpy_Pnt(acad_centre,origin);
		AcDbCircle *pCircle=new AcDbCircle(acad_centre,norm,len*0.5);
		pCircle->setColorIndex(1);
		DRAGSET.AppendAcDbEntity(pCurBlockTblRec,entId,pCircle);
		pCircle->close();
	}
	if(bDrawAsBlock&&pPlateCenter&&pCurBlockTblRec!=pBlockTableRecord)
		DRAGSET.EndBlockRecord(pBlockTableRecord,*pPlateCenter);
	pBlockTableRecord->close();
}

static BOOL IsInSector(double start_ang,double sector_ang,double verify_ang,BOOL bClockWise=TRUE)
{
	double ang=0;
	if(!bClockWise)	//��ʱ��
		ang=ftoi(verify_ang-start_ang)%360;
	else			//˳ʱ��
		ang=ftoi(start_ang-verify_ang)%360;
	if(ang<0)
		ang+=360;
	if(sector_ang>ang||fabs(sector_ang-ang)<EPS)
		return TRUE;
	else
		return FALSE;
}

void CPlateProcessInfo::CalEquidistantShape(double minDistance,ATOM_LIST<VERTEX> *pDestList)
{
	if (pDestList == NULL)
		return;
	for(VERTEX *vertex = vertexList.GetFirst();vertex;vertex=vertexList.GetNext())
	{
		VERTEX *vertexPre = vertexList.GetPrev();
		if (vertexPre == NULL)
		{
			vertexPre = vertexList.GetTail();
			vertexList.GetFirst();
		}
		else
			vertexList.GetNext();
		VERTEX *vertexNext = vertexList.GetNext();
		if (vertexNext == NULL)
		{
			vertexNext = vertexList.GetFirst();
			vertexList.GetTail();
		}
		else
			vertexList.GetPrev();
		f3dPoint curPt = vertex->pos;
		f3dPoint prePt = vertexPre->pos;
		f3dPoint nextPt = vertexNext->pos;
		f3dPoint preLineVec = curPt - prePt;
		normalize(preLineVec);
		f3dPoint nextLineVec = curPt - nextPt;
		normalize(nextLineVec);

		double angle = cal_angle_of_2vec(preLineVec,nextLineVec);
		double offset = minDistance/sin(angle/2);
		f3dPoint vec = preLineVec + nextLineVec;
		normalize(vec);
		if(DistOf2dPtLine(nextPt,prePt,curPt)<0)
			vec*=-1;	//�������ƫ�Ʒ���
		//����������㲢���������� wht 19-08-14
		VERTEX *pDestVertex = pDestList->append();
		*pDestVertex = *vertex;
		pDestVertex->pos = GEPOINT(curPt + vec*offset);
		double radius = pDestVertex->arc.radius;
		if (pDestVertex->ciEdgeType >= 2)
			pDestVertex->arc.radius = radius + offset;
		//pDestVertex->pos = (double*)curPt; //�˴����ܸ�ֵ�������޷����ü�϶ֵ��Ч wht 19-09-29
		pDestVertex->arc.radius = radius;
		if (pDestVertex->ciEdgeType >= 2)
		{	//ΪԲ������Բ����ӷ�Ƭ�����㣬������ȡ��˨��ʱ����©�� wht 19-08-14
			f3dArcLine arcLine;
			if (vertex->ciEdgeType == 2)	//ָ��Բ���н�
				arcLine.CreateMethod2(curPt, nextPt, vertex->arc.work_norm, vertex->arc.fSectAngle);
			else if (vertex->ciEdgeType == 3)
				arcLine.CreateEllipse(vertex->arc.center, curPt, nextPt, vertex->arc.column_norm,
					vertex->arc.work_norm, vertex->arc.radius);
			//��������Բ���������������仯
			int nSlices = CalArcResolution(arcLine.Radius(), arcLine.SectorAngle(), 1.0, 5.0, 18);
			double angle = arcLine.SectorAngle() / nSlices;
			for (int i = 1; i < nSlices; i++)
			{
				f3dPoint pos = arcLine.PositionInAngle(angle*i);
				VERTEX *pNewVertex=pDestList->append();
				pNewVertex->ciEdgeType = 1;
				pNewVertex->pos.Set(pos.x,pos.y,0);
			}
		}
	}
}
f2dRect CPlateProcessInfo::GetMinWrapRect(double minDistance/*=0*/,fPtList *pVertexList/*=NULL*/)
{
	f2dRect rect;
	rect.SetRect(f2dPoint(0,0),f2dPoint(0,0));
	if(m_bHasInnerDimPos)
	{
		ARRAY_LIST<AcDbObjectId> entIdList;
		for(CAD_ENTITY *pEnt=EnumFirstEnt();pEnt;pEnt=EnumNextEnt())
			entIdList.append(MkCadObjId(pEnt->idCadEnt));
		rect=GetCadEntRect(entIdList,minDistance);
		datumStartVertex.srcPos.Set(rect.topLeft.x,rect.bottomRight.y);
		datumEndVertex.srcPos.Set(rect.bottomRight.x,rect.bottomRight.y);
		datumStartVertex.offsetPos=datumStartVertex.srcPos-GEPOINT(rect.topLeft.x,rect.topLeft.y);
		datumEndVertex.offsetPos=datumEndVertex.srcPos-GEPOINT(rect.topLeft.x,rect.topLeft.y);
		return rect;
	}
	int i = 0, n=vertexList.GetNodeNum();
	if (n < 3)
		return rect;
	GECS tmucs,minUcs;
	SCOPE_STRU scope,min_scope;
	typedef VERTEX* VERTEX_PTR;
	ATOM_LIST<VERTEX> list;
	if (minDistance > EPS)
		CalEquidistantShape(minDistance,&list);
	else
	{
		for(VERTEX *pVertex=vertexList.GetFirst();pVertex;pVertex=vertexList.GetNext())
			list.append(*pVertex);
	}
	n = list.GetNodeNum();
	//��������������ʱʹ��list.GetNodeNum(����Ⱦ��ʱ�ὫԲ����Сֱ�߶δ���),����ᵼ���ڴ�Խ������ wht 19-08-18
	DYN_ARRAY<VERTEX_PTR> vertex_arr;
	vertex_arr.Resize(n);
	for(VERTEX *pVertex=list.GetFirst();pVertex;pVertex=list.GetNext(),i++)
		vertex_arr[i]=pVertex;
	tmucs.axis_z.Set(0,0,1);
	double minarea=10000000000000000;	//Ԥ����һ����
	fPtList ptList;
	fPtList minRectPtList;
	for(i=0;i<n;i++)
	{
		tmucs.axis_x=vertex_arr[(i+1)%n]->pos-vertex_arr[i]->pos;
		if(tmucs.axis_x.IsZero())
			continue;	//�ӽ��ص�
		tmucs.origin=vertex_arr[i]->pos;
		tmucs.axis_x.z = tmucs.origin.z = 0;
		tmucs.axis_y=tmucs.axis_z^tmucs.axis_x;
		normalize(tmucs.axis_x);
		normalize(tmucs.axis_y);
		if (vertex_arr[i]->m_bRollEdge)
			tmucs.origin -= tmucs.axis_y*vertex_arr[i]->manu_space;
		scope.ClearScope();
		ptList.Empty();
		for(int j=0;j<n;j++)
		{
			f3dPoint vertice = vertex_arr[j]->pos;
			coord_trans(vertice, tmucs, FALSE);
			scope.VerifyVertex(vertice);
			ptList.append(vertice);
			if(vertex_arr[j]->ciEdgeType>1)
			{
				f3dPoint ptS = vertex_arr[j]->pos;
				f3dPoint ptE = vertex_arr[(j + 1) % n]->pos;
				f3dArcLine arcLine;
				if (vertex_arr[j]->ciEdgeType == 2)	//ָ��Բ���н�
					arcLine.CreateMethod2(ptS,ptE,vertex_arr[j]->arc.work_norm,vertex_arr[j]->arc.fSectAngle);
				else if (vertex_arr[j]->ciEdgeType == 3)
					arcLine.CreateEllipse(vertex_arr[j]->arc.center,ptS,ptE,vertex_arr[j]->arc.column_norm,
											vertex_arr[j]->arc.work_norm,vertex_arr[j]->arc.radius);
				//��������Բ���������������仯
				int nSlices = CalArcResolution(arcLine.Radius(), arcLine.SectorAngle(), 1.0, 5.0, 18);
				double angle = arcLine.SectorAngle() / nSlices;
				for (int i = 1; i < nSlices; i++)
				{
					f3dPoint pos = arcLine.PositionInAngle(angle*i);
					coord_trans(pos,tmucs,FALSE);
					scope.VerifyVertex(pos);
					ptList.append(pos);
				}
				coord_trans(ptS, tmucs, FALSE);
				scope.VerifyVertex(ptS);
				coord_trans(ptE, tmucs, FALSE);
				scope.VerifyVertex(ptE);
				ptList.append(ptS);
				ptList.append(ptE);
			}
		}
		if(minarea>scope.wide()*scope.high())
		{
			minarea=scope.wide()*scope.high();
			min_scope=scope;
			minUcs = tmucs;
			datumStartVertex.index=i;
			datumEndVertex.index=(i+1)%n;
			minRectPtList.Empty();
			for (f3dPoint *pPt = ptList.GetFirst(); pPt; pPt = ptList.GetNext())
				minRectPtList.append(*pPt);
		}
	}
	if (pVertexList)
	{
		pVertexList->Empty();
		for (f3dPoint *pPt = minRectPtList.GetFirst(); pPt; pPt = minRectPtList.GetNext())
			pVertexList->append(*pPt);
	}

	rect.topLeft.Set(min_scope.fMinX,min_scope.fMaxY);
	rect.bottomRight.Set(min_scope.fMaxX,min_scope.fMinY);
	datumStartVertex.srcPos=vertex_arr[datumStartVertex.index]->pos;
	datumEndVertex.srcPos=vertex_arr[datumEndVertex.index]->pos;
	GEPOINT topLeft(rect.topLeft.x,rect.topLeft.y,0);
	GEPOINT startPt=datumStartVertex.srcPos,endPt=datumEndVertex.srcPos;
	coord_trans(startPt,minUcs,FALSE);
	coord_trans(endPt,minUcs,FALSE);
	datumStartVertex.offsetPos=startPt-topLeft;
	datumEndVertex.offsetPos=endPt-topLeft;
	return rect;
}
bool CPlateProcessInfo::InitLayoutVertexByBottomEdgeIndex(f2dRect &rect)
{
	if (xPlate.mcsFlg.ciBottomEdge == 0xFF)
		InitBtmEdgeIndex();
	rect.SetRect(f2dPoint(0, 0), f2dPoint(0, 0));
	if (m_bHasInnerDimPos)
	{
		ARRAY_LIST<AcDbObjectId> entIdList;
		for (CAD_ENTITY *pEnt = EnumFirstEnt(); pEnt; pEnt = EnumNextEnt())
			entIdList.append(MkCadObjId(pEnt->idCadEnt));
		rect = GetCadEntRect(entIdList, 0);
		datumStartVertex.srcPos.Set(rect.topLeft.x, rect.bottomRight.y);
		datumEndVertex.srcPos.Set(rect.bottomRight.x, rect.bottomRight.y);
		datumStartVertex.offsetPos = datumStartVertex.srcPos - GEPOINT(rect.topLeft.x, rect.topLeft.y);
		datumEndVertex.offsetPos = datumEndVertex.srcPos - GEPOINT(rect.topLeft.x, rect.topLeft.y);
	}
	else
	{
		int n = vertexList.GetNodeNum();
		if (n < 3)
			return false;
		int iCurIndex = xPlate.mcsFlg.ciBottomEdge;
		if (iCurIndex == 0xFF)
		{	//������δ��ʼ��,�޷����� wht 19-11-11
			return false;
		}
		SCOPE_STRU scope;
		DYN_ARRAY<VERTEX*> vertex_arr;
		vertex_arr.Resize(vertexList.GetNodeNum());
		ATOM_LIST<VERTEX> list;
		for (VERTEX *pVertex = vertexList.GetFirst(); pVertex; pVertex = vertexList.GetNext())
			list.append(*pVertex);
		int index = 0;
		for (VERTEX *pVertex = list.GetFirst(); pVertex; pVertex = list.GetNext(), index++)
			vertex_arr[index] = pVertex;
		GECS tmucs;
		tmucs.axis_z.Set(0, 0, 1);
		tmucs.axis_x = vertex_arr[(iCurIndex + 1) % n]->pos - vertex_arr[iCurIndex]->pos;
		if (tmucs.axis_x.IsZero())
			return false;	//�ӽ��ص�
		tmucs.origin = vertex_arr[iCurIndex]->pos;
		tmucs.axis_x.z = tmucs.origin.z = 0;
		tmucs.axis_y = tmucs.axis_z^tmucs.axis_x;
		normalize(tmucs.axis_x);
		normalize(tmucs.axis_y);
		if (vertex_arr[iCurIndex]->m_bRollEdge)
			tmucs.origin -= tmucs.axis_y*vertex_arr[iCurIndex]->manu_space;
		scope.ClearScope();
		for (int j = 0; j < n; j++)
		{
			f3dPoint vertice = vertex_arr[j]->pos;
			coord_trans(vertice, tmucs, FALSE);
			scope.VerifyVertex(vertice);
			if (vertex_arr[j]->ciEdgeType > 1)
			{
				f3dPoint ptS = vertex_arr[j]->pos;
				f3dPoint ptE = vertex_arr[(j + 1) % n]->pos;
				f3dArcLine arcLine;
				if (vertex_arr[j]->ciEdgeType == 2)	//ָ��Բ���н�
					arcLine.CreateMethod2(ptS, ptE, vertex_arr[j]->arc.work_norm, vertex_arr[j]->arc.fSectAngle);
				else if (vertex_arr[j]->ciEdgeType == 3)	//��Բ��
					arcLine.CreateEllipse(vertex_arr[j]->arc.center, ptS, ptE, vertex_arr[j]->arc.column_norm,
						vertex_arr[j]->arc.work_norm, vertex_arr[j]->arc.radius);
				//��������Բ���������������仯
				int nSlices = CalArcResolution(arcLine.Radius(), arcLine.SectorAngle(), 1.0, 5.0, 18);
				double angle = arcLine.SectorAngle() / nSlices;
				for (int i = 1; i < nSlices; i++)
				{
					f3dPoint pos = arcLine.PositionInAngle(angle*i);
					coord_trans(pos, tmucs, FALSE);
					scope.VerifyVertex(pos);
				}
				coord_trans(ptS, tmucs, FALSE);
				scope.VerifyVertex(ptS);
				coord_trans(ptE, tmucs, FALSE);
				scope.VerifyVertex(ptE);
			}
		}
		datumStartVertex.index = iCurIndex;
		datumEndVertex.index = (iCurIndex + 1) % n;
		//
		rect.topLeft.Set(scope.fMinX, scope.fMaxY);
		rect.bottomRight.Set(scope.fMaxX, scope.fMinY);
		datumStartVertex.srcPos = vertexList[datumStartVertex.index].pos;
		datumEndVertex.srcPos = vertexList[datumEndVertex.index].pos;
		GEPOINT topLeft(rect.topLeft.x, rect.topLeft.y, 0);
		GEPOINT startPt = datumStartVertex.srcPos, endPt = datumEndVertex.srcPos;
		coord_trans(startPt, tmucs, FALSE);
		coord_trans(endPt, tmucs, FALSE);
		datumStartVertex.offsetPos = startPt - topLeft;
		datumEndVertex.offsetPos = endPt - topLeft;
	}
	return true;
}
void CPlateProcessInfo::InitEdgeEntIdMap()
{
	AcDbEntity *pEnt=NULL;
	//1.��ʼ��ԭʼֱ�����¡ֱ��
	ARRAY_LIST<ACAD_LINEID> lineIdList;
	CHashList<ACAD_LINEID> hashCloneEntIdByLineId;
	for(CAD_ENTITY *pCadEnt=EnumFirstEnt();pCadEnt;pCadEnt=EnumNextEnt())
	{
		AcDbObjectId entId=MkCadObjId(pCadEnt->idCadEnt);
		acdbOpenAcDbEntity(pEnt,entId,AcDb::kForRead);
		CAcDbObjLife entLife(pEnt);
		if(pEnt==NULL)
			continue;
		if (pEnt->isKindOf(AcDbLine::desc()))
		{
			AcDbLine *pLine = (AcDbLine*)pEnt;
			ACAD_LINEID *pLineId = lineIdList.append();
			GEPOINT start(pLine->startPoint().x, pLine->startPoint().y, 0), end(pLine->endPoint().x, pLine->endPoint().y, 0);
			pLineId->Init(entId, start, end);
		}
		else if (pEnt->isKindOf(AcDbArc::desc()))
		{
			AcDbArc* pArc = (AcDbArc*)pEnt;
			AcGePoint3d startPt, endPt;
			pArc->getStartPoint(startPt);
			pArc->getEndPoint(endPt);
			GEPOINT start(startPt.x, startPt.y, 0), end(endPt.x, endPt.y, 0);
			ACAD_LINEID *pLineId = lineIdList.append();
			pLineId->Init(entId, start, end);
		}
		else if (pEnt->isKindOf(AcDbEllipse::desc()))
		{
			AcDbEllipse *pEllipse = (AcDbEllipse*)pEnt;
			AcGePoint3d startPt, endPt;
			pEllipse->getStartPoint(startPt);
			pEllipse->getEndPoint(endPt);
			GEPOINT start(startPt.x, startPt.y, 0), end(endPt.x, endPt.y, 0);
			ACAD_LINEID *pLineId = lineIdList.append();
			pLineId->Init(entId, start, end);
		}
	}
	for(ULONG *pId=m_cloneEntIdList.GetFirst();pId;pId=m_cloneEntIdList.GetNext())
	{
		AcDbObjectId entId=MkCadObjId(*pId);
		acdbOpenAcDbEntity(pEnt,entId,AcDb::kForRead);
		CAcDbObjLife entLife(pEnt);
		if(pEnt==NULL)
			continue;
		if (pEnt->isKindOf(AcDbLine::desc()))
		{
			AcDbLine *pLine = (AcDbLine*)pEnt;
			ACAD_LINEID *pLineId = hashCloneEntIdByLineId.Add(*pId);
			GEPOINT start(pLine->startPoint().x, pLine->startPoint().y, 0), end(pLine->endPoint().x, pLine->endPoint().y, 0);
			pLineId->Init(entId, start, end);
		}
		else if (pEnt->isKindOf(AcDbArc::desc()))
		{
			AcDbArc* pArc = (AcDbArc*)pEnt;
			AcGePoint3d startPt, endPt;
			pArc->getStartPoint(startPt);
			pArc->getEndPoint(endPt);
			GEPOINT start(startPt.x, startPt.y, 0), end(endPt.x, endPt.y, 0);
			ACAD_LINEID *pLineId = hashCloneEntIdByLineId.Add(*pId);
			pLineId->Init(entId, start, end);
		}
		else if (pEnt->isKindOf(AcDbEllipse::desc()))
		{
			AcDbEllipse *pEllipse = (AcDbEllipse*)pEnt;
			AcGePoint3d startPt, endPt;
			pEllipse->getStartPoint(startPt);
			pEllipse->getEndPoint(endPt);
			GEPOINT start(startPt.x, startPt.y, 0), end(endPt.x, endPt.y, 0);
			ACAD_LINEID *pLineId = hashCloneEntIdByLineId.Add(*pId);
			pLineId->Init(entId, start, end);
		}
	}
	//2.��ʼ�������߶�Ӧ��CADʵ��
	int n=vertexList.GetNodeNum();
	m_hashCloneEdgeEntIdByIndex.Empty();
	for(ACAD_LINEID *pLineId=hashCloneEntIdByLineId.GetFirst();pLineId;pLineId=hashCloneEntIdByLineId.GetNext())
		pLineId->UpdatePos();
	for(int i=0;i<n;i++)
	{
		VERTEX *pCurVertex=vertexList.GetByIndex(i);
		VERTEX *pNextVertex=vertexList.GetByIndex((i+1)%n);
		GEPOINT start=pCurVertex->pos,end=pNextVertex->pos;
		for(ACAD_LINEID *pLineId=lineIdList.GetFirst();pLineId;pLineId=lineIdList.GetNext())
		{
			ULONG *pCloneEntId=m_hashColneEntIdBySrcId.GetValue(pLineId->m_lineId.asOldId());
			ACAD_LINEID *pCloneLineId=pCloneEntId?hashCloneEntIdByLineId.GetValue(*pCloneEntId):NULL;
			if(pCloneLineId==NULL)
				continue;
			if(pLineId->m_ptStart.IsEqual(start)&&pLineId->m_ptEnd.IsEqual(end))
			{
				m_hashCloneEdgeEntIdByIndex.SetValue(i+1,*pCloneLineId);
				break;
			}
			else if(pLineId->m_ptStart.IsEqual(end)&&pLineId->m_ptEnd.IsEqual(start))
			{
				GEPOINT temp=pCloneLineId->m_ptStart;
				pCloneLineId->m_ptStart=pCloneLineId->m_ptEnd;
				pCloneLineId->m_ptEnd=temp;
				pCloneLineId->m_bReverse=TRUE;
				m_hashCloneEdgeEntIdByIndex.SetValue(i+1,*pCloneLineId);
				break;
			}
		}
	}
}
void CPlateProcessInfo::UpdateEdgeEntPos()
{
	for(ACAD_LINEID *pLineId=m_hashCloneEdgeEntIdByIndex.GetFirst();pLineId;pLineId=m_hashCloneEdgeEntIdByIndex.GetNext())
		pLineId->UpdatePos();
}

//���ݿ�¡�ְ���MKPOSλ�ã����¸�ӡ��λ�� wht 19-03-05
bool CPlateProcessInfo::SyncSteelSealPos()
{
	if (m_hashCloneEdgeEntIdByIndex.GetNodeNum() <= 0)
		return false;
	//1.��ȡCloneʵ��ǰ������
	ACAD_LINEID *pFirstLineId = m_hashCloneEdgeEntIdByIndex.GetValue(1);
	ACAD_LINEID *pSecondLineId = m_hashCloneEdgeEntIdByIndex.GetValue(2);
	if (pFirstLineId == NULL || pSecondLineId == NULL)
		return false;
	//2.��ȡSrcʵ���Ӧ��ǰ������
	AcDbObjectId srcFirstId=0, srcSecondId=0;
	for (ULONG *pId = m_hashColneEntIdBySrcId.GetFirst(); pId; pId = m_hashColneEntIdBySrcId.GetNext())
	{
		long idSrcEnt= m_hashColneEntIdBySrcId.GetCursorKey();
		if (*pId == pFirstLineId->m_lineId.asOldId())
			srcFirstId = MkCadObjId(idSrcEnt);
		else if (*pId == pSecondLineId->m_lineId.asOldId())
			srcSecondId = MkCadObjId(idSrcEnt);
		if (srcFirstId.asOldId() != 0 && srcSecondId.asOldId() != 0)
			break;
	}
	if (srcFirstId.asOldId() == 0 || srcSecondId.asOldId() == 0)
		return false;
	//3.��������ϵ
	UCS_STRU src_ucs, clone_ucs;
	pFirstLineId->UpdatePos();
	pSecondLineId->UpdatePos();
	clone_ucs.origin = pFirstLineId->m_ptEnd;
	clone_ucs.axis_x = pSecondLineId->m_ptEnd - pSecondLineId->m_ptStart;
	clone_ucs.axis_y = pFirstLineId->m_ptStart - pFirstLineId->m_ptEnd;
	normalize(clone_ucs.axis_x);
	normalize(clone_ucs.axis_y);
	clone_ucs.axis_z = clone_ucs.axis_x^clone_ucs.axis_y;
	normalize(clone_ucs.axis_z);
	clone_ucs.axis_y = clone_ucs.axis_z^clone_ucs.axis_x;
	normalize(clone_ucs.axis_y);
	//
	ACAD_LINEID srcFirstLineId, srcSecondLineId;
	srcFirstLineId.m_lineId = srcFirstId;
	srcFirstLineId.m_bReverse = pFirstLineId->m_bReverse;
	srcFirstLineId.UpdatePos();
	srcSecondLineId.m_lineId = srcSecondId;
	srcSecondLineId.m_bReverse = pSecondLineId->m_bReverse;
	srcSecondLineId.UpdatePos();
	src_ucs.origin = srcFirstLineId.m_ptEnd;
	src_ucs.axis_x = srcSecondLineId.m_ptEnd - srcSecondLineId.m_ptStart;
	src_ucs.axis_y = srcFirstLineId.m_ptStart - srcFirstLineId.m_ptEnd;
	normalize(src_ucs.axis_x);
	normalize(src_ucs.axis_y);
	src_ucs.axis_z = src_ucs.axis_x^src_ucs.axis_y;
	normalize(src_ucs.axis_z);
	src_ucs.axis_y = src_ucs.axis_z^src_ucs.axis_x;
	normalize(src_ucs.axis_y);
	//
	AcDbEntity *pEnt = NULL;
	if (m_xMkDimPoint.idCadEnt != 0)
	{
		Acad::ErrorStatus es=acdbOpenAcDbEntity(pEnt, MkCadObjId(m_xMkDimPoint.idCadEnt), AcDb::kForRead);
		if (es != Acad::eOk)
			return false;
	}
	CAcDbObjLife life(pEnt);
	if (pEnt == NULL || !pEnt->isKindOf(AcDbPoint::desc()))
		return false;
	AcDbPoint *pPoint = (AcDbPoint*)pEnt;
	GEPOINT mk_pos(pPoint->position().x, pPoint->position().y, pPoint->position().z);
	coord_trans(mk_pos, clone_ucs, FALSE);
	coord_trans(mk_pos, src_ucs, TRUE);
	GEPOINT mk_vec(1,0,0);
	vector_trans(mk_vec, clone_ucs, FALSE);
	vector_trans(mk_vec, src_ucs, TRUE);
	xPlate.mkpos = mk_pos;
	xPlate.mkVec = mk_vec;
	return true;
}

bool CPlateProcessInfo::AutoCorrectedSteelSealPos()
{
	ARRAY_LIST<AcDbObjectId> entIdList;
	for (ULONG *pId = m_cloneEntIdList.GetFirst(); pId; pId = m_cloneEntIdList.GetNext())
		entIdList.append(MkCadObjId(*pId));
	f2dRect rect = GetCadEntRect(entIdList);
	f2dPoint leftBtm(rect.topLeft.x, rect.bottomRight.y);
	//����ӡ�ֺ��Ƿ񳬳���Χ
	GEPOINT pos;
	GetSteelSealPos(pos);
	double fHalfW = g_pncSysPara.m_nMkRectWidth*0.5;
	if (pos.y - leftBtm.y < fHalfW)
	{
		pos.y = leftBtm.y + fHalfW * 1.1;
		UpdateSteelSealPos(pos);
		return true;
	}
	else
		return false;
}

bool CPlateProcessInfo::GetSteelSealPos(GEPOINT &pos)
{
	AcDbEntity* pEnt = NULL;
	acdbOpenAcDbEntity(pEnt, MkCadObjId(m_xMkDimPoint.idCadEnt), AcDb::kForRead);
	if (pEnt == NULL || !pEnt->isKindOf(AcDbPoint::desc()))
		return false;
	pEnt->close();
	AcDbPoint *pPoint = (AcDbPoint*)pEnt;
	m_xMkDimPoint.pos.Set(pPoint->position().x, pPoint->position().y, 0);
	pos = m_xMkDimPoint.pos;
	return true;
}

bool CPlateProcessInfo::UpdateSteelSealPos(GEPOINT &pos)
{
	AcDbEntity* pEnt = NULL;
	acdbOpenAcDbEntity(pEnt, MkCadObjId(m_xMkDimPoint.idCadEnt), AcDb::kForWrite);
	if (pEnt == NULL || !pEnt->isKindOf(AcDbPoint::desc()))
		return false;
	CAcDbObjLife life(pEnt);
	AcDbPoint *pPoint = (AcDbPoint*)pEnt;
	pPoint->setPosition(AcGePoint3d(pos.x, pos.y, pos.z));
	m_xMkDimPoint.pos.Set(pos.x, pos.y, 0);

	SyncSteelSealPos();
	return true;
}
//���¸ְ�ӹ���
void CPlateProcessInfo::RefreshPlateNum()
{
	if (partNumId == NULL)
		return;
	GetCurDwg()->setClayer(LayerTable::VisibleProfileLayer.layerId);
	acDocManager->lockDocument(curDoc(), AcAp::kWrite, NULL, NULL, true);
	AcDbEntity *pEnt = NULL;
	acdbOpenAcDbEntity(pEnt, partNumId, AcDb::kForWrite);
	CAcDbObjLife entLife(pEnt);
	CXhChar100 sValueG, sValueS, sValueM;
	if (pEnt->isKindOf(AcDbText::desc()))
	{
		AcDbText* pText = (AcDbText*)pEnt;
#ifdef _ARX_2007
		sValueG.Copy(_bstr_t(pText->textString()));
#else
		sValueG.Copy(pText->textString());
#endif
		if (g_pncSysPara.m_iDimStyle == 1 &&
			((strstr(sValueG, "����:") != NULL && strstr(g_pncSysPara.m_sPnNumKey, "����:") != NULL) ||
			(strstr(sValueG, "������") != NULL && strstr(g_pncSysPara.m_sPnNumKey, "������") != NULL)))
		{	//�޸ĸְ�ӹ��� wht 19-08-05
			sValueS.Printf("%s%d", (char*)g_pncSysPara.m_sPnNumKey, xPlate.feature);
		}
		else
		{
			for (char* sKey = strtok(sValueG, " \t"); sKey; sKey = strtok(NULL, " \t"))
			{
				if (strstr(sKey, "Q"))
				{
					sValueM.Copy(sKey);
					break;
				}
			}
			sValueS.Printf("-%.0f %s %d��", xPlate.m_fThick, (char*)sValueM, xPlate.feature);
		}
#ifdef _ARX_2007
		pText->setTextString(_bstr_t(sValueS));
#else
		pText->setTextString(sValueS);
#endif
	}
	else if (pEnt->isKindOf(AcDbMText::desc()))
	{
		CXhChar500 sText;
		AcDbMText *pMText = (AcDbMText*)pEnt;
#ifdef _ARX_2007
		sText.Copy(_bstr_t(pMText->contents()));
#else
		sText.Copy(pMText->contents());
#endif
		CXhChar500 sTempText(sText);
		for (char* sKey = strtok(sTempText, "\\P"); sKey; sKey = strtok(NULL, "\\P"))
		{
			CXhChar200 sTemp(sKey);
			if (g_pncSysPara.m_iDimStyle == 1 &&
				((strstr(sTemp, "����:") != NULL && strstr(g_pncSysPara.m_sPnNumKey, "����:") != NULL) ||
				(strstr(sTemp, "������") != NULL && strstr(g_pncSysPara.m_sPnNumKey, "������") != NULL)))
			{
				sTemp.Replace("\\P", "");
				sValueS.Printf("%s%d", (char*)g_pncSysPara.m_sPnNumKey, xPlate.feature);
				sText.Replace(sTemp, sValueS);	//���������� wht 19-08-13
#ifdef _ARX_2007
				pMText->setContents(_bstr_t(sText));
#else
				pMText->setContents(sText);
#endif
				break;
			}
			
		}
	}
	acDocManager->unlockDocument(curDoc());
}
SCOPE_STRU CPlateProcessInfo::GetCADEntScope(BOOL bIsColneEntScope /*= FALSE*/)
{
	SCOPE_STRU scope;
	scope.ClearScope();
	if (bIsColneEntScope)
	{
		for (unsigned long *pId = m_cloneEntIdList.GetFirst(); pId; pId = m_cloneEntIdList.GetNext())
			VerifyVertexByCADEntId(scope, MkCadObjId(*pId));
	}
	else
	{
		for (CPlateObject::CAD_ENTITY *pEnt = EnumFirstEnt(); pEnt; pEnt = EnumNextEnt())
			VerifyVertexByCADEntId(scope, MkCadObjId(pEnt->idCadEnt));
	}
	return scope;
}
bool CPlateProcessInfo::IsMarkPosCadEnt(int idCadEnt)
{
	if (idCadEnt == m_xMkDimPoint.idCadEnt)
		return true;	//����ְ�����ʱ������Ͽ�ʵ�� wht 19-10-16
	else
		return false;
}
//////////////////////////////////////////////////////////////////////////
//ExtractPlateInfo
//ExtractPlateProfile
CPNCModel::CPNCModel()
{
	Empty();
}
CPNCModel::~CPNCModel()
{

}
void CPNCModel::Empty()
{
	m_hashPlateInfo.Empty();
	m_xAllEntIdSet.Empty();
}
//��ȡʵ�������ID
AcDbObjectId CPNCModel::GetEntLineTypeId(AcDbEntity *pEnt,char* sLayer/*=NULL*/)
{
	if(pEnt==NULL)
		return NULL;
	CXhChar50 sLineTypeName,sLayerName;
#ifdef _ARX_2007
	sLineTypeName.Copy((char*)_bstr_t(pEnt->linetype()));
	sLayerName.Copy((char*)_bstr_t(pEnt->layer()));
#else
	sLineTypeName.Copy(pEnt->linetype());
	sLayerName.Copy(pEnt->layer());
#endif
	AcDbObjectId linetypeId;
	if(stricmp(sLineTypeName,"ByLayer")==0)
	{	//�������
		AcDbLayerTableRecord *pLayerTableRecord;
		acdbOpenObject(pLayerTableRecord,pEnt->layerId(),AcDb::kForRead);
		pLayerTableRecord->close();
		linetypeId=pLayerTableRecord->linetypeObjectId();
	}
	else if(stricmp(sLineTypeName,"ByBlock")==0)
		linetypeId=m_idSolidLine;		//���ͼԪ����������ΪByBlock,�����;���ʵ��
	else
		linetypeId=pEnt->linetypeId();
	if(sLayer)
		strcpy(sLayer,sLayerName);
	return linetypeId;
}
//����bpoly�����ʼ���ְ��������
bool GetLineTypeIdByLineType(const char* line_type,AcDbObjectId &lineTypeId)
{
	lineTypeId=0;
	//��ȡͼ���Ӧ������Id
	AcDbLinetypeTable *pLinetypeTbl;
	GetCurDwg()->getLinetypeTable(pLinetypeTbl,AcDb::kForRead);
	if(pLinetypeTbl)
	{
#ifdef _ARX_2007
		pLinetypeTbl->getAt((ACHAR*)_bstr_t(line_type), lineTypeId);
#else
		pLinetypeTbl->getAt(line_type, lineTypeId);
#endif
		pLinetypeTbl->close();
		if (lineTypeId.isNull())
			return false;
		else
			return true;
	}
	else
		return false;
}
void CPNCModel::ExtractPlateProfile(CHashSet<AcDbObjectId>& selectedEntIdSet)
{
	//���һ���ض�ͼ��
	CXhChar16 sNewLayer("pnc_layer"),sLineType=g_pncSysPara.m_sProfileLineType;
	CreateNewLayer(sNewLayer, sLineType,AcDb::kLnWt013,1,m_idNewLayer,m_idSolidLine);
	m_hashSolidLineTypeId.SetValue(m_idSolidLine.asOldId(),m_idSolidLine);
	AcDbObjectId lineTypeId=0;
	if(GetLineTypeIdByLineType("�����",lineTypeId))
		m_hashSolidLineTypeId.SetValue(lineTypeId.asOldId(),lineTypeId);
	//����Spline��ȡ������������Ϣ(�ֶ��߶μ���)
	CSymbolRecoginzer symbols;
	AcDbEntity *pEnt=NULL;
	ATOM_LIST<ACAD_CIRCLE> cirList;
	CHashStrList< ATOM_LIST<ACAD_LINEID> > hashLineArrByPosKeyStr;
	for(AcDbObjectId objId=selectedEntIdSet.GetFirst();objId;objId=selectedEntIdSet.GetNext())
	{
		acdbOpenAcDbEntity(pEnt,objId,AcDb::kForRead);
		if(pEnt)
			pEnt->close();
		if(pEnt->isKindOf(AcDbSpline::desc()))
		{
			AcGePoint3d pt;
			AcDbSpline* pSpline=(AcDbSpline*)pEnt;
			CSymbolEntity* pSymbol=symbols.listSymbols.AttachObject();
			for(int i=0;i<pSpline->numFitPoints()-1;i++)
			{
				CSymbolEntity::LINE* pLine=pSymbol->listSonlines.AttachObject();
				pSymbol->ciSymbolType=1;	//��Ĭ��Ϊ������S�ͷ���
				pSpline->getFitPointAt(i,pt);
				Cpy_Pnt(pLine->start,pt);
				pSpline->getFitPointAt(i+1,pt);
				Cpy_Pnt(pLine->end,pt);
			}
		}
		else if(pEnt->isKindOf(AcDbLine::desc()))
		{
			AcDbLine *pLine=(AcDbLine*)pEnt;
			AcGePoint3d acad_start_pt,acad_end_pt;
			f3dPoint start_point,end_point;
			//ʼ�˵�
			pLine->getStartPoint(acad_start_pt);
			Cpy_Pnt(start_point,acad_start_pt);
			pLine->getEndPoint(acad_end_pt);
			Cpy_Pnt(end_point,acad_end_pt);
			double len=DISTANCE(start_point,end_point);
			CXhChar50 startKeyStr(start_point);
			ATOM_LIST<ACAD_LINEID> *pStartLineList=hashLineArrByPosKeyStr.GetValue(startKeyStr);
			if(pStartLineList==NULL)
				pStartLineList=hashLineArrByPosKeyStr.Add(startKeyStr);
			pStartLineList->append(ACAD_LINEID(objId,len));
			//�ն˵�
			CXhChar50 endKeyStr(end_point);
			ATOM_LIST<ACAD_LINEID> *pEndLineList=hashLineArrByPosKeyStr.GetValue(endKeyStr);
			if(pEndLineList==NULL)
				pEndLineList=hashLineArrByPosKeyStr.Add(endKeyStr);
			pEndLineList->append(ACAD_LINEID(objId,len));
		}
		else if(pEnt->isKindOf(AcDbArc::desc()))
		{
			AcDbArc *pArc=(AcDbArc*)pEnt;
			AcGePoint3d acad_start_pt,acad_end_pt;
			f3dPoint start_point,end_point;
			//ʼ�˵�
			pArc->getStartPoint(acad_start_pt);
			Cpy_Pnt(start_point,acad_start_pt);
			pArc->getEndPoint(acad_end_pt);
			Cpy_Pnt(end_point,acad_end_pt);
			double len=DISTANCE(start_point,end_point);
			CXhChar50 startKeyStr(start_point);
			ATOM_LIST<ACAD_LINEID> *pStartLineList=hashLineArrByPosKeyStr.GetValue(startKeyStr);
			if(pStartLineList==NULL)
				pStartLineList=hashLineArrByPosKeyStr.Add(startKeyStr);
			pStartLineList->append(ACAD_LINEID(objId,len));
			//�ն˵�
			CXhChar50 endKeyStr(end_point);
			ATOM_LIST<ACAD_LINEID> *pEndLineList=hashLineArrByPosKeyStr.GetValue(endKeyStr);
			if(pEndLineList==NULL)
				pEndLineList=hashLineArrByPosKeyStr.Add(endKeyStr);
			pEndLineList->append(ACAD_LINEID(objId,len));
		}
		else if (pEnt->isKindOf(AcDbEllipse::desc()))
		{
			AcDbEllipse* pEllipse = (AcDbEllipse*)pEnt;
			AcGePoint3d acad_start_pt, acad_end_pt;
			f3dPoint startPt, endPt;
			pEllipse->getStartPoint(acad_start_pt);
			Cpy_Pnt(startPt, acad_start_pt);
			pEllipse->getEndPoint(acad_end_pt);
			Cpy_Pnt(endPt, acad_end_pt);
			double len = DISTANCE(startPt, endPt);
			CXhChar50 startKeyStr(startPt);
			ATOM_LIST<ACAD_LINEID> *pStartLineList = hashLineArrByPosKeyStr.GetValue(startKeyStr);
			if (pStartLineList == NULL)
				pStartLineList = hashLineArrByPosKeyStr.Add(startKeyStr);
			pStartLineList->append(ACAD_LINEID(objId, len));
			//�ն˵�
			CXhChar50 endKeyStr(endPt);
			ATOM_LIST<ACAD_LINEID> *pEndLineList = hashLineArrByPosKeyStr.GetValue(endKeyStr);
			if (pEndLineList == NULL)
				pEndLineList = hashLineArrByPosKeyStr.Add(endKeyStr);
			pEndLineList->append(ACAD_LINEID(objId, len));
		}
		else if(pEnt->isKindOf(AcDbCircle::desc()))
		{
			AcDbCircle *pCir=(AcDbCircle*)pEnt;
			GEPOINT center;
			Cpy_Pnt(center,pCir->center());
			cirList.append(ACAD_CIRCLE(objId,pCir->radius(),center));
		}
	}
	const int WELD_MAX_HEIGHT=20;
	for(ATOM_LIST<ACAD_LINEID> *pList=hashLineArrByPosKeyStr.GetFirst();pList;pList=hashLineArrByPosKeyStr.GetNext())
	{
		if(pList->GetNodeNum()!=1)
			continue;
		ACAD_LINEID *pLineId=pList->GetFirst();
		if(pLineId&&pLineId->m_fLen<WELD_MAX_HEIGHT)
			selectedEntIdSet.DeleteNode(pLineId->m_lineId.asOldId());
	}
	selectedEntIdSet.Clean();
	//��Բ������Բ������Ӹ���ͼԪ
	double fMaxEdge=0,fMinEdge=200000;
	f3dPoint ptS,ptE;
	AcDbBlockTableRecord *pBlockTableRecord=GetBlockTableRecord();
	CHashSet<AcDbObjectId> xAssistCirSet;
	for(AcDbObjectId objId=selectedEntIdSet.GetFirst();objId;objId=selectedEntIdSet.GetNext())
	{
		acdbOpenAcDbEntity(pEnt,objId,AcDb::kForRead);
		if(pEnt==NULL)
			continue;
		pEnt->close();
		CXhChar50 sLayerName;
		AcDbObjectId lineId=GetEntLineTypeId(pEnt,sLayerName);
		if(g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_LINETYPE)
		{	//�����͹���
			if (m_hashSolidLineTypeId.GetValue(lineId.asOldId()) == NULL)
				continue;
		}
		else if(g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_LAYER)
		{	//��ͼ�����
			if(g_pncSysPara.IsNeedFilterLayer(sLayerName))
				continue;
		}
		else if(g_pncSysPara.m_ciRecogMode==CPNCSysPara::FILTER_BY_COLOR)
		{	//����ɫ����
			if(GetEntColorIndex(pEnt)!=g_pncSysPara.m_ciProfileColorIndex)
				continue;
		} 
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
			xAssistCirSet.SetValue(lineId.asOldId(),lineId);
			pLine->close();
			//
			Cpy_Pnt(ptS,startPt);
			Cpy_Pnt(ptE,endPt);
			double fDist=DISTANCE(ptS,ptE);
			if(fDist>fMaxEdge)
				fMaxEdge=fDist;
			if(fDist<fMinEdge)
				fMinEdge=fDist;
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
			xAssistCirSet.SetValue(circleId.asOldId(),circleId);
			pCircle1->close();
			//�յ㴦��Ӹ���СԲ
			AcDbCircle *pCircle2=new AcDbCircle(endPt,norm,2);
			pBlockTableRecord->appendAcDbEntity(circleId,pCircle2);
			xAssistCirSet.SetValue(circleId.asOldId(),circleId);
			pCircle2->close();
			//
			if(pArc->radius()>fMaxEdge)
				fMaxEdge=pArc->radius();
			if(pArc->radius()<fMinEdge)
				fMinEdge=pArc->radius();
		}
		else if(pEnt->isKindOf(AcDbLine::desc()))
		{
			AcDbLine* pLine=(AcDbLine*)pEnt;
			Cpy_Pnt(ptS,pLine->startPoint());
			Cpy_Pnt(ptE,pLine->endPoint());
			double fDist=DISTANCE(ptS,ptE);
			if(fDist>fMaxEdge)
				fMaxEdge=fDist;
			if(fDist<fMinEdge)
				fMinEdge=fDist;
		}
	}
	pBlockTableRecord->close();
	//1������������ͼԪ�Ƶ��ض�ͼ��
	CHashList<CXhChar50> hashLayerList;
	for(AcDbObjectId objId=m_xAllEntIdSet.GetFirst();objId;objId=m_xAllEntIdSet.GetNext())
	{
		acdbOpenAcDbEntity(pEnt,objId,AcDb::kForWrite);
		if(pEnt==NULL)
			continue;
		CXhChar50 sLayerName;
		AcDbObjectId curLineId=GetEntLineTypeId(pEnt,sLayerName);
		hashLayerList.SetValue(objId.asOldId(),sLayerName);
		//�����ѡ��ʵ�壺ͳһ�Ƶ��ض�ͼ��
		if(selectedEntIdSet.GetValue(pEnt->id().asOldId()).isNull())
		{
			pEnt->setLayer(m_idNewLayer);
			pEnt->close();
			continue;
		}
		if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_LAYER)
		{	//��ͼ�����
			if (g_pncSysPara.IsNeedFilterLayer(sLayerName))
			{
				pEnt->setLayer(m_idNewLayer);
				pEnt->close();
				continue;
			}
			//���˸���ͼԪ����ע��ͼ��ȣ�
			if (!pEnt->isKindOf(AcDbLine::desc()) &&	//ֱ��
				!pEnt->isKindOf(AcDbArc::desc()) &&		//Բ��
				!pEnt->isKindOf(AcDbRegion::desc()) &&	//����
				!pEnt->isKindOf(AcDbPolyline::desc()))	//���߶�
			{
				pEnt->setLayer(m_idNewLayer);
				pEnt->close();
				continue;
			}
		}
		else if(g_pncSysPara.m_ciRecogMode==CPNCSysPara::FILTER_BY_COLOR)
		{	//����ɫ����
			int iColor=GetEntColorIndex(pEnt);
			if( iColor!=g_pncSysPara.m_ciProfileColorIndex||
				iColor==g_pncSysPara.m_ciBendLineColorIndex)
			{
				pEnt->setLayer(m_idNewLayer);
				pEnt->close();
				continue;
			}
			//���˸���ͼԪ����ע��ͼ��ȣ�
			if (!pEnt->isKindOf(AcDbLine::desc()) &&	//ֱ��
				!pEnt->isKindOf(AcDbArc::desc()) &&		//Բ��
				!pEnt->isKindOf(AcDbRegion::desc()) &&	//����
				!pEnt->isKindOf(AcDbPolyline::desc()))	//���߶�
			{
				pEnt->setLayer(m_idNewLayer);
				pEnt->close();
				continue;
			}
		}
		else if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_LINETYPE)
		{	//�����͹���
			//���˷�����������ͼԪ
			if (m_hashSolidLineTypeId.GetValue(curLineId.asOldId()) == NULL)
			{
				pEnt->setLayer(m_idNewLayer);
				pEnt->close();
				continue;
			}
			//���˻�����
			if (pEnt->isKindOf(AcDbLine::desc()) && g_pncSysPara.IsBendLine((AcDbLine*)pEnt, &symbols))
			{
				pEnt->setLayer(m_idNewLayer);
				pEnt->close();
				continue;
			}
			//���˸���ͼԪ����ע��ͼ��ȣ�
			if (!pEnt->isKindOf(AcDbLine::desc())&&		//ֱ��
				!pEnt->isKindOf(AcDbArc::desc())&&		//Բ��
				!pEnt->isKindOf(AcDbRegion::desc()) &&	//����
				!pEnt->isKindOf(AcDbPolyline::desc()))	//���߶�
			{
					pEnt->setLayer(m_idNewLayer);
					pEnt->close();
					continue;
			}
		}
		pEnt->close();
	}
	//��ʼ���µ����״̬
	for(CPlateProcessInfo* pInfo=m_hashPlateInfo.GetFirst();pInfo;pInfo=m_hashPlateInfo.GetNext())
	{
		for(ACAD_CIRCLE *pCir=cirList.GetFirst();pCir;pCir=cirList.GetNext())
		{
			if(pCir->IsInCircle(pInfo->dim_pos))
			{
				pInfo->m_bIslandDetection=TRUE;
				break;
			}
		}
	}
	//2����ʼ���ְ���������Ϣ
	CShieldCadLayer shieldLayer(sNewLayer,TRUE);	//���β���Ҫ��ͼ��
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
	//3����ԭͼԪ����ͼ��
	for(CXhChar50 *pLayer=hashLayerList.GetFirst();pLayer;pLayer=hashLayerList.GetNext())
	{
		long handle=hashLayerList.GetCursorKey();
		AcDbObjectId objId=m_xAllEntIdSet.GetValue(handle);
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
}
//����ְ�һ���ŵ����
void CPNCModel::MergeManyPartNo()
{
	for(CPlateProcessInfo* pPlateProcess=model.EnumFirstPlate(TRUE);pPlateProcess;pPlateProcess=model.EnumNextPlate(TRUE))
	{
		if(pPlateProcess->vertexList.GetNodeNum()<=3)
			continue;
		pPlateProcess->pnTxtIdList.SetValue(pPlateProcess->partNoId.asOldId(),pPlateProcess->partNoId);
		m_hashPlateInfo.push_stack();
		for(CPlateProcessInfo* pTemPlate=EnumNextPlate(TRUE);pTemPlate;pTemPlate=EnumNextPlate(TRUE))
		{
			if(!pPlateProcess->IsInPlate(pTemPlate->dim_pos))
				continue;
			if(pPlateProcess->m_sRelatePartNo.Length<=0)
				pPlateProcess->m_sRelatePartNo.Copy(pTemPlate->GetPartNo());
			else
				pPlateProcess->m_sRelatePartNo.Append(CXhChar16(",%s",(char*)pTemPlate->GetPartNo()));
			pPlateProcess->pnTxtIdList.SetValue(pTemPlate->partNoId.asOldId(),pTemPlate->partNoId);
			m_hashPlateInfo.DeleteCursor();
		}
		m_hashPlateInfo.pop_stack();
	}
	m_hashPlateInfo.Clean();
}
void CPNCModel::LayoutPlates(BOOL bRelayout)
{
	if(m_hashPlateInfo.GetNodeNum()<=0)
	{
		logerr.Log("ȱ�ٸְ���Ϣ��������ȷ��ȡ�ְ���Ϣ��");
		return;
	}
	//
	f2dRect minRect;
	int minDistance=g_pncSysPara.m_nMinDistance;
	int hight=g_pncSysPara.m_nMapWidth;
	int paperLen=(g_pncSysPara.m_nMapLength<=0)?100000:g_pncSysPara.m_nMapLength;
	DRAGSET.ClearEntSet();
	CXhPtrSet<CPlateProcessInfo> needUpdatePlateList;
	CHashStrList<CDrawingRect> hashDrawingRectByLabel;
	for(CPlateProcessInfo *pPlate=m_hashPlateInfo.GetFirst();pPlate;pPlate=m_hashPlateInfo.GetNext())
	{
		fPtList ptList;
		minRect=pPlate->GetMinWrapRect((double)minDistance,&ptList);
		CDrawingRect *pRect=hashDrawingRectByLabel.Add(pPlate->GetPartNo());
		pRect->m_pDrawing=pPlate;
		pRect->height=minRect.Height();
		pRect->width=minRect.Width();
		pRect->m_vertexArr.Empty();
		for (f3dPoint *pPt = ptList.GetFirst(); pPt; pPt = ptList.GetNext())
			pRect->m_vertexArr.append(GEPOINT(*pPt));
		pRect->topLeft.Set(minRect.topLeft.x,minRect.topLeft.y);
	}
	if(bRelayout==CPNCSysPara::LAYOUT_PRINT)
	{
		double paperX=0;
		while(hashDrawingRectByLabel.GetNodeNum()>0)
		{
			CDrawingRectLayout rectLayout;
			for(CDrawingRect *pRect=hashDrawingRectByLabel.GetFirst();pRect;pRect=hashDrawingRectByLabel.GetNext())
				rectLayout.drawRectArr.Add(*pRect);
			if (rectLayout.Relayout(hight, paperLen) == FALSE)
			{	//���еİ嶼����ʧ��
				for (CDrawingRect *pRect = hashDrawingRectByLabel.GetFirst(); pRect; pRect = hashDrawingRectByLabel.GetNext())
				{
					CPlateProcessInfo *pPlateDraw = (CPlateProcessInfo*)pRect->m_pDrawing;
					if (pRect->m_bException)
						logerr.Log("�ְ�%s�Ű�ʧ��,�ְ���ο�ȴ���ָ����ͼ���!",(char*)pPlateDraw->GetPartNo());
					else
						logerr.Log("�ְ�%s�Ű�ʧ��!", (char*)pPlateDraw->GetPartNo());
				}
				break;
			}
			else
			{	//���ֳɹ����������п���ĳЩ��û�в��ֳɹ�
				AcDbBlockTableRecord *pBlockTableRecord = GetBlockTableRecord();
				CreateAcadLine(pBlockTableRecord, f3dPoint(paperX, 0), f3dPoint(paperX + paperLen, 0));
				CreateAcadLine(pBlockTableRecord, f3dPoint(paperX, -hight), f3dPoint(paperX + paperLen, -hight));
				CreateAcadLine(pBlockTableRecord, f3dPoint(paperX, 0), f3dPoint(paperX, -hight));
				CreateAcadLine(pBlockTableRecord, f3dPoint(paperX + paperLen, 0), f3dPoint(paperX + paperLen, -hight));
				pBlockTableRecord->close();
				f3dPoint topLeft;
				for (int i = 0; i < rectLayout.drawRectArr.GetSize(); i++)
				{
					CDrawingRect drawingRect = rectLayout.drawRectArr[i];
					CPlateProcessInfo *pPlate = (CPlateProcessInfo*)drawingRect.m_pDrawing;
					if (!drawingRect.m_bLayout)
						continue;
					topLeft = drawingRect.topLeft;
					topLeft.x += paperX;
					GEPOINT center(topLeft.x + drawingRect.width*0.5, topLeft.y - drawingRect.height*0.5);
					pPlate->DrawPlate(&topLeft, FALSE, TRUE, &center);
					//
					hashDrawingRectByLabel.DeleteNode(pPlate->GetPartNo());
				}
				paperX += (paperLen + 50);
			}
		}
		//�����Ű溯��������Ű��쳣�ְ� wht 18-11-27
		/*if(hashExceptionPlateByLabel.GetNodeNum()>0)
		{
			for(CPlateProcessInfo **ppPlateInfo=hashExceptionPlateByLabel.GetFirst();ppPlateInfo;ppPlateInfo=hashExceptionPlateByLabel.GetNext())
			{
				CPlateProcessInfo *pPlateInfo=*ppPlateInfo;
				logerr.Log("����%s���Ű��쳣",(char*)pPlateInfo->GetPartNo());
			}
		}*/
	}
	else if (bRelayout == CPNCSysPara::LAYOUT_SEG)
	{
		CSortedModel sortedModel(this);
		int nSegCount = 0;
		SEGI prevSegI, curSegI;
		f3dPoint datum_pos;
		const int PAPER_WIDTH = 1500;
		//���
		for (CPlateProcessInfo *pPlate = sortedModel.EnumFirstPlate(); pPlate; pPlate = sortedModel.EnumNextPlate())
		{
			CXhChar16 sPartNo = pPlate->GetPartNo();
			ParsePartNo(sPartNo, &curSegI, NULL, "SHPGT");
			CXhChar16 sSegStr = curSegI.ToString();
			if (sSegStr.Length > 3)
			{	//�κ��ַ������ȴ���3ʱ���ٴδӶκ�����ȡһ�ηֶκţ�����5401-48����ţ� wht 19-03-07
				SEGI segI;
				if (ParsePartNo(sSegStr, &segI, NULL, "SHPGT"))
					curSegI = segI;
			}
			f2dRect rect;
			pPlate->InitLayoutVertexByBottomEdgeIndex(rect);
			if (rect.Width() < EPS2 || rect.Height() <= EPS2)
			{	//�����������޷�����ְ��������ʱ����CAD���� wht 19-03-12
				ARRAY_LIST<AcDbObjectId> entIdList;
				for (CPlateProcessInfo::CAD_ENTITY *pEnt = pPlate->EnumFirstEnt(); pEnt; pEnt = pPlate->EnumNextEnt())
					entIdList.append(MkCadObjId(pEnt->idCadEnt));
				rect = GetCadEntRect(entIdList);
			}
			double wide = (rect.Width() + rect.Height())*1.5;
			double high = rect.Height();
			f3dPoint leftBtm(datum_pos.x, datum_pos.y + high);
			pPlate->DrawPlate(&leftBtm, TRUE);
			datum_pos.x += wide;
			if (prevSegI.iSeg == 0)
				prevSegI = curSegI;
			else if (prevSegI.iSeg != curSegI.iSeg)
			{
				prevSegI = curSegI;
				datum_pos.x = 0;
				datum_pos.y -= PAPER_WIDTH;
			}
		}
		for (CPlateProcessInfo *pPlate = sortedModel.EnumFirstPlate(); pPlate; pPlate = sortedModel.EnumNextPlate())
		{
			if (pPlate->AutoCorrectedSteelSealPos())
				needUpdatePlateList.append(pPlate);
		}
	}
	else
	{
		for(CPlateProcessInfo *pPlate=m_hashPlateInfo.GetFirst();pPlate;pPlate=m_hashPlateInfo.GetNext())
			pPlate->DrawPlate(NULL);
	}
	//
	SCOPE_STRU scope;
	DRAGSET.GetDragScope(scope);
	ads_point base;
	base[X] = scope.fMinX;
	base[Y] = scope.fMaxY;
	base[Z] = 0;
	DragEntSet(base, "���ȡ����ͼ�Ĳ����");
	//
	if (bRelayout == CPNCSysPara::LAYOUT_SEG)
	{
		for (CPlateProcessInfo *pPlate = model.EnumFirstPlate(TRUE); pPlate; pPlate = model.EnumNextPlate(TRUE))
			pPlate->InitEdgeEntIdMap();	//��ʼ�������߶�Ӧ��ϵ
		for (CPlateProcessInfo *pPlate = needUpdatePlateList.GetFirst(); pPlate; pPlate = needUpdatePlateList.GetNext())
		{	//�����ֺ���λ��֮��ͬ������PPI�ļ��и�ӡ��λ��
			pPlate->SyncSteelSealPos();
			//����PPI�ļ�
			CString file_path;
			GetCurWorkPath(file_path);
			pPlate->CreatePPiFile(file_path);
		}
	}
}

CPlateProcessInfo* CPNCModel::GetPlateInfo(AcDbObjectId partNoEntId)
{
	for (CPlateProcessInfo *pPlate = EnumFirstPlate(FALSE); pPlate; pPlate = EnumNextPlate(FALSE))
	{
		if (pPlate->partNoId == partNoEntId)
			return pPlate;
	}
	return NULL;
}

void CPNCModel::WritePrjTowerInfoToCfgFile(const char* cfg_file_path)
{
	if(cfg_file_path==NULL||strlen(cfg_file_path)<=0)
		return;
	FILE *fp=fopen(cfg_file_path,"wt");
	if(fp==NULL)
		return;
	if(m_sPrjName.GetLength()>0)
		fprintf(fp,"PROJECT_NAME=%s\n",(char*)m_sPrjName);
	if(m_sPrjCode.GetLength()>0)
		fprintf(fp,"PROJECT_CODE=%s\n",(char*)m_sPrjCode);
	if(m_sTaType.GetLength()>0)
		fprintf(fp,"TOWER_NAME=%s\n",(char*)m_sTaType);
	if(m_sTaAlias.GetLength()>0)
		fprintf(fp,"TOWER_CODE=%s\n",(char*)m_sTaAlias);
	if(m_sTaStampNo.GetLength()>0)
		fprintf(fp,"STAMP_NO=%s\n",(char*)m_sTaStampNo);
	fclose(fp);
}

//////////////////////////////////////////////////////////////////////////
// CSortedModel
#include "SortFunc.h"
#include "ComparePartNoString.h"

typedef CPlateProcessInfo* CPlateInfoPtr;
int ComparePlatePtrByPartNo(const CPlateInfoPtr &plate1,const CPlateInfoPtr &plate2)
{
	CXhChar16 sPartNo1=plate1->xPlate.GetPartNo();
	CXhChar16 sPartNo2=plate2->xPlate.GetPartNo();
	return ComparePartNoString(sPartNo1,sPartNo2,"SHGPT");
}
CSortedModel::CSortedModel(CPNCModel *pModel)
{
	if(pModel==NULL)
		return;
	platePtrList.Empty();
	for(CPlateProcessInfo *pPlate=pModel->EnumFirstPlate(FALSE);pPlate;pPlate=pModel->EnumNextPlate(FALSE))
		platePtrList.append(pPlate);
	CHeapSort<CPlateInfoPtr>::HeapSort(platePtrList.m_pData,platePtrList.Size(),ComparePlatePtrByPartNo);
}
CPlateProcessInfo *CSortedModel::EnumFirstPlate()
{
	CPlateProcessInfo **ppPlate=platePtrList.GetFirst();
	if(ppPlate)
		return *ppPlate;
	else
		return NULL;
}
CPlateProcessInfo *CSortedModel::EnumNextPlate()
{
	CPlateProcessInfo **ppPlate=platePtrList.GetNext();
	if(ppPlate)
		return *ppPlate;
	else
		return NULL;
}
