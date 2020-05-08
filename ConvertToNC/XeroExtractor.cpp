#include "stdafx.h"
#include "f_alg_fun.h"
#include "ArrayList.h"
#include "XeroExtractor.h"
#include "CadToolFunc.h"
#include "LogFile.h"
#include "DefCard.h"
#include "ProcessPart.h"
#include <vector>
#include "PNCSysPara.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define  LEN_SCALE		0.8
#define  MIN_DISTANCE	25
//////////////////////////////////////////////////////////////////////////
//
void CSymbolRecoginzer::AppendSymbolEnt(AcDbSpline* pSpline)
{
	if (pSpline == NULL)
		return;
	SYMBOL_ENTITY* pSymbolEnt = listSymbols.AttachObject();
	pSymbolEnt->ciSymbolType = 1;	//������S�ͷ���
	for (int i = 0; i < pSpline->numFitPoints() - 1; i++)
	{
		AcGePoint3d ptS, ptE;
		pSpline->getFitPointAt(i, ptS);
		pSpline->getFitPointAt(i + 1, ptE);
		//����߶�
		GELINE* pLine = pSymbolEnt->listSonlines.AttachObject();
		Cpy_Pnt(pLine->start, ptS);
		Cpy_Pnt(pLine->end, ptE);
	}
}
bool CSymbolRecoginzer::IsHuoquLine(GELINE* pCurveLine,DWORD cbFilterFlag/*=0*/)
{	//ͨ����Ƿ���ʶ�������
	f3dPoint inters;
	for(SYMBOL_ENTITY* pSymbol=listSymbols.EnumObjectFirst();pSymbol;pSymbol=listSymbols.EnumObjectNext())
	{
		if(pSymbol->ciSymbolType!=cbFilterFlag)
			continue;
		int nInters=0;
		for(GELINE* pSonLine=pSymbol->listSonlines.EnumObjectFirst();pSonLine;pSonLine=pSymbol->listSonlines.EnumObjectNext())
		{
			if(Int3dll(f3dLine(pCurveLine->start,pCurveLine->end),f3dLine(pSonLine->start,pSonLine->end),inters)==1)
				nInters++;
		}
		if(nInters>=3)
			return true;
	}
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
BOOL CPlateObject::RecogWeldLine(const double* ptS, const double* ptE)
{
	return RecogWeldLine(f3dLine(ptS, ptE));
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
	hashBoltDList.SetValue("M24",BOLT_BLOCK("TMA","M24",24));
	hashBoltDList.SetValue("M20",BOLT_BLOCK("TMA", "M20",20));
	hashBoltDList.SetValue("M16",BOLT_BLOCK("TMA", "M16",16));
	hashBoltDList.SetValue("M12",BOLT_BLOCK("TMA", "M12",12));
	hashBoltDList.SetValue("���25.5",BOLT_BLOCK("TW", "���25.5",24));
	hashBoltDList.SetValue("���21.5",BOLT_BLOCK("TW", "���21.5",20));
	hashBoltDList.SetValue("���19.5",BOLT_BLOCK("TW", "���19.5",18));
	hashBoltDList.SetValue("���17.5",BOLT_BLOCK("TW", "���17.5",16));
	hashBoltDList.SetValue("���13.5",BOLT_BLOCK("TW", "���13.5",12));
	hashBoltDList.SetValue("���Ĭ��",BOLT_BLOCK("TW", "���Ĭ��",0));
	//
	m_sBendLineLayer="8";
	m_sSlopeLineLayer.Empty();
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
int CPlateExtractor::GetPnKeyNum(const char* dim_str)
{
	if(strlen(dim_str)<=0)
		return 0;
	if(strstr(dim_str,m_sPnKey)==NULL)
		return 0;
	CXhChar100 sValue(dim_str);
	sValue.Replace(" ","");	//ȡ���ո�
	int n=0;
	if(m_sPnKey.GetLength()==1)
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
		if (strlen(m_sPnNumKey) > 0)
		{
			if (strstr(m_sPnNumKey, "|"))
			{	//�����ؼ���֧�ֶ������"��|��"
				CXhChar50 sNumKey = m_sPnNumKey;
				char* sKey = NULL;
				for (sKey = strtok(sNumKey, "|"); sKey; sKey = strtok(NULL, "|"))
				{
					if(strstr(sText, sKey))
						break;
				}
				if (sKey == NULL)
					return FALSE;
			}
			else if(strstr(sText, m_sPnNumKey) == NULL)
				return FALSE;
		}
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
	if(strstr(sText,m_sMatKey)&&(strstr(sText, "Q2")||strstr(sText,"Q3")||strstr(sText,"Q4")))
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
	CXhChar50 sNumKey = m_sPnNumKey;
	for (char* sKey = strtok(sNumKey, "|"); sKey; sKey = strtok(NULL, "|"))
	{
		if (strstr(sText, sKey))
			return TRUE;
	}
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
BOOL CPlateExtractor::IsBriefMatMark(char cMat)
{
	if ('H' == cMat || 'h' == cMat || 'G' == cMat ||
		'P' == cMat || 'T' == cMat || 'S' == cMat)
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
			//���ݼ����д��ո����������磺101 H,��ȡΪ101H �� H 101,��ȡΪH101�� wht 19-07-22
			if (str.GetLength() == 1 && IsBriefMatMark(str[0]) && sPrevStr.GetLength() > 0)
				sPartNo.Printf("%s%s", (char*)sPrevStr, (char*)str);
			else if (sPrevStr.GetLength() == 1 && IsBriefMatMark(str[0]) && str.GetLength() > 0)
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
		int nValue = 0;
		sscanf(str,"%s%d",(char*)sValue,&nValue);
		if (nValue > 0)
			nThick = nValue;
	}
}
void CPlateExtractor::ParseMatText(const char* sText,char& cMat,char& cQuality)
{
	CXhChar100 sValue(sText);
	sValue.Replace("��"," ");
	if(m_iDimStyle==0)
		sValue.Replace(m_sPnKey,"| ");
	CString str(sValue);
	if (strstr(str, "Q") == NULL)	//���ʱ�ʶ��û��Qʱ����Ҫ���ؼ����滻ΪQ wht 19-10-22
		str.Replace(m_sMatKey, " Q");
	sValue.Copy(str);
	//for(char* sKey=strtok(sValue," \t\\P");sKey;sKey=strtok(NULL," \t\\P"))
	for(char* sKey=strtok(sValue," \t");sKey;sKey=strtok(NULL," \t"))
	{
		if (cMat == 0 && strstr(sKey, "Q") && strstr(sKey, "|") == NULL)
		{
			char cMark = CProcessPart::QueryBriefMatMark(sKey);
			cQuality = CProcessPart::QueryBriefQuality(sKey);
			cMat = (cMark == 'A') ? 0 : cMark;
		}
	}
}
void CPlateExtractor::ParseNumText(const char* sText,int& nNum)
{
	//��ȡ���������ɸ��ؼ���
	CXhChar50 sNumKey = m_sPnNumKey;
	std::vector<CXhChar16> numKeyArr;
	for (char* sKey = strtok(sNumKey, "|"); sKey; sKey = strtok(NULL, "|"))
		numKeyArr.push_back(CXhChar16(sKey));
	//�����ַ�������ȡ����ֵ
	CXhChar100 str,sValue(sText);
	sValue.Replace("��"," ");
	if(m_iDimStyle==0)
		sValue.Replace(m_sPnKey,"| ");
	for(char* sSubStr=strtok(sValue," \t"); sSubStr; sSubStr =strtok(NULL," \t"))
	{
		for (size_t i = 0; i < numKeyArr.size(); i++)
		{
			if (strstr(sSubStr, numKeyArr[i]))
			{
				str.Copy(sSubStr);
				str.Replace(numKeyArr[i], "");
				nNum = atoi(str);
				return;
			}
		}
	}
}
void CPlateExtractor::ParseBendText(const char* sText,double &degree,BOOL &bFrontBend)
{
	if (strstr(sText, m_sFrontBendKey))
		bFrontBend = TRUE;
	else
		bFrontBend = FALSE;
	CXhChar100 sValue(sText);
	sValue.Replace("��"," ");
	sValue.Replace(m_sFrontBendKey,"");
	sValue.Replace(m_sReverseBendKey,"");
	sValue.Replace("���", "");
	sValue.Replace("��"," ");
	sValue.Replace("��"," ");
	char* sKey = strtok(sValue, " ");
	if(strlen(sKey)>0)
		degree=atof(sKey);
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
		GELINE line;
		Cpy_Pnt(line.start, startPt);
		Cpy_Pnt(line.end, endPt);
		bRet=pRecognizer->IsHuoquLine(&line,0x01);
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

double RecogHoleDByBlockRef(AcDbBlockTableRecord *pTempBlockTableRecord,double scale)
{
	if (pTempBlockTableRecord == NULL)
		return 0;
	double fHoleD = 0;
	AcDbEntity *pEnt = NULL;
	AcDbBlockTableRecordIterator *pIterator = NULL;
	pTempBlockTableRecord->newIterator(pIterator);
	if (pIterator)
	{
		SCOPE_STRU scope;
		for (; !pIterator->done(); pIterator->step())
		{
			pIterator->getEntity(pEnt, AcDb::kForRead);
			if (pEnt == NULL)
				continue;
			VerifyVertexByCADEnt(scope, pEnt);
			pEnt->close();
		}
		fHoleD = fabs(max(scope.wide(), scope.high())*scale);
		//�Լ���õ��Ŀ׾�����Բ������ȷ��С����һλ
		int nValue=(int)floor(fHoleD);		//��������
		double fValue = fHoleD - nValue;	//С������
		if (fValue < EPS2)	//�׾�Ϊ����
			fHoleD = nValue;
		else if (fValue > EPS_COS2)
			fHoleD = nValue + 1;
		else if (fabs(fValue - 0.5) < EPS2)
			fHoleD = nValue + 0.5;
		else
			fHoleD = ftoi(fHoleD);
	}
	return fHoleD;
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
		if(sName.GetLength()<=0)
			return FALSE;
		BOLT_BLOCK* pBoltD=hashBoltDList.GetValue(sName);
		if (pBoltD == NULL)
		{	//ֻ������δ��˨ͼ���Ŀ���д���������ܴ���ʶ��������Ϊ��˨�� wht 20-04-28
			return FALSE;
		}
		hole.posX=(float)pReference->position().x;
		hole.posY=(float)pReference->position().y;
		if(pBoltD->diameter > 0 || pBoltD->hole_d > 0)
		{	//ָ����˨���ֱ����׾������ձ�׼��˨����
			hole.ciSymbolType = 0;
			if (pBoltD->diameter > 0 && pBoltD->hole_d > pBoltD->diameter)
			{	//ͬʱָ����˨���ֱ���Ϳ׾�
				hole.d = pBoltD->diameter;
				hole.increment = (float)(pBoltD->hole_d - pBoltD->diameter);
			}
			else if (pBoltD->diameter > 0)
			{	//ָ����˨���ֱ��
				hole.d = pBoltD->diameter;
				hole.increment = 1.5;
			}
			else if(pBoltD->hole_d > 0)
			{	//ָ����˨��Ŀ׾�
				hole.d = pBoltD->hole_d;
				hole.increment = 0;
			}
			else
			{
				logerr.LevelLog(CLogFile::WARNING_LEVEL1_IMPORTANT, "��˨ͼ����%s��δ���ö�Ӧ�Ŀ׾���", (char*)sName);
				return FALSE;
			}
		}
		else
		{	//δָ����˨���ֱ���Ϳ׾�,������״����׾����ݿ���ͼԪ�Զ�����
			double fHoleD = RecogHoleDByBlockRef(pTempBlockTableRecord, pReference->scaleFactors().sx);
			if (fHoleD > 0)
			{
				hole.d = fHoleD;
				hole.increment = 0;
				hole.ciSymbolType = 1;	//����ͼ��
			}
			else
			{
				logerr.LevelLog(CLogFile::WARNING_LEVEL1_IMPORTANT, "��˨ͼ����%s��δ���ö�Ӧ�Ŀ׾������ݿ�ʶ��ߴ�ʧ�ܣ�", (char*)sName);
				return FALSE;
			}
		}
		return TRUE;
	}
	else if(pEnt->isKindOf(AcDbCircle::desc()))
	{
		AcDbCircle* pCircle=(AcDbCircle*)pEnt;
		if(int(pCircle->radius())<=0)	//ȥ����
			return FALSE;
		/* �����ֱ��ֱ������ֱ��������׾�����ֵ��
		/*������PPE��ͳһ����׾�����ֵʱ�ᶪʧ�˴���ȡ�Ŀ׾�����ֵ wht 19-09-12
		/*�Կ׾�����Բ������ȷ��С����һλ
		*/
		double fDiameter=pCircle->radius()*2;
		int nValue = (int)floor(fDiameter);	//��������
		double fValue = fDiameter - nValue;	//С������
		if (fValue < EPS2)	//�׾�Ϊ����
			fDiameter = nValue;
		else if (fValue > EPS_COS2)
			fDiameter = nValue + 1;
		else if (fabs(fValue - 0.5) < EPS2)
			fDiameter = nValue + 0.5;
		else
			fDiameter = ftoi(fDiameter);
		hole.d = fDiameter;
		hole.increment = 0;
		hole.ciSymbolType = 2;	//Ĭ�Ϲ��߿�
		hole.posX=(float)pCircle->center().x;
		hole.posY=(float)pCircle->center().y;
		return TRUE;
	}
	else if (pEnt->isKindOf(AcDbEllipse::desc()))
	{
		AcDbEllipse* pEllipse = (AcDbEllipse*)pEnt;
		double radiusRatio = pEllipse->radiusRatio();
		AcGeVector3d minorAxis = pEllipse->minorAxis();
		hole.posX = (float)pEllipse->center().x;
		hole.posY = (float)pEllipse->center().y;
		GEPOINT axis(minorAxis.x, minorAxis.y, minorAxis.z);
		hole.d = axis.mod() * 2;
		//if (fabs(radiusRatio - 1) > EPS2)
		//	return FALSE;
		hole.ciSymbolType = 2;	//Ĭ�Ϲ��߿�
		if (hole.d > MAX_BOLT_HOLE)
			return FALSE;
		else
			return TRUE;
	}
	return FALSE;
}
BOOL CPlateExtractor::RecogBasicInfo(AcDbEntity* pEnt,BASIC_INFO& basicInfo)
{
	if(pEnt==NULL)
		return FALSE;
	//�ӿ��н����ְ���Ϣ
	if (pEnt->isKindOf(AcDbBlockReference::desc()))
	{
		AcDbBlockReference *pBlockRef = (AcDbBlockReference*)pEnt;
		BOOL bRetCode = false;
		AcDbEntity *pSubEnt = NULL;
		AcDbObjectIterator *pIter = pBlockRef->attributeIterator();
		for (pIter->start(); !pIter->done(); pIter->step())
		{
			CAcDbObjLife objLife(pIter->objectId());
			if((pSubEnt=objLife.GetEnt())==NULL)
				continue;
			if (!pSubEnt->isKindOf(AcDbAttribute::desc()))
				continue;
			AcDbAttribute *pAttr = (AcDbAttribute*)pSubEnt;
			CXhChar100 sTag, sText;
#ifdef _ARX_2007
			sTag.Copy(_bstr_t(pAttr->tag()));
			sText.Copy(_bstr_t(pAttr->textString()));
#else
			sTag.Copy(pAttr->tag());
			sText.Copy(pAttr->textString());
#endif
			if (sTag.GetLength() == 0 || sText.GetLength() == 0)
				continue;
			if (sTag.EqualNoCase("����&���&����"))
				bRetCode = RecogBasicInfo(pAttr, basicInfo);
			else if (sTag.EqualNoCase("����"))
			{
				CXhChar50 sTemp(sText);
				for (char* token = strtok(sTemp, "X="); token; token = strtok(NULL, "X="))
				{
					CXhChar16 sToken(token);
					if (sToken.Replace("��", "") > 0)
						basicInfo.m_nNum = atoi(sToken);
				}
			}
			else if (sTag.EqualNoCase("����"))
				basicInfo.m_sTaType.Copy(sText);
			else if (sTag.EqualNoCase("��ӡ"))
			{
				sText.Replace("��ӡ", "");
				sText.Replace(":", "");
				sText.Replace("��", "");
				basicInfo.m_sTaStampNo.Copy(sText);
			}
			else if (sTag.EqualNoCase("�׾�"))
				basicInfo.m_sBoltStr.Copy(sText);
			else if (sTag.EqualNoCase("���̴���"))
			{
				sText.Replace("���̴���", "");
				sText.Replace(":", "");
				sText.Replace("��", "");
				basicInfo.m_sPrjCode.Copy(sText);
			}
		}
		return bRetCode;
	}
	//���ַ����н����ְ���Ϣ
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
				ParseMatText(sTemp, basicInfo.m_cMat,basicInfo.m_cQuality);
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
			if (strstr(sText, "����"))
			{
				sText.Replace("����", "");
				sText.Replace(":", "");
				sText.Replace("��", "");
				basicInfo.m_sTaType.Copy(sText);
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
			ParseMatText(sText, basicInfo.m_cMat,basicInfo.m_cQuality);
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
		if(strstr(sText,"����"))
		{
			sText.Replace("����", "");
			sText.Replace(":", "");
			sText.Replace("��", "");
			basicInfo.m_sTaType.Copy(sText);
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
		if(radius>0 && fabs(angle)>0&&DISTANCE(startPt,endPt)>EPS)
		{	//���˴����Բ��(���磺��ʱpEnt��һ����,����������ʾΪԲ��)
			//��֤startPt-endPt���ص� wht 19-11-11
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
BOOL CPlateExtractor::ParsePartNoText(AcDbEntity *pAcadText, CXhChar16& sPartNo)
{
	if (pAcadText == NULL)
		return FALSE;
	CXhChar100 sText;
	if (pAcadText->isKindOf(AcDbText::desc()))
		sText = GetCadTextContent(pAcadText);
	else if (pAcadText->isKindOf(AcDbMText::desc()))
	{
		sText = GetCadTextContent(pAcadText);
		if (sText.GetLength() <= 0)
			return FALSE;
		//�˴�ʹ��\P�ָ���ܻ���2310P�еĲ����ַ�Ĩȥ����Ҫ���⴦��\P�滻\W wht 19-09-09
		CXhChar100 sNewText;
		char cPreChar = sText.At(0);
		sNewText.Append(cPreChar);
		for (int i = 1; i < sText.GetLength(); i++)
		{
			char cCurChar = sText.At(i);
			if (cPreChar == '\\'&&cCurChar == 'P')
				sNewText.Append('W');
			else
				sNewText.Append(cCurChar);
			cPreChar = cCurChar;
		}
		sText.Copy(sNewText);
		//
		ATOM_LIST<CXhChar50> lineTextList;
		for (char* sKey = strtok(sText, "\\W"); sKey; sKey = strtok(NULL, "\\W"))
		{
			CXhChar50 sTemp(sKey);
			sTemp.Replace("\\W", "");
			lineTextList.append(sTemp);
		}
		if (lineTextList.GetNodeNum() > 0)
		{
			for (CXhChar50 *pLineText = lineTextList.GetFirst(); pLineText; pLineText = lineTextList.GetNext())
			{
				if (IsMatchPNRule(*pLineText))
				{
					sText.Copy(*pLineText);
					break;
				}
			}
		}
	}
	if (!IsMatchPNRule(sText))
		return FALSE;
	BYTE ciRetCode = ParsePartNoText(sText, sPartNo);
	if (ciRetCode == CPlateExtractor::PART_LABEL_WELD)
		return FALSE;	//��ǰ����Ϊ�����Ӽ����� wht 19-07-22
	return TRUE;
}
//////////////////////////////////////////////////////////////////////////
//CJgCardExtractor
CJgCardExtractor::CJgCardExtractor()
{
	fMaxX = 0;
	fMaxY = 0;
	fMinX = 0;
	fMinY = 0;
	fTextHigh = 0;
	fPnDistX = 0;
	fPnDistY = 0;
}
CJgCardExtractor::~CJgCardExtractor()
{

}
BYTE CJgCardExtractor::InitJgCardInfo(const char* sJgCardPath)
{
	if (strlen(sJgCardPath) <= 0)
		return CARD_READ_FAIL;
	f3dPoint startPt, endPt;
	AcDbDatabase blkDb(Adesk::kFalse);//����յ����ݿ�
	Acad::ErrorStatus retCode;
	PRESET_ARRLIST(CXhChar100, partNoPosArr, 10);
#ifdef _ARX_2007
	if ((retCode=blkDb.readDwgFile((ACHAR*)_bstr_t(sJgCardPath), _SH_DENYRW, true)) == Acad::eOk)
#else
	if ((retCode=blkDb.readDwgFile(sJgCardPath, _SH_DENYRW, true)) == Acad::eOk)
#endif
	{
		AcDbEntity *pEnt;
		AcDbBlockTable *pTempBlockTable;
		blkDb.getBlockTable(pTempBlockTable, AcDb::kForRead);
		//��õ�ǰͼ�ο���¼ָ��
		AcDbBlockTableRecord *pTempBlockTableRecord;//�������¼ָ��
		//��д��ʽ��ģ�Ϳռ䣬��ÿ���¼ָ��
		pTempBlockTable->getAt(ACDB_MODEL_SPACE, pTempBlockTableRecord, AcDb::kForRead);
		pTempBlockTable->close();//�رտ��
		AcDbBlockTableRecordIterator *pIterator = NULL;
		pTempBlockTableRecord->newIterator(pIterator);
		SCOPE_STRU scope;
		CXhChar50 sText;
		int nPartLabelCount = 0;
		for (; !pIterator->done(); pIterator->step())
		{
			pIterator->getEntity(pEnt, AcDb::kForRead);
			pEnt->close();
			if (pEnt->isKindOf(AcDbLine::desc()))
			{
				AcDbLine* pLine = (AcDbLine*)pEnt;
				Cpy_Pnt(startPt, pLine->startPoint());
				Cpy_Pnt(endPt, pLine->endPoint());
				scope.VerifyVertex(startPt);
				scope.VerifyVertex(endPt);
				continue;
			}
			if (pEnt->isKindOf(AcDbMText::desc()))
			{
				AcDbMText* pMText = (AcDbMText*)pEnt;
#ifdef _ARX_2007
				sText.Copy(_bstr_t(pMText->contents()));
#else
				sText.Copy(pMText->contents());
#endif
				if (g_pncSysPara.IsPartLabelTitle(sText))
				{
					double fPosX = pMText->location().x;
					double fPosY = pMText->location().y;
					if (nPartLabelCount == 0 || fPnDistX<fPosX || fPnDistY>fPosY)
					{
						fPnDistX = fPosX;
						fPnDistY = fPosY;
						partNoPosArr.Append(CXhChar100("%s λ��(%.1f,%.1f)", (char*)sText, fPosX, fPosY));
					}
					nPartLabelCount++;
				}
				continue;
			}
			if (pEnt->isKindOf(AcDbText::desc()))
			{
				AcDbText* pText = (AcDbText*)pEnt;
#ifdef _ARX_2007
				sText.Copy(_bstr_t(pText->textString()));
#else
				sText.Copy(pText->textString());
#endif
				if (g_pncSysPara.IsPartLabelTitle(sText))
				{
					double fPosX = pText->position().x;
					double fPosY = pText->position().y;
					if (nPartLabelCount == 0 || fPnDistX<fPosX || fPnDistY>fPosY)
					{
						fPnDistX = fPosX;
						fPnDistY = fPosY;
						partNoPosArr.Append(CXhChar100("%s λ��(%.1f,%.1f)",(char*)sText,fPosX,fPosY));
					}
					nPartLabelCount++;
				}
				continue;
			}
			if (!pEnt->isKindOf(AcDbPoint::desc()))
				continue;
			GRID_DATA_STRU grid_data;
			if (!GetGridKey((AcDbPoint*)pEnt, &grid_data))
				continue;
			if (grid_data.type_id == ITEM_TYPE_PART_NO)		//����
			{
				part_no_rect.topLeft.Set(grid_data.min_x, grid_data.max_y);
				part_no_rect.bottomRight.Set(grid_data.max_x, grid_data.min_y);
			}
			else if (grid_data.type_id == ITEM_TYPE_DES_MAT)	//��Ʋ���
			{
				mat_rect.topLeft.Set(grid_data.min_x, grid_data.max_y);
				mat_rect.bottomRight.Set(grid_data.max_x, grid_data.min_y);
			}
			else if (grid_data.type_id == ITEM_TYPE_DES_GUIGE)	//��ƹ��
			{
				guige_rect.topLeft.Set(grid_data.min_x, grid_data.max_y);
				guige_rect.bottomRight.Set(grid_data.max_x, grid_data.min_y);
			}
			else if (grid_data.type_id == ITEM_TYPE_LENGTH)	//����
			{
				length_rect.topLeft.Set(grid_data.min_x, grid_data.max_y);
				length_rect.bottomRight.Set(grid_data.max_x, grid_data.min_y);
			}
			else if (grid_data.type_id == ITEM_TYPE_PIECE_WEIGHT)	//����
			{
				piece_weight_rect.topLeft.Set(grid_data.min_x, grid_data.max_y);
				piece_weight_rect.bottomRight.Set(grid_data.max_x, grid_data.min_y);
			}
			else if (grid_data.type_id == ITEM_TYPE_PART_NUM)	//������
			{
				danji_num_rect.topLeft.Set(grid_data.min_x, grid_data.max_y);
				danji_num_rect.bottomRight.Set(grid_data.max_x, grid_data.min_y);
			}
			else if (grid_data.type_id == ITEM_TYPE_SUM_PART_NUM)	//�ӹ���
			{
				jiagong_num_rect.topLeft.Set(grid_data.min_x, grid_data.max_y);
				jiagong_num_rect.bottomRight.Set(grid_data.max_x, grid_data.min_y);
				fTextHigh = grid_data.fTextHigh;
			}
			else if (grid_data.type_id == ITEM_TYPE_SUM_WEIGHT)	//�ӹ�������
			{
				sum_weight_rect.topLeft.Set(grid_data.min_x, grid_data.max_y);
				sum_weight_rect.bottomRight.Set(grid_data.max_x, grid_data.min_y);
				fTextHigh = grid_data.fTextHigh;
			}
			else if (grid_data.type_id == ITEM_TYPE_PART_NOTES)	//��ע
			{
				note_rect.topLeft.Set(grid_data.min_x, grid_data.max_y);
				note_rect.bottomRight.Set(grid_data.max_x, grid_data.min_y);
			}
			else if (grid_data.type_id == ITEM_TYPE_CUT_ANGLE_S_X)
			{
				cut_angle_SX_rect.topLeft.Set(grid_data.min_x, grid_data.max_y);
				cut_angle_SX_rect.bottomRight.Set(grid_data.max_x, grid_data.min_y);
			}
			else if (grid_data.type_id == ITEM_TYPE_CUT_ANGLE_S_Y)
			{
				cut_angle_SY_rect.topLeft.Set(grid_data.min_x, grid_data.max_y);
				cut_angle_SY_rect.bottomRight.Set(grid_data.max_x, grid_data.min_y);
			}
			else if (grid_data.type_id == ITEM_TYPE_CUT_ANGLE_E_X)
			{
				cut_angle_EX_rect.topLeft.Set(grid_data.min_x, grid_data.max_y);
				cut_angle_EX_rect.bottomRight.Set(grid_data.max_x, grid_data.min_y);
			}
			else if (grid_data.type_id == ITEM_TYPE_CUT_ANGLE_E_Y)
			{
				cut_angle_EY_rect.topLeft.Set(grid_data.min_x, grid_data.max_y);
				cut_angle_EY_rect.bottomRight.Set(grid_data.max_x, grid_data.min_y);
			}
			else if (grid_data.type_id == ITEM_TYPE_HUOQU_FST)
			{
				huoqu_fst_rect.topLeft.Set(grid_data.min_x, grid_data.max_y);
				huoqu_fst_rect.bottomRight.Set(grid_data.max_x, grid_data.min_y);
			}
			else if (grid_data.type_id == ITEM_TYPE_HUOQU_SEC)
			{
				huoqu_sec_rect.topLeft.Set(grid_data.min_x, grid_data.max_y);
				huoqu_sec_rect.bottomRight.Set(grid_data.max_x, grid_data.min_y);
			}
			else if (grid_data.type_id == ITEM_TYPE_CUT_ROOT)	//�ٸ�
			{
				cut_root_rect.topLeft.Set(grid_data.min_x, grid_data.max_y);
				cut_root_rect.bottomRight.Set(grid_data.max_x, grid_data.min_y);
			}
			else if (grid_data.type_id == ITEM_TYPE_CUT_BER)	//����
			{
				cut_ber_rect.topLeft.Set(grid_data.min_x, grid_data.max_y);
				cut_ber_rect.bottomRight.Set(grid_data.max_x, grid_data.min_y);
			}
			else if (grid_data.type_id == ITEM_TYPE_PUSH_FLAT)	//ѹ��
			{
				push_flat_rect.topLeft.Set(grid_data.min_x, grid_data.max_y);
				push_flat_rect.bottomRight.Set(grid_data.max_x, grid_data.min_y);
			}
			else if (grid_data.type_id == ITEM_TYPE_WELD)		//����
			{
				weld_rect.topLeft.Set(grid_data.min_x, grid_data.max_y);
				weld_rect.bottomRight.Set(grid_data.max_x, grid_data.min_y);
			}
			else if (grid_data.type_id == ITEM_TYPE_KAIJIAO)	//����
			{
				kai_jiao_rect.topLeft.Set(grid_data.min_x, grid_data.max_y);
				kai_jiao_rect.bottomRight.Set(grid_data.max_x, grid_data.min_y);
			}
			else if (grid_data.type_id == ITEM_TYPE_HEJIAO)		//�Ͻ�
			{
				he_jiao_rect.topLeft.Set(grid_data.min_x, grid_data.max_y);
				he_jiao_rect.bottomRight.Set(grid_data.max_x, grid_data.min_y);
			}
		}
		pTempBlockTableRecord->close();
		//���տ���������
		fMinX = scope.fMinX;
		fMinY = scope.fMinY;
		fMaxX = scope.fMaxX;
		fMaxY = scope.fMaxY;

		if (nPartLabelCount == 1)
			return CARD_READ_SUCCEED;
		else
		{
			if (partNoPosArr.GetSize() > 1)
			{
				logerr.LogString(CXhChar100("���տ�ͼ��{%s}�д��ڶ���ļ�,��ȷ�ϣ�", (char*)sJgCardPath));
				for (CXhChar100 *pPartPos = partNoPosArr.GetFirst(); pPartPos; pPartPos = partNoPosArr.GetNext())
					logerr.LogString(*pPartPos);
				logerr.ShowToScreen();
			}
			return CARD_READ_ERROR_PARTNO;
		}
	}
	return CARD_READ_FAIL;
}
f3dPoint CJgCardExtractor::GetJgCardOrigin(f3dPoint partNo_pt)
{
	return f3dPoint(partNo_pt.x - fPnDistX, partNo_pt.y - fPnDistY, 0);
}
