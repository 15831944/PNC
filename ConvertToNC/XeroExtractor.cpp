#include "stdafx.h"
#include "f_alg_fun.h"
#include "ArrayList.h"
#include "XeroExtractor.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define  LEN_SCALE		0.8
#define  MIN_DISTANCE	25

CStraightLine::CStraightLine(const double* _start,const double* _end)
{
	ciCurveType=STRAIGHT;
	start=_start;
	end=_end;
}
bool CSymbolRecoginzer::IsIntersWith(ICurveLine* pCurveLine,DWORD cbFilterFlag/*=0*/)
{	//ͨ����Ƿ���ʶ�������
	f3dPoint inters;
	for(CSymbolEntity* pSymbol=listSymbols.EnumObjectFirst();pSymbol;pSymbol=listSymbols.EnumObjectNext())
	{
		if(pSymbol->ciSymbolType!=cbFilterFlag)
			continue;
		int nInters=0;
		for(CSymbolEntity::LINE* pSonLine=pSymbol->listSonlines.EnumObjectFirst();pSonLine;pSonLine=pSymbol->listSonlines.EnumObjectNext())
		{
			if(Int3dll(f3dLine(pCurveLine->start,pCurveLine->end),f3dLine(pSonLine->start,pSonLine->end),inters)==1)
				nInters++;
		}
		if(nInters>=3)
			return true;
	}
	return false;
}
bool CSymbolRecoginzer::IsWeldingAlongSide(ICurveLine* pCurveLine)
{	//������ʶ��
	return false;
}
//////////////////////////////////////////////////////////////////////////
//CPlateObject
CPlateObject::CPlateObject()
{

}
CPlateObject::~CPlateObject()
{
	vertexList.Empty();
}
void CPlateObject::CreateRgn()
{
	ARRAY_LIST<f3dPoint> vertices;
	for(VERTEX* pVer=vertexList.GetFirst();pVer;pVer=vertexList.GetNext())
		vertices.append(pVer->pos);
	if(vertices.GetSize()>0)
		region.CreatePolygonRgn(vertices.m_pData,vertices.GetSize());
}
//�ж�������Ƿ��ڸְ���
bool CPlateObject::IsInPlate(const double* poscoord)
{
	if(region.GetVertexCount()<3)
		CreateRgn();
	if(region.GetAxisZ().IsZero())
		return false;
	//return region.PtInRgn(poscoord)==1;
	int iRet=region.PtInRgn2(poscoord);
	if(iRet==1||iRet==2)
		return true;
	return false;
}
//�ж�ֱ���Ƿ��ڸְ���
bool CPlateObject::IsInPlate(const double* start,const double* end)
{
	if(region.GetVertexCount()<3)
		CreateRgn();
	if(region.GetAxisZ().IsZero())
		return false;
	//int iRet=region.LineInRgn(start,end);
	int iRet=region.LineInRgn2(start,end);
	if(iRet==1)
		return true;
	else if(iRet==2)
	{	//�����ڶ����������
		f3dPoint pt,inters1,inters2;
		for(int i=0;i<region.GetVertexCount();i++)
		{
			GEPOINT prePt=region.GetVertexAt(i);
			GEPOINT curPt=region.GetVertexAt((i+1)%region.GetVertexCount());
			if(Int3dll(prePt,curPt,start,end,pt)>0)
			{
				if(inters1.IsZero())
					inters1=pt;
				else
					inters2=pt;
			}
		}
		double fExternLen=0,fSumLen=DISTANCE(GEPOINT(start),GEPOINT(end));
		if(!inters2.IsZero())
			fExternLen=DISTANCE(inters1,inters2);
		else if(IsInPlate(start))
			fExternLen=DISTANCE(GEPOINT(start),inters1);
		else if(IsInPlate(end))
			fExternLen=DISTANCE(GEPOINT(end),inters1);
		if(ftoi(fExternLen)<=ftoi(fSumLen) && fExternLen/fSumLen>LEN_SCALE)
			return true;
		else
			return false;
	}
	else
		return false;
}
//�ж���ȡ�ɹ����������Ƿ���ʱ������
BOOL CPlateObject::IsValidVertexs()
{
	int n=vertexList.GetNodeNum();
	if(n<3)
		return FALSE;
	int i=0;
	double wrap_area=0;
	DYN_ARRAY<GEPOINT> pnt_arr(n);
	for(VERTEX* pVertex=vertexList.GetFirst();pVertex;pVertex=vertexList.GetNext(),i++)
		pnt_arr[i]=pVertex->pos;
	for(i=1;i<n-1;i++)
	{
		double result=DistOf2dPtLine(pnt_arr[i+1],pnt_arr[0],pnt_arr[i]);
		if(result>0)		// ���������࣬�����������
			wrap_area+=CalTriArea(pnt_arr[0].x,pnt_arr[0].y,pnt_arr[i].x,pnt_arr[i].y,pnt_arr[i+1].x,pnt_arr[i+1].y);
		else if(result<0)	// ��������Ҳ࣬�����������
			wrap_area-=CalTriArea(pnt_arr[0].x,pnt_arr[0].y,pnt_arr[i].x,pnt_arr[i].y,pnt_arr[i+1].x,pnt_arr[i+1].y);
	}
	if(wrap_area>0)
		return TRUE;
	else
		return FALSE;
}
void CPlateObject::ReverseVertexs()
{
	int n=vertexList.GetNodeNum();
	ARRAY_LIST<VERTEX> vertexArr;
	vertexArr.SetSize(n);
	VERTEX* pVertex=NULL;
	int i=0;
	for(pVertex=vertexList.GetTail();pVertex;pVertex=vertexList.GetPrev())
	{
		vertexArr[i]=*pVertex;
		i++;
	}
	//
	vertexList.Empty();
	for(i=0;i<n;i++)
	{
		pVertex=vertexList.append();
		*pVertex=vertexArr[i];
	}
}
void CPlateObject::DeleteAssisstPts()
{	//ȥ�������Ͷ���
	for(VERTEX* pVer=vertexList.GetFirst();pVer;pVer=vertexList.GetNext())
	{
		if(pVer->tag.lParam==-1)
			vertexList.DeleteCursor();
	}
	vertexList.Clean();
}
void CPlateObject::UpdateVertexPropByArc(f3dArcLine& arcLine,int type)
{
	BOOL bFind=FALSE;
	int i=0,iStart=-1,iEnd=-1;
	VERTEX* pVer=NULL;
	//����Բ�����¶�����Ϣ
	for(pVer=vertexList.GetFirst();pVer;pVer=vertexList.GetNext())
	{
		if(pVer->pos.IsEqual(arcLine.Start(),0.5))
		{
			iStart=i;
			pVer->pos=arcLine.Start();
		}
		if(pVer->pos.IsEqual(arcLine.End(),0.5))
		{
			iEnd=i;
			pVer->pos=arcLine.End();
		}
		if(iStart>-1&&iEnd>-1)
		{
			bFind=TRUE;
			break;
		}
		i++;		
	}
	if(bFind==FALSE)
		return;
	int tag=1,nNum=vertexList.GetNodeNum();
	if((iStart>iEnd||(iStart==0&&iEnd==nNum-1)) && (iStart+1)%nNum!=iEnd)
	{	//
		tag*=-1;
		int index=iEnd;
		iEnd=iStart;
		iStart=index;
	}
	pVer=vertexList.GetByIndex(iStart);
	if(pVer)
	{
		pVer->ciEdgeType=type;
		pVer->arc.fSectAngle=arcLine.SectorAngle();
		pVer->arc.radius=arcLine.Radius();
		pVer->arc.center=arcLine.Center();
		pVer->arc.work_norm=arcLine.WorkNorm()*tag;
		pVer->arc.column_norm=arcLine.ColumnNorm();
	}
	//ɾ������Ҫ�Ķ���
	if((iStart+1)%nNum==iEnd || (iEnd+1)%nNum==iStart)
		return;
	for(int i=iStart+1;i<iEnd;i++)
		vertexList.DeleteAt(i);
	vertexList.Clean();
}
BOOL CPlateObject::RecogWeldLine()
{
	return FALSE;
}
BOOL CPlateObject::RecogWeldLine(f3dLine slop_line)
{
	f3dPoint slop_vec=slop_line.endPt-slop_line.startPt;
	normalize(slop_vec);
	VERTEX* pPreVer=vertexList.GetTail();
	for(VERTEX* pCurVer=vertexList.GetFirst();pCurVer;pCurVer=vertexList.GetNext())
	{
		f3dPoint vec=pCurVer->pos-pPreVer->pos;
		normalize(vec);
		if(fabs(vec*slop_vec)<eps_cos)
		{
			pPreVer=pCurVer;
			continue;
		}
		double fDist=0;
		f3dPoint inters,midPt=(pCurVer->pos+pPreVer->pos)*0.5;
		SnapPerp(&inters,slop_line,midPt,&fDist);
		if(fDist<MIN_DISTANCE && slop_line.PtInLine(inters)==2)
			break;
		pPreVer=pCurVer;
	}
	if(pPreVer)
	{
		pPreVer->m_bWeldEdge=true;
		return TRUE;
	}
	return FALSE;
}
//////////////////////////////////////////////////////////////////////////
//CPlateExtractor
CPlateExtractor::CPlateExtractor()
{
	Init();
}
void CPlateExtractor::Init()
{	//���ű�ע����
	m_iDimStyle=0;
	m_sPnKey.Copy("#");
	m_sThickKey.Copy("-");
	m_sMatKey.Copy("Q");
	//��˨ֱ������
	hashBoltDList.SetValue("M24",BOLT_BLOCK("M24",24));
	hashBoltDList.SetValue("M20",BOLT_BLOCK("M20",20));
	hashBoltDList.SetValue("M16",BOLT_BLOCK("M16",16));
	hashBoltDList.SetValue("M12",BOLT_BLOCK("M12",12));
	hashBoltDList.SetValue("���25.5",BOLT_BLOCK("���25.5",24));
	hashBoltDList.SetValue("���21.5",BOLT_BLOCK("���21.5",20));
	hashBoltDList.SetValue("���17.5",BOLT_BLOCK("���17.5",16));
	hashBoltDList.SetValue("���13.5",BOLT_BLOCK("���13.5",12));
	hashBoltDList.SetValue("���Ĭ��",BOLT_BLOCK("���Ĭ��",0));
	//
	m_sBendLineLayer="8";
	m_sSlopeLineLayer="2";
}
CPlateExtractor::~CPlateExtractor()
{
	hashBoltDList.Empty();
}
int CPlateExtractor::GetKeyMemberNum()
{
	int nNum=0;
	if(m_sPnKey.GetLength()>0)
		nNum++;
	if(m_sThickKey.GetLength()>0)
		nNum++;
	if(m_sMatKey.GetLength()>0)
		nNum++;
	if(m_sPnNumKey.GetLength()>0)
		nNum++;
	return nNum;
}
char CPlateExtractor::QueryBriefMatMark(const char* sMatMark)
{
	char cMat='S';
	if(strstr(sMatMark,"Q345"))
		cMat='H';
	else if(strstr(sMatMark,"Q390"))
		cMat='G';
	else if(strstr(sMatMark,"Q420"))
		cMat='P';
	else if(strstr(sMatMark,"Q460"))
		cMat='T';
	return cMat;
}
int CPlateExtractor::GetPnKeyNum(const char* dim_str)
{
	if(strlen(dim_str)<=0)
		return 0;
	if(strstr(dim_str,m_sPnKey)==NULL)
		return 0;
	CXhChar100 sValue(dim_str);
	sValue.Replace(" ","");	//ȡ���ո�
	int n=0;
	if(m_sPnKey.GetLength()>1)
	{
		for(int i=0;i<(int)strlen(dim_str);i++)
		{
			if(dim_str[i]==m_sPnKey[0])
				n++;
		}
	}
	else
	{
		CString ss(sValue);
		if(ss.Find(m_sPnKey)>=0)
			n=1;
	}
	return n;
}
BOOL CPlateExtractor::IsMatchPNRule(const char* sText)
{
	if(strlen(sText)<=0 || strlen(m_sPnKey)<=0)
		return FALSE;
	if(strstr(sText,m_sPnKey)==NULL)
		return FALSE;
	if(m_iDimStyle==1&&GetPnKeyNum(sText)!=1)	//���б�ע����������ֻ����һ��#�ؼ���
		return FALSE;
	if(m_iDimStyle==0)	//���б�ע��������������
	{
		if(strlen(m_sThickKey)>0 && strstr(sText,m_sThickKey)==NULL)
			return FALSE;
		if(strlen(m_sMatKey)>0 && strstr(sText,m_sMatKey)==NULL)
			return FALSE;
		if(strlen(m_sPnNumKey)>0 && strstr(sText,m_sPnNumKey)==NULL)
			return FALSE;
	}
	else	//���б�ע��������
	{	//���б�ע�и����ض����ֽ����ų�(�������ڴ˲�����)
		//if(strstr(sText,"��")||strstr(sText,"��")||strstr(sText,"��"))
			//return FALSE;
	}
	return TRUE;
}
BOOL CPlateExtractor::IsMatchThickRule(const char* sText)
{
	if(strlen(sText)<=0 || strlen(m_sThickKey)<=0)
		return FALSE;
	if(m_iDimStyle==0)
		return IsMatchPNRule(sText);
	if(strstr(sText,m_sThickKey))
		return TRUE;
	else
		return FALSE;
}
BOOL CPlateExtractor::IsMatchMatRule(const char* sText)
{
	if(strlen(sText)<=0 || strlen(m_sMatKey)<=0)
		return FALSE;
	if(m_iDimStyle==0)
		return IsMatchPNRule(sText);
	if(strstr(sText,m_sMatKey)&&(strstr(sText,"Q3")||strstr(sText,"Q4")))
		return TRUE;
	else
		return FALSE;
}
BOOL CPlateExtractor::IsMatchNumRule(const char* sText)
{
	if(strlen(sText)<=0 || strlen(m_sPnNumKey)<=0)
		return FALSE;
	if(m_iDimStyle==0)
		return IsMatchPNRule(sText);
	else if(strstr(sText,m_sPnNumKey))
		return TRUE;
	else
		return FALSE;
}
BOOL CPlateExtractor::IsMatchBendRule(const char* sText)
{
	if(strlen(sText)<=0||(strlen(m_sFrontBendKey)<=0&&strlen(m_sReverseBendKey)<=0))
		return FALSE;
	else if(strstr(sText,m_sFrontBendKey)||strstr(sText,m_sReverseBendKey))
		return TRUE;
	else
		return FALSE;
}
BYTE CPlateExtractor::ParsePartNoText(const char* sText,CXhChar16& sPartNo)
{
	CXhChar100 str,sValue(sText);
	sValue.Replace("��"," ");
	if(strstr(m_sPnKey,"#"))	//#��ʶ���ڼ��ź���
		sValue.Replace(m_sPnKey,"| "); 
	else
		sValue.Replace(m_sPnKey,"|"); 
	//for(char* sKey=strtok(sValue," \t\\P");sKey;sKey=strtok(NULL," \t\\P"))
	CXhChar100 sPrevStr;
	BOOL bHasWeldFlag = FALSE;
	for(char* sKey=strtok(sValue," \t");sKey;sKey=strtok(NULL," \t"))
	{
		if (strstr(sKey, "|"))
		{	//��ȡ����
			str.Copy(sKey);
			str.Replace("|", "");
			str.Remove(' ');
			//���ݼ����д��ո����������磺101 H,��ȡΪ101H�� wht 19-07-22
			if (str.Length == 1 && sPrevStr.GetLength() > 0)
				sPartNo.Printf("%s%s", (char*)sPrevStr, (char*)str);
			else
				sPartNo.Copy(str);
			if (sPartNo.GetLength() > 0)
			{
				if(bHasWeldFlag)
					return PART_LABEL_WELD;
				else
					return PART_LABEL_VALID;
			}
			else
				return PART_LABEL_EMPTY;
		}
		else
		{
			sPrevStr.Copy(sKey);
			if (strstr(sKey, "��"))
				bHasWeldFlag = TRUE;
		}
	}
	return PART_LABEL_EMPTY;
}
void CPlateExtractor::ParseThickText(const char* sText,int& nThick)
{
	CXhChar100 str,sValue(sText);
	sValue.Replace("��"," ");
	if(m_iDimStyle==0)
		sValue.Replace(m_sPnKey,"| ");
	//for(char* sKey=strtok(sValue," \t\\P");sKey;sKey=strtok(NULL," \t\\P"))
	for(char* sKey=strtok(sValue," \t");sKey;sKey=strtok(NULL," \t"))
	{
		if(strstr(sKey,m_sThickKey)==NULL||strstr(sKey,"|"))
			continue;
		//�鿴��ȱ�ʶ����λ��
		UINT i,index=0;
		for(i=0;i<strlen(sKey);i++)
		{
			if(sKey[i]==m_sThickKey[0])
			{
				index=i;
				break;
			}
		}
		if(index>0 && sKey[index-1]!=' ')
		{	//��ȱ�ʶ�����ַ����м�
			if(strstr(sKey,"Q")==NULL)
				continue;	//
			CXhChar50 sGroupStr(sKey),sMat,sThick;
			sGroupStr.Replace(m_sThickKey," ");
			sscanf(sGroupStr,"%s%s",(char*)sMat,(char*)sThick);
			sprintf(sKey,"%s%s",(char*)m_sThickKey,(char*)sThick);
		}
		//�����ַ���
		str.Copy(sKey);
		if(strstr(str,"mm"))
			str.Replace("mm","");
		str.Replace(m_sThickKey,"| ");
		sscanf(str,"%s%d",(char*)sValue,&nThick);
	}
}
void CPlateExtractor::ParseMatText(const char* sText,char& cMat)
{
	CXhChar100 sValue(sText);
	sValue.Replace("��"," ");
	if(m_iDimStyle==0)
		sValue.Replace(m_sPnKey,"| ");
	CString str(sValue);
	str.Replace(m_sMatKey," Q");
	sValue.Copy(str);
	//for(char* sKey=strtok(sValue," \t\\P");sKey;sKey=strtok(NULL," \t\\P"))
	for(char* sKey=strtok(sValue," \t");sKey;sKey=strtok(NULL," \t"))
	{
		if(strstr(sKey,"Q")&&strstr(sKey,"|")==NULL)
		{
			cMat=QueryBriefMatMark(sKey);
			return;
		}
	}
}
void CPlateExtractor::ParseNumText(const char* sText,int& nNum)
{
	CXhChar100 str,sValue(sText);
	sValue.Replace("��"," ");
	if(m_iDimStyle==0)
		sValue.Replace(m_sPnKey,"| ");
	for(char* sKey=strtok(sValue," \t");sKey;sKey=strtok(NULL," \t"))
	{
		if(strstr(sKey,m_sPnNumKey))
		{
			str.Copy(sKey);
			str.Replace(m_sPnNumKey,"");
			nNum=atoi(str);
			return;
		}
	}
}
void CPlateExtractor::ParseBendText(const char* sText,double &degree,BOOL &bFrontBend)
{
	CXhChar100 sValue(sText);
	sValue.Replace("��"," ");
	if(strstr(sText,m_sFrontBendKey))
		bFrontBend=TRUE;
	else 
		bFrontBend=FALSE;
	CString str(sValue);
	str.Replace(m_sFrontBendKey,"");
	str.Replace(m_sReverseBendKey,"");
	str.Replace("��","");
	str.Replace("��","");
	if(str.GetLength()>0)
		degree=atof(str);
	else
		degree=0;
}
BOOL CPlateExtractor::IsBendLine(AcDbLine* pAcDbLine,ISymbolRecognizer* pRecognizer/*=NULL*/)
{
	BOOL bRet=FALSE;
	if(pRecognizer!=NULL)
	{	//����ֱ����SPLINE�ֶ����󽻽����ж�
		AcGePoint3d startPt,endPt;
		pAcDbLine->getStartPoint(startPt);
		pAcDbLine->getEndPoint(endPt);
		CStraightLine line(&startPt.x,&endPt.x);
		bRet=pRecognizer->IsIntersWith(&line,0x01);
	}
	if(!bRet)
	{	//TODO:����Ӧ����ͨ��ͼ���������ʶ������� wjh-2017.6.3
		/*CXhChar50 sLayerName;
#ifdef _ARX_2007
		sLayerName.Copy((char*)_bstr_t(pAcDbLine->layer()));
#else
		sLayerName.Copy(pAcDbLine->layer());
#endif
		if(sLayerName.Equal(m_sBendLineLayer))
			bRet=TRUE;*/
	}
	return bRet;
}
BOOL CPlateExtractor::IsSlopeLine(AcDbLine* pAcDbLine,ISymbolRecognizer* pRecognizer/*=NULL*/)
{
	BOOL bRet=FALSE;
	if(pRecognizer!=NULL)
	{

	}
	if(!bRet)
	{	//ͨ��ͼ��ʶ���¿���
		CXhChar50 sLayerName;
#ifdef _ARX_2007
		sLayerName.Copy((char*)_bstr_t(pAcDbLine->layer()));
#else
		sLayerName.Copy(pAcDbLine->layer());
#endif
		if(sLayerName.Equal(m_sSlopeLineLayer))
			bRet=TRUE;
	}
	return bRet;
}

BOOL CPlateExtractor::RecogBoltHole(AcDbEntity* pEnt,BOLT_HOLE& hole)
{
	if(pEnt==NULL)
		return FALSE;
	if(pEnt->isKindOf(AcDbBlockReference::desc()))
	{
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
		if(sName.Length<=0)
			return FALSE;
		BOLT_BLOCK* pBoltD=hashBoltDList.GetValue(sName);
		if(pBoltD==NULL)
			return FALSE;
		hole.posX=(float)pReference->position().x;
		hole.posY=(float)pReference->position().y;
		if(pBoltD->diameter>0)
		{
			hole.d=(BYTE)pBoltD->diameter;
			hole.increment=1.5;
		}
		else if(pBoltD->hole_d>0)
		{	
			int nHoleD=(int)floor(pBoltD->hole_d);
			double fHoleFloat=pBoltD->hole_d-nHoleD;
			if(fabs(fHoleFloat)<EPS)
			{	//�׾�Ϊ����
				hole.d=(BYTE)pBoltD->hole_d;
				hole.increment=0;
			}
			else 
			{	//�׾�Ϊ������
				hole.d=(BYTE)(nHoleD-1);
				hole.increment=(float)(1+fHoleFloat);
			}
		}
		hole.ciSymbolType=0;
		return TRUE;
	}
	else if(pEnt->isKindOf(AcDbCircle::desc()))
	{
		AcDbCircle* pCircle=(AcDbCircle*)pEnt;
		if(int(pCircle->radius())<=0)	//ȥ����
			return FALSE;
		double fDiameter=pCircle->radius()*2;
		hole.posX=(float)pCircle->center().x;
		hole.posY=(float)pCircle->center().y;
		//�����ֱ��ֱ������ֱ��������׾�����ֵ��������PPE��ͳһ����׾�����ֵʱ�ᶪʧ�˴���ȡ�Ŀ׾�����ֵ wht 19-09-12
		hole.d = fDiameter;
		hole.increment = 0;
		/*
		int bolt_d=(int)pCircle->radius()*2;
		hole.d=bolt_d;
		if(fabs(fDiameter-bolt_d)>EPS2)
			hole.increment=(float)fDiameter-bolt_d;
		else
			hole.increment=0;*/
		hole.ciSymbolType=1;	//Ĭ�Ϲ��߿�
		return TRUE;
	}
	return FALSE;
}
BOOL CPlateExtractor::RecogBasicInfo(AcDbEntity* pEnt,BASIC_INFO& basicInfo)
{
	if(pEnt==NULL)
		return FALSE;
	if(!pEnt->isKindOf(AcDbText::desc())&&!pEnt->isKindOf(AcDbMText::desc()))
		return FALSE;
	CXhChar500 sText;
	ATOM_LIST<CXhChar200> lineList;
	if(pEnt->isKindOf(AcDbText::desc()))
	{
		AcDbText* pText=(AcDbText*)pEnt;
#ifdef _ARX_2007
		sText.Copy(_bstr_t(pText->textString()));
#else
		sText.Copy(pText->textString());
#endif
	}
	else if(pEnt->isKindOf(AcDbMText::desc()))
	{
		AcDbMText* pMText=(AcDbMText*)pEnt;
#ifdef _ARX_2007
		sText.Copy(_bstr_t(pMText->contents()));
#else
		sText.Copy(pMText->contents());
#endif
		lineList.Empty();
		for (char* sKey = strtok(sText, "\\P"); sKey; sKey = strtok(NULL, "\\P"))
		{
			CXhChar200 sTemp(sKey);
			sTemp.Replace("\\P", "");
			lineList.append(sTemp);
		}
	}
	else if(pEnt->isKindOf(AcDbAttribute::desc()))
	{
		AcDbAttribute* pText=(AcDbAttribute*)pEnt;
#ifdef _ARX_2007
		sText.Copy(_bstr_t(pText->textString()));
#else
		sText.Copy(pText->textString());
#endif
	}
	BOOL bRet = FALSE;
	if (lineList.GetNodeNum() > 0)
	{
		for (CXhChar200 *pLineText = lineList.GetFirst(); pLineText; pLineText = lineList.GetNext())
		{
			CXhChar200 sTemp(*pLineText);
			if (IsMatchThickRule(sTemp))
			{
				ParseThickText(sTemp, basicInfo.m_nThick);
				bRet = TRUE;
			}
			if (IsMatchMatRule(sTemp))
			{
				ParseMatText(sTemp, basicInfo.m_cMat);
				bRet = TRUE;
			}
			if (IsMatchNumRule(sTemp))
			{
				ParseNumText(sTemp, basicInfo.m_nNum);
				//��¼����������Ӧ��ʵ��Id wht 19-08-05
				basicInfo.m_idCadEntNum = pEnt->id().asOldId();
				bRet = TRUE;
			}
			if (IsMatchPNRule(sTemp))
			{
				ParsePartNoText(sTemp, basicInfo.m_sPartNo);
				bRet = TRUE;
			}
		}
	}
	else
	{
		if (IsMatchThickRule(sText))
		{
			ParseThickText(sText, basicInfo.m_nThick);
			bRet = TRUE;
		}
		if (IsMatchMatRule(sText))
		{
			ParseMatText(sText, basicInfo.m_cMat);
			bRet = TRUE;
		}
		if (IsMatchNumRule(sText))
		{
			ParseNumText(sText, basicInfo.m_nNum);
			//��¼����������Ӧ��ʵ��Id wht 19-08-05
			basicInfo.m_idCadEntNum = pEnt->id().asOldId();
			bRet = TRUE;
		}
		if (IsMatchPNRule(sText))
		{
			ParsePartNoText(sText, basicInfo.m_sPartNo);
			bRet = TRUE;
		}
	}
	return bRet;
}
BOOL CPlateExtractor::RecogArcEdge(AcDbEntity* pEnt,f3dArcLine& arcLine,BYTE& ciEdgeType)
{
	if(pEnt==NULL)
		return FALSE;
	if(pEnt->isKindOf(AcDbArc::desc()))
	{
		AcDbArc* pArc=(AcDbArc*)pEnt;
		AcGePoint3d pt;
		f3dPoint startPt,endPt,center,norm;
		pArc->getStartPoint(pt);
		Cpy_Pnt(startPt,pt);
		pArc->getEndPoint(pt);
		Cpy_Pnt(endPt,pt);
		Cpy_Pnt(center,pArc->center());
		Cpy_Pnt(norm,pArc->normal());
		double radius=pArc->radius();
		double angle=(pArc->endAngle()-pArc->startAngle());
		if(radius>0 && fabs(angle)>0)
		{	//���˴����Բ��(���磺��ʱpEnt��һ����,����������ʾΪԲ��)
			ciEdgeType=2;
			return arcLine.CreateMethod3(startPt,endPt,norm,radius,center);
		}
	}
	else if(pEnt->isKindOf(AcDbEllipse::desc()))
	{	//���¸ְ嶥�����(��Բ)
		AcDbEllipse* pEllipse=(AcDbEllipse*)pEnt;
		AcGePoint3d pt;
		AcGeVector3d minorAxis;
		f3dPoint startPt,endPt,center,min_vec,maj_vec,column_norm,work_norm;
		pEllipse->getStartPoint(pt);
		Cpy_Pnt(startPt,pt);
		pEllipse->getEndPoint(pt);
		Cpy_Pnt(endPt,pt);
		Cpy_Pnt(center,pEllipse->center());
		Cpy_Pnt(min_vec,pEllipse->minorAxis());
		Cpy_Pnt(maj_vec,pEllipse->majorAxis());
		Cpy_Pnt(work_norm,pEllipse->normal());
		double min_R=min_vec.mod();
		double maj_R=maj_vec.mod();
		double cosa=min_R/maj_R;
		double sina=SQRT(1-cosa*cosa);
		column_norm=work_norm;
		RotateVectorAroundVector(column_norm,sina,cosa,min_vec);
		//
		ciEdgeType=3;
		return arcLine.CreateEllipse(center,startPt,endPt,column_norm,work_norm,min_R);
	}
	return FALSE;
}
