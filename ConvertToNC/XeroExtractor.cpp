#include "stdafx.h"
#include "f_alg_fun.h"
#include "ArrayList.h"
#include "XeroExtractor.h"
#include "CadToolFunc.h"
#include "LogFile.h"

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
{	//通过标记符号识别火曲线
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
{	//焊缝线识别
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
//判断坐标点是否在钢板内
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
//判断直线是否在钢板内
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
	{	//部分在多边形区域内
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
//判断提取成功的轮廓点是否按逆时针排序
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
		if(result>0)		// 后点在线左侧，正三角形面积
			wrap_area+=CalTriArea(pnt_arr[0].x,pnt_arr[0].y,pnt_arr[i].x,pnt_arr[i].y,pnt_arr[i+1].x,pnt_arr[i+1].y);
		else if(result<0)	// 后点在线右侧，负三角形面积
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
{	//去除辅助型顶点
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
	//根据圆弧更新顶点信息
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
	//删除不需要的顶点
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
{	//件号标注设置
	m_iDimStyle=0;
	m_sPnKey.Copy("#");
	m_sThickKey.Copy("-");
	m_sMatKey.Copy("Q");
	//螺栓直径设置
	hashBoltDList.SetValue("M24",BOLT_BLOCK("TMA","M24",24));
	hashBoltDList.SetValue("M20",BOLT_BLOCK("TMA", "M20",20));
	hashBoltDList.SetValue("M16",BOLT_BLOCK("TMA", "M16",16));
	hashBoltDList.SetValue("M12",BOLT_BLOCK("TMA", "M12",12));
	hashBoltDList.SetValue("板孔25.5",BOLT_BLOCK("TW", "板孔25.5",24));
	hashBoltDList.SetValue("板孔21.5",BOLT_BLOCK("TW", "板孔21.5",20));
	hashBoltDList.SetValue("板孔17.5",BOLT_BLOCK("TW", "板孔17.5",16));
	hashBoltDList.SetValue("板孔13.5",BOLT_BLOCK("TW", "板孔13.5",12));
	hashBoltDList.SetValue("板孔默认",BOLT_BLOCK("TW", "板孔默认",0));
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
char CPlateExtractor::QueryBriefMatMark(const char* sMatMark)
{
	char cMat='S';
	if(strstr(sMatMark,"Q345"))
		cMat='H';
	//用小写h标识Q355 wht 19-11-05
	else if(strstr(sMatMark,"Q355"))
		cMat='h';
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
	sValue.Replace(" ","");	//取消空格
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
	if(m_iDimStyle==1&&GetPnKeyNum(sText)!=1)	//多行标注，件号文字只能有一个#关键符
		return FALSE;
	if(m_iDimStyle==0)	//单行标注需满足设置条件
	{
		if(strlen(m_sThickKey)>0 && strstr(sText,m_sThickKey)==NULL)
			return FALSE;
		if(strlen(m_sMatKey)>0 && strstr(sText,m_sMatKey)==NULL)
			return FALSE;
		if(strlen(m_sPnNumKey)>0 && strstr(sText,m_sPnNumKey)==NULL)
			return FALSE;
	}
	else	//多行标注设置条件
	{	//多行标注中根据特定文字进行排除(特例化在此不合适)
		//if(strstr(sText,"上")||strstr(sText,"下")||strstr(sText,"焊"))
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
	sValue.Replace("　"," ");
	if(strstr(m_sPnKey,"#"))	//#标识符在件号后面
		sValue.Replace(m_sPnKey,"| "); 
	else
		sValue.Replace(m_sPnKey,"|"); 
	//for(char* sKey=strtok(sValue," \t\\P");sKey;sKey=strtok(NULL," \t\\P"))
	CXhChar100 sPrevStr;
	BOOL bHasWeldFlag = FALSE;
	for(char* sKey=strtok(sValue," \t");sKey;sKey=strtok(NULL," \t"))
	{
		if (strstr(sKey, "|"))
		{	//提取件号
			str.Copy(sKey);
			str.Replace("|", "");
			str.Remove(' ');
			//兼容件号中带空格的情况（比如：101 H,提取为101H） wht 19-07-22
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
			if (strstr(sKey, "焊"))
				bHasWeldFlag = TRUE;
		}
	}
	return PART_LABEL_EMPTY;
}
void CPlateExtractor::ParseThickText(const char* sText,int& nThick)
{
	CXhChar100 str,sValue(sText);
	sValue.Replace("　"," ");
	if(m_iDimStyle==0)
		sValue.Replace(m_sPnKey,"| ");
	//for(char* sKey=strtok(sValue," \t\\P");sKey;sKey=strtok(NULL," \t\\P"))
	for(char* sKey=strtok(sValue," \t");sKey;sKey=strtok(NULL," \t"))
	{
		if(strstr(sKey,m_sThickKey)==NULL||strstr(sKey,"|"))
			continue;
		//查看厚度标识符的位置
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
		{	//厚度标识符在字符串中间
			if(strstr(sKey,"Q")==NULL)
				continue;	//
			CXhChar50 sGroupStr(sKey),sMat,sThick;
			sGroupStr.Replace(m_sThickKey," ");
			sscanf(sGroupStr,"%s%s",(char*)sMat,(char*)sThick);
			sprintf(sKey,"%s%s",(char*)m_sThickKey,(char*)sThick);
		}
		//解析字符串
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
	sValue.Replace("　"," ");
	if(m_iDimStyle==0)
		sValue.Replace(m_sPnKey,"| ");
	CString str(sValue);
	if (strstr(str, "Q") == NULL)	//材质标识中没有Q时才需要将关键字替换为Q wht 19-10-22
		str.Replace(m_sMatKey, " Q");
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
	sValue.Replace("　"," ");
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
	sValue.Replace("　"," ");
	if(strstr(sText,m_sFrontBendKey))
		bFrontBend=TRUE;
	else 
		bFrontBend=FALSE;
	CString str(sValue);
	str.Replace(m_sFrontBendKey,"");
	str.Replace(m_sReverseBendKey,"");
	str.Replace("°","");
	str.Replace("度","");
	if(str.GetLength()>0)
		degree=atof(str);
	else
		degree=0;
}
BOOL CPlateExtractor::IsBendLine(AcDbLine* pAcDbLine,ISymbolRecognizer* pRecognizer/*=NULL*/)
{
	BOOL bRet=FALSE;
	if(pRecognizer!=NULL)
	{	//根据直线与SPLINE分段线求交进行判断
		AcGePoint3d startPt,endPt;
		pAcDbLine->getStartPoint(startPt);
		pAcDbLine->getEndPoint(endPt);
		CStraightLine line(&startPt.x,&endPt.x);
		bRet=pRecognizer->IsIntersWith(&line,0x01);
	}
	if(!bRet)
	{	//TODO:将来应允许通过图层或线型来识别火曲线 wjh-2017.6.3
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
	{	//通过图层识别坡口线
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
			hole.d = pBoltD->hole_d;
			hole.increment = 0;
			/*int nHoleD=(int)floor(pBoltD->hole_d);
			double fHoleFloat=pBoltD->hole_d-nHoleD;
			if(fabs(fHoleFloat)<EPS)
			{	//孔径为整数
				hole.d=(BYTE)pBoltD->hole_d;
				hole.increment=0;
			}
			else 
			{	//孔径为浮点数
				hole.d=(BYTE)(nHoleD-1);
				hole.increment=(float)(1+fHoleFloat);
			}*/
		}
		else
		{	//根据块内图元识别孔径 wht 19-10-16
			double fHoleD = RecogHoleDByBlockRef(pTempBlockTableRecord, pReference->scaleFactors().sx);
			if (fHoleD > 0)
			{
				hole.d = fHoleD;
				hole.increment = 0;
			}
			else
				logerr.LevelLog(CLogFile::WARNING_LEVEL1_IMPORTANT, "螺栓图符（%s）未设置对应的孔径！", (char*)sName);
		}
		hole.ciSymbolType=0;
		return TRUE;
	}
	else if(pEnt->isKindOf(AcDbCircle::desc()))
	{
		AcDbCircle* pCircle=(AcDbCircle*)pEnt;
		if(int(pCircle->radius())<=0)	//去除点
			return FALSE;
		double fDiameter=pCircle->radius()*2;
		hole.posX=(float)pCircle->center().x;
		hole.posY=(float)pCircle->center().y;
		//特殊孔直径直接设置直径，不设孔径增大值，否则在PPE中统一处理孔径增大值时会丢失此处提取的孔径增大值 wht 19-09-12
		hole.d = fDiameter;
		hole.increment = 0;
		/*
		int bolt_d=(int)pCircle->radius()*2;
		hole.d=bolt_d;
		if(fabs(fDiameter-bolt_d)>EPS2)
			hole.increment=(float)fDiameter-bolt_d;
		else
			hole.increment=0;*/
		hole.ciSymbolType=1;	//默认挂线孔
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
				//记录构件数量对应的实体Id wht 19-08-05
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
			//记录构件数量对应的实体Id wht 19-08-05
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
		if(radius>0 && fabs(angle)>0&&DISTANCE(startPt,endPt)>EPS)
		{	//过滤错误的圆弧(例如：有时pEnt是一个点,但是属性显示为圆弧)
			//保证startPt-endPt不重叠 wht 19-11-11
			ciEdgeType=2;
			return arcLine.CreateMethod3(startPt,endPt,norm,radius,center);
		}
	}
	else if(pEnt->isKindOf(AcDbEllipse::desc()))
	{	//更新钢板顶点参数(椭圆)
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
