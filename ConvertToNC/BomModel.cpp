#include "StdAfx.h"
#include "BomModel.h"
#include "ExcelOper.h"
#include "TblDef.h"
#include "DefCard.h"
#include "ArrayList.h"
#include "SortFunc.h"
#include "CadToolFunc.h"
#include "BomTblTitleCfg.h"

#if defined(__UBOM_) || defined(__UBOM_ONLY_)
CBomModel g_xUbomModel;
//TMA_BOM EXCEL �б���
const int TMA_EXCEL_COL_COUNT				= 20;
const char* T_TMA_PART_NO					= "ͼֽ����";
const char* T_TMA_SPEC						= "���";
const char* T_TMA_LEN						= "����";
const char* T_TMA_METERIAL					= "����";
const char* T_TMA_SINGLEBASE_NUM			= "����";
const char* T_TMA_PROCESS_NUM				= "�ӹ�";
const char* T_TMA_SINGLEPIECE_WEIGHT		= "����";
const char* T_TMA_NOTE						= "��ע";
const char* T_TMA_SUM_WEIGHT				= "�ӹ�������";
const char* T_TMA_REPLACE_SPEC				= "���ù��";
//ERP_BOM EXCLE �б���
const int ERP_EXCEL_COL_COUNT				= 12;
const char* T_ERP_PART_NO					= "������";
const char* T_ERP_PART_TYPE					= "����";
const char* T_ERP_METERIAL					= "����";
const char* T_ERP_LEN						= "����";
const char* T_ERP_SPEC						= "���";
const char* T_ERP_NUM						= "����";
const char* T_ERP_SING_WEIGHT				= "����";
const char* T_ERP_NOTE						= "��ע";
const char* T_ERP_SUM_WEIGHT				= "�ӹ�������";
const char* T_ERP_REPLACE_SPEC				= "���ù��";

static CXhChar100 VariantToString(VARIANT value)
{
	CXhChar100 sValue;
	if(value.vt==VT_BSTR)
		return sValue.Copy(CString(value.bstrVal));
	else if(value.vt==VT_R8)
		return sValue.Copy(CXhChar100(value.dblVal));
	else if(value.vt==VT_R4)
		return sValue.Copy(CXhChar100(value.fltVal));
	else if(value.vt==VT_INT)
		return sValue.Copy(CXhChar100(value.intVal));
	else 
		return sValue;
}
//
static CProcessPart* CreatePart(int idPartClsType,const char* key,void* pContext)
{
	CProcessPart* pPart=NULL;
	if(idPartClsType==CGeneralPart::TYPE_LINEANGLE)
		pPart=new CProcessAngle();
	else if(idPartClsType==CGeneralPart::TYPE_PLATE)
		pPart=new CProcessPlate();
	else
		pPart=new CProcessPart();	
	return pPart;
}
static BOOL DestroyPart(CProcessPart* pPart)
{
	if(pPart->m_cPartType==CGeneralPart::TYPE_LINEANGLE)
		delete (CProcessAngle*)pPart;
	else if(pPart->m_cPartType==CGeneralPart::TYPE_PLATE)
		delete (CProcessPlate*)pPart;
	else
		delete (CProcessPart*)pPart;
	return TRUE;
}
int compare_func(const CXhChar16& str1,const CXhChar16& str2)
{
	CString keyStr1(str1),keyStr2(str2);
	return keyStr1.Compare(keyStr2);
}
//////////////////////////////////////////////////////////////////////////
//CBomFile
CBomFile::CBomFile()
{
	m_hashPartByPartNo.CreateNewAtom=CreatePart;
	m_hashPartByPartNo.DeleteAtom=DestroyPart;
	m_bLoftBom=TRUE;
	m_pProject=NULL;
}
CBomFile::~CBomFile()
{

}
void CBomFile::ImportBomFile(const char* sFileName,BOOL bLoftBom)
{
	if(strlen(sFileName)<=0)
		return;
	m_sFileName.Copy(sFileName);
	m_bLoftBom=bLoftBom;
	if(m_bLoftBom)
		ImportTmaExcelFile();
	else
		ImportErpExcelFile();
}
//
void CBomFile::UpdateProcessPart(const char* sOldKey,const char* sNewKey)
{
	CProcessPart* pPart=m_hashPartByPartNo.GetValue(sOldKey);
	if(pPart==NULL)
	{
		logerr.Log("%s�����Ҳ���",sOldKey);
		return;
	}
	pPart=m_hashPartByPartNo.ModifyKeyStr(sOldKey,sNewKey);
	if(pPart)
		pPart->SetPartNo(sNewKey);
}
//��������BOMSHEET����
BOOL CBomFile::ParseTmaSheetContent(CVariant2dArray &sheetContentMap)
{
	if (sheetContentMap.RowsCount() < 1)
		return FALSE;
	CLogErrorLife logLife;
	int iStartRow = 5;
	int startRowArr[10] = {1,2,3,4,5,6,7,8,9,10};
	CHashStrList<DWORD> hashColIndexByColTitle;
	for (int j = 0; j < 10; j++)
	{
		hashColIndexByColTitle.Empty();
		int iTestStartRow = startRowArr[j];
		for (int i = 0; i < TMA_EXCEL_COL_COUNT; i++)
		{
			VARIANT value;
			sheetContentMap.GetValueAt(iTestStartRow, i, value);
			CString str(value.bstrVal);
			if ((str.CompareNoCase(T_TMA_PART_NO) == 0) ||
				CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_PART_NO, str))
				hashColIndexByColTitle.SetValue(T_TMA_PART_NO, i);
			else if ((str.CompareNoCase(T_TMA_SPEC) == 0) ||
				CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_SPEC, str))
				hashColIndexByColTitle.SetValue(T_TMA_SPEC, i);
			else if ((str.CompareNoCase(T_TMA_LEN) == 0) ||
				CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_LEN, str))
				hashColIndexByColTitle.SetValue(T_TMA_LEN, i);
			else if ((str.CompareNoCase(T_TMA_METERIAL) == 0) ||
				CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_METERIAL, str))
				hashColIndexByColTitle.SetValue(T_TMA_METERIAL, i);
			else if ((str.CompareNoCase(T_TMA_SINGLEBASE_NUM) == 0) ||
				CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_NUM, str))
				hashColIndexByColTitle.SetValue(T_TMA_SINGLEBASE_NUM, i);
			else if ((str.CompareNoCase(T_TMA_PROCESS_NUM) == 0) ||
				CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_MANU_NUM, str))
				hashColIndexByColTitle.SetValue(T_TMA_PROCESS_NUM, i);
			else if ((str.CompareNoCase(T_TMA_SINGLEPIECE_WEIGHT) == 0) ||
				CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_SING_WEIGHT, str))
				hashColIndexByColTitle.SetValue(T_TMA_SINGLEPIECE_WEIGHT, i);
			else if ((str.CompareNoCase(T_TMA_SUM_WEIGHT) == 0) ||
				CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_MANU_WEIGHT, str))
				hashColIndexByColTitle.SetValue(T_TMA_SUM_WEIGHT, i);
			else if ((str.CompareNoCase(T_TMA_NOTE) == 0) ||
				CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_NOTES, str))
				hashColIndexByColTitle.SetValue(T_TMA_NOTE, i);
			else if ((str.CompareNoCase(T_TMA_REPLACE_SPEC) == 0) ||
				CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_REPLACE_SPEC, str))
				hashColIndexByColTitle.SetValue(T_TMA_REPLACE_SPEC, i);
		}
		if (hashColIndexByColTitle.GetValue(T_TMA_PART_NO) &&		//����
			hashColIndexByColTitle.GetValue(T_TMA_SPEC) &&			//���
			hashColIndexByColTitle.GetValue(T_TMA_METERIAL))		//����
		{
			iStartRow = startRowArr[j];
			break;
		}
	}
	return ParseTmaSheetContentCore(sheetContentMap, hashColIndexByColTitle, iStartRow);
}

BOOL CBomFile::ParseTmaSheetContentCore(CVariant2dArray &sheetContentMap, CHashStrList<DWORD> &hashColIndexByColTitle,int iStartRow)
{
	if( hashColIndexByColTitle.GetValue(T_TMA_PART_NO)==NULL||		//����
		hashColIndexByColTitle.GetValue(T_TMA_SPEC)==NULL||			//���
		hashColIndexByColTitle.GetValue(T_TMA_METERIAL)==NULL)		//����
	{
		//logerr.Log("ȱ�ٹؼ���(���Ż������ʻ򵥻���)!");
		return FALSE;
	}
	//2.��ȡExcel���е�Ԫ���ֵ
	for(int i=iStartRow+1;i<= sheetContentMap.RowsCount();i++)
	{
		VARIANT value;
		CXhChar100 sPartNo,sMaterial,sSpec,sNote;
		//����
		DWORD *pColIndex=hashColIndexByColTitle.GetValue(T_TMA_PART_NO);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		if(value.vt==VT_EMPTY)
			continue;
		sPartNo=VariantToString(value);
		//���
		int cls_id=0;
		pColIndex=hashColIndexByColTitle.GetValue(T_TMA_SPEC);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		sSpec=VariantToString(value);
		if(strstr(sSpec,"L")||strstr(sSpec,"��"))
		{
			cls_id=CProcessPart::TYPE_LINEANGLE;
			if(strstr(sSpec,"��"))
				sSpec.Replace("��","L");
			if(strstr(sSpec,"��"))
				sSpec.Replace("��","*");
			if(strstr(sSpec,"x"))
				sSpec.Replace("x","*");
		}
		else if(strstr(sSpec,"-"))
			cls_id=CProcessPart::TYPE_PLATE;
		else //if(strstr(sSpec,"��"))
			cls_id=CProcessPart::TYPE_LINETUBE;
		//���ù��
		pColIndex = hashColIndexByColTitle.GetValue(T_ERP_REPLACE_SPEC);
		CXhChar100 sReplaceSpec, sReplaceMat, sTempSpec;
		if (pColIndex != NULL)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sTempSpec = VariantToString(value);
		}
		//����
		float fLength=0;
		pColIndex=hashColIndexByColTitle.GetValue(T_TMA_LEN);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		fLength=(float)atof(VariantToString(value));
		//����
		pColIndex=hashColIndexByColTitle.GetValue(T_TMA_METERIAL);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		sMaterial=VariantToString(value);
		//������
		int nSingleNum=0;
		pColIndex=hashColIndexByColTitle.GetValue(T_TMA_SINGLEBASE_NUM);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		nSingleNum=atoi(VariantToString(value));
		//�ӹ���
		int nProcessNum=0;
		pColIndex=hashColIndexByColTitle.GetValue(T_TMA_PROCESS_NUM);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			nProcessNum = atoi(VariantToString(value));
		}
		//��������
		double fWeight=0,fSumWeight=0;
		pColIndex=hashColIndexByColTitle.GetValue(T_TMA_SINGLEPIECE_WEIGHT);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			fWeight = atof(VariantToString(value));
		}
		pColIndex = hashColIndexByColTitle.GetValue(T_TMA_SUM_WEIGHT);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			fSumWeight = atof(VariantToString(value));
		}
		//��ע
		pColIndex=hashColIndexByColTitle.GetValue(T_TMA_NOTE);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		sNote=VariantToString(value);
		//����ϣ��
		CProcessPart* pProPart = NULL;
		if (m_hashPartByPartNo.GetValue(sPartNo))
		{
			pProPart = m_hashPartByPartNo.GetValue(sPartNo);
			logerr.Log("�����ظ����ţ�%s", (char*)sPartNo);
		}
		if (pProPart == NULL)
		{
			pProPart = m_hashPartByPartNo.Add(sPartNo, cls_id);
			pProPart->m_nDanJiNum = nSingleNum;
			pProPart->feature = nProcessNum;
			pProPart->SetPartNo(sPartNo);
			pProPart->SetSpec(sSpec);
			pProPart->SetNotes(sNote);
			pProPart->cMaterial = CProcessPart::QueryBriefMatMark(sMaterial);
			if (sMaterial.GetLength() == 5)	//��ʼ�������ȼ� wht 19-09-15
				pProPart->cQuality = toupper(sMaterial.At(4));
			if (pProPart->cMaterial == 'A')	//�Ǳ�׼���ʿ⣬m_sRelatePartNo��¼����
				pProPart->m_sRelatePartNo.Copy(sMaterial);
			pProPart->m_wLength = (WORD)fLength;
			pProPart->m_fWeight = (float)fWeight;
			pProPart->m_fSumWeight = (float)fSumWeight;
			int nWidth = 0, nThick = 0;
			CProcessPart::RestoreSpec(sSpec, &nWidth, &nThick);
			pProPart->m_fWidth = (float)nWidth;
			pProPart->m_fThick = (float)nThick;
		}
		else
		{	//�����ظ����ӹ������ۼӼ��� wht 19-09-15
			pProPart->feature += nProcessNum;
		}
	}
	return TRUE;
}
//��������ϵ�EXCEL�ļ�
BOOL CBomFile::ImportTmaExcelFile()
{
	if(m_sFileName.GetLength()<=0)
		return FALSE;
	CExcelOperObject excelobj;
	if (!excelobj.OpenExcelFile(m_sFileName))
		return FALSE;
	LPDISPATCH pWorksheets=excelobj.GetWorksheets();
	if(pWorksheets==NULL)
		return FALSE;
	ASSERT(pWorksheets != NULL);
	Sheets       excel_sheets;	//��������
	excel_sheets.AttachDispatch(pWorksheets);
	int nSheetNum=excel_sheets.GetCount();
	if(nSheetNum<1)
	{
		excel_sheets.ReleaseDispatch();
		return FALSE;
	}
	int index=1;
	int nValidSheetCount = 0;
	m_hashPartByPartNo.Empty();
	int iAngleSheet = CExcelOper::GetExcelIndexOfSpecifySheet(&excelobj, "�Ǹ�����");
	BOOL bRetCode = FALSE;
	if (iAngleSheet > 0)
	{	//���ȶ�ȡ���Ǹ����ϡ��������յ�װ��ʽ��wht 19-09-15
		CVariant2dArray sheetContentMap(1, 1);
		CExcelOper::GetExcelContentOfSpecifySheet(&excelobj, sheetContentMap, iAngleSheet);
		if (ParseTmaSheetContent(sheetContentMap))
		{
			nValidSheetCount++;
			bRetCode = TRUE;
		}
	}
	if (!bRetCode)
	{
		for (int iSheet = 1; iSheet <= nSheetNum; iSheet++)
		{
			CVariant2dArray sheetContentMap(1, 1);
			CExcelOper::GetExcelContentOfSpecifySheet(&excelobj, sheetContentMap, iSheet);
			//2����������
			if (ParseTmaSheetContent(sheetContentMap))
				nValidSheetCount++;
		}
	}
	excel_sheets.ReleaseDispatch();
	if (nValidSheetCount == 0)
	{
		logerr.Log("ȱ�ٹؼ���(���Ż������ʻ򵥻���)!");
		return FALSE;
	}
	else
		return TRUE;
}
//����ERP�ϵ�EXCEL�ļ�
BOOL CBomFile::ImportErpExcelFile()
{
	//1.��ȡExcel���ݴ洢��sheetContentMap��,�����б�����������ӳ���hashColIndexByColTitle
	CVariant2dArray sheetContentMap(1,1);
	if(!CExcelOper::GetExcelContentOfSpecifySheet(m_sFileName,sheetContentMap,1))
		return false;
	CHashStrList<DWORD> hashColIndexByColTitle;
	for(int i=0;i<ERP_EXCEL_COL_COUNT;i++)
	{
		VARIANT value;
		sheetContentMap.GetValueAt(0,i,value);
		if(CString(value.bstrVal).CompareNoCase(T_ERP_PART_NO)==0) 
			hashColIndexByColTitle.SetValue(T_ERP_PART_NO,i);
		else if(CString(value.bstrVal).CompareNoCase(T_ERP_PART_TYPE)==0)
			hashColIndexByColTitle.SetValue(T_ERP_PART_TYPE,i);
		else if(CString(value.bstrVal).CompareNoCase(T_ERP_METERIAL)==0)
			hashColIndexByColTitle.SetValue(T_ERP_METERIAL,i);
		else if(CString(value.bstrVal).CompareNoCase(T_ERP_LEN)==0)
			hashColIndexByColTitle.SetValue(T_ERP_LEN,i);
		else if(CString(value.bstrVal).CompareNoCase(T_ERP_SPEC)==0)
			hashColIndexByColTitle.SetValue(T_ERP_SPEC,i);
		else if(CString(value.bstrVal).CompareNoCase(T_ERP_NUM)==0)
			hashColIndexByColTitle.SetValue(T_ERP_NUM,i);
		else if(CString(value.bstrVal).CompareNoCase(T_ERP_SING_WEIGHT)==0)
			hashColIndexByColTitle.SetValue(T_ERP_SING_WEIGHT,i);
		else if(CString(value.bstrVal).CompareNoCase(T_ERP_NOTE)==0)
			hashColIndexByColTitle.SetValue(T_ERP_NOTE,i);
	}
	if( hashColIndexByColTitle.GetValue(T_ERP_PART_NO)==NULL||		//����
		hashColIndexByColTitle.GetValue(T_ERP_SPEC)==NULL||			//���
		hashColIndexByColTitle.GetValue(T_ERP_METERIAL)==NULL)		//����
	{
		logerr.Log("ȱ�ٹؼ���(���Ż������ʻ򵥻���)!");
		return FALSE;
	}
	//2.��ȡExcel���е�Ԫ���ֵ
	m_hashPartByPartNo.Empty();
	for(int i=1;i<=sheetContentMap.RowsCount();i++)
	{
		VARIANT value;
		CXhChar100 sPartNo,sPartType,sMaterial,sSpec,sNote;
		//����
		DWORD *pColIndex=hashColIndexByColTitle.GetValue(T_ERP_PART_NO);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		if(value.vt==VT_EMPTY)
			continue;
		sPartNo=VariantToString(value);
		//����
		pColIndex=hashColIndexByColTitle.GetValue(T_ERP_METERIAL);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		sMaterial=VariantToString(value);
		//����
		float fLength=0;
		pColIndex=hashColIndexByColTitle.GetValue(T_ERP_LEN);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		fLength=(float)atof(VariantToString(value));
		//���
		int cls_id=0;
		pColIndex=hashColIndexByColTitle.GetValue(T_ERP_SPEC);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		sSpec=VariantToString(value);
		if(strstr(sSpec,"L")||strstr(sSpec,"��"))
		{
			cls_id=CProcessPart::TYPE_LINEANGLE;
			if(strstr(sSpec,"��"))
				sSpec.Replace("��","L");
			if(strstr(sSpec,"��"))
				sSpec.Replace("��","*");
			if(strstr(sSpec,"x"))
				sSpec.Replace("x","*");
		}
		else if(strstr(sSpec,"-"))
			cls_id=CProcessPart::TYPE_PLATE;
		else //if(strstr(sSpec,"��"))
			cls_id=CProcessPart::TYPE_LINETUBE;
		//������
		int nNum=0;
		pColIndex=hashColIndexByColTitle.GetValue(T_ERP_NUM);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		nNum=atoi(VariantToString(value));
		//����
		float fWeight=0;
		pColIndex=hashColIndexByColTitle.GetValue(T_ERP_SING_WEIGHT);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		fWeight=(float)atof(VariantToString(value));
		//��ע
		pColIndex=hashColIndexByColTitle.GetValue(T_ERP_NOTE);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		sNote=VariantToString(value);
		//����ϣ��
		CProcessPart* pProPart=m_hashPartByPartNo.Add(sPartNo,cls_id);
		pProPart->m_nDanJiNum=nNum;
		pProPart->SetPartNo(sPartNo);
		pProPart->SetSpec(sSpec);
		pProPart->SetNotes(sNote);
		pProPart->cMaterial=CProcessPart::QueryBriefMatMark(sMaterial);
		if (sMaterial.GetLength() == 5)	//��ʼ�������ȼ� wht 19-09-15
			pProPart->cQuality = toupper(sMaterial.At(4));
		if(pProPart->cMaterial=='A')
			pProPart->m_sRelatePartNo.Copy(sMaterial);
		pProPart->m_wLength=(WORD)fLength;
		pProPart->m_fWeight=fWeight;
		int nWidth=0,nThick=0;
		CProcessPart::RestoreSpec(sSpec,&nWidth,&nThick);
		pProPart->m_fWidth=(float)nWidth;
		pProPart->m_fThick=(float)nThick;
	}
	return TRUE;
}

CString CBomFile::GetPartNumStr()
{
	int nSumNum = m_hashPartByPartNo.GetNodeNum();
	int nJgNum = 0, nPlateNum = 0, nOtherNum = 0;
	for (CProcessPart *pPart = m_hashPartByPartNo.GetFirst(); pPart; pPart = m_hashPartByPartNo.GetNext())
	{
		if (pPart->IsAngle())
			nJgNum++;
		else if (pPart->IsPlate())
			nPlateNum++;
	}
	nOtherNum = nSumNum - nPlateNum - nJgNum;
	CString sNum;
	if (nSumNum == 0)
		sNum = "0";
	else if (nPlateNum > 0 && nJgNum > 0 && nOtherNum > 0)
		sNum.Format("%d=%d+%d+%d", nSumNum, nJgNum, nPlateNum, nOtherNum);
	else if (nPlateNum > 0 && nJgNum > 0)
		sNum.Format("%d=%d+%d", nSumNum, nJgNum, nPlateNum);
	else
		sNum.Format("%d", nSumNum);
	return sNum;
}

//////////////////////////////////////////////////////////////////////////
//CProjectTowerType
CProjectTowerType::CProjectTowerType()
{
	
}
CProjectTowerType::~CProjectTowerType()
{

}
//��ȡ�ϵ������ļ�
void CProjectTowerType::ReadProjectFile(CString sFilePath)
{
	CFileBuffer file(sFilePath,FALSE);
	double version=1.0;
	file.Read(&version,sizeof(double));
	file.ReadString(m_sProjName);
	CXhChar100 sValue;
	file.ReadString(sValue);	//TMA�ϵ�
	InitBomInfo(sValue,TRUE);
	file.ReadString(sValue);	//ERP�ϵ�
	InitBomInfo(sValue,FALSE);
	//DWG�ļ���Ϣ
	DWORD n=0;
	file.Read(&n,sizeof(DWORD));
	for(DWORD i=0;i<n;i++)
	{
		int ibValue=0;
		file.ReadString(sValue);
		file.Read(&ibValue,sizeof(int));
		AppendDwgBomInfo(sValue,ibValue);
	}
}
//�����ϵ������ļ�
void CProjectTowerType::WriteProjectFile(CString sFilePath)
{
	CFileBuffer file(sFilePath,TRUE);
	double version=1.0;
	file.Write(&version,sizeof(double));
	file.WriteString(m_sProjName);
	file.WriteString(m_xLoftBom.GetFileName());	//TMA�ϵ�
	file.WriteString(m_xOrigBom.GetFileName());	//ERP�ϵ�
	//DWG�ļ���Ϣ
	DWORD n=dwgFileList.GetNodeNum();
	file.Write(&n,sizeof(DWORD));
	for(CDwgFileInfo* pDwgInfo=dwgFileList.GetFirst();pDwgInfo;pDwgInfo=dwgFileList.GetNext())
	{
		file.WriteString(pDwgInfo->GetFileName());
		int ibValue=0;
		if(pDwgInfo->IsJgDwgInfo())
			ibValue=1;
		file.Write(&ibValue,sizeof(int));
	}
}
//��ʼ��BOM��Ϣ
void CProjectTowerType::InitBomInfo(const char* sFileName,BOOL bLoftBom)
{
	if(bLoftBom)
	{
		m_xLoftBom.SetBelongModel(this);
		m_xLoftBom.ImportBomFile(sFileName,bLoftBom);
	}
	else
	{	
		m_xOrigBom.SetBelongModel(this);
		m_xOrigBom.ImportBomFile(sFileName,bLoftBom);
	}
}
//�ȶ�BOM��Ϣ 0.��ͬ 1.��ͬ 2.�ļ�����
int CProjectTowerType::CompareOrgAndLoftParts()
{
	const double COMPARE_EPS=0.5;
	m_hashCompareResultByPartNo.Empty();
	if(m_xLoftBom.GetPartNum()<=0)
	{
		logerr.Log("ȱ�ٷ���BOM��Ϣ!");
		return 2;
	}
	if(m_xOrigBom.GetPartNum()<=0)
	{
		logerr.Log("ȱ�ٹ��տ�BOM��Ϣ");
		return 2;
	}
	CHashStrList<BOOL> hashBoolByPropName;
	for(CProcessPart *pLoftPart=m_xLoftBom.EnumFirstPart();pLoftPart;pLoftPart=m_xLoftBom.EnumNextPart())
	{	
		CProcessPart *pOrgPart = m_xOrigBom.FindPart(pLoftPart->GetPartNo());
		if (pOrgPart == NULL)
		{	//1.���ڷ���������������ԭʼ����
			COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pLoftPart->GetPartNo());
			pResult->pOrgPart=NULL;
			pResult->pLoftPart=pLoftPart;
		}
		else 
		{	//2. �Ա�ͬһ���Ź�������
			hashBoolByPropName.Empty();
			if( pOrgPart->cMaterial!=pLoftPart->cMaterial||
				pOrgPart->cQuality!=pLoftPart->cQuality||
				(pOrgPart->cMaterial=='A'&&!pOrgPart->m_sRelatePartNo.Equal(pLoftPart->m_sRelatePartNo)))		//����
				hashBoolByPropName.SetValue("cMaterial",TRUE);
			if(pOrgPart->IsAngle() && fabsl(pOrgPart->m_wLength-pLoftPart->m_wLength)>50)	//���ݿͻ����󣬳��ȱȽ������50����
				hashBoolByPropName.SetValue("m_fLength",TRUE);
			if(pOrgPart->m_nDanJiNum!=pLoftPart->m_nDanJiNum)		//������
				hashBoolByPropName.SetValue("m_nDanJiNum",TRUE);
			if(stricmp(pOrgPart->GetSpec(FALSE),pLoftPart->GetSpec(FALSE))!=0)	//���
				hashBoolByPropName.SetValue("spec",TRUE);
			if(hashBoolByPropName.GetNodeNum()>0)//�������
			{	
				COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pLoftPart->GetPartNo());
				pResult->pOrgPart=pOrgPart;
				pResult->pLoftPart=pLoftPart;
				for(BOOL *pValue=hashBoolByPropName.GetFirst();pValue;pValue=hashBoolByPropName.GetNext())
					pResult->hashBoolByPropName.SetValue(hashBoolByPropName.GetCursorKey(),*pValue);
			}
		}
	}
	//2.3 ���������ԭʼ��,�����Ƿ���©�����
	for(CProcessPart *pPart=m_xOrigBom.EnumFirstPart();pPart;pPart=m_xOrigBom.EnumNextPart())
	{
		if(m_xLoftBom.FindPart(pPart->GetPartNo()))
			continue;
		COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pPart->GetPartNo());
		pResult->pOrgPart=pPart;
		pResult->pLoftPart=NULL;
	}
	if(m_hashCompareResultByPartNo.GetNodeNum()==0)
	{
		AfxMessageBox("����BOM�͹��տ�BOM��Ϣ��ͬ!");
		return 0;
	}
	else
		return 1;
}
//���нǸ�DWG�ļ���©�ż��
int CProjectTowerType::CompareLoftAndAngleDwgs()
{
	m_hashCompareResultByPartNo.Empty();
	CHashStrList<BOOL> hashAngleByPartNo;
	for(CDwgFileInfo* pJgDwg=dwgFileList.GetFirst();pJgDwg;pJgDwg=dwgFileList.GetNext())
	{
		if(!pJgDwg->IsJgDwgInfo())
			continue;
		for(CAngleProcessInfo* pJgInfo=pJgDwg->EnumFirstJg();pJgInfo;pJgInfo=pJgDwg->EnumNextJg())
			hashAngleByPartNo.SetValue(pJgInfo->m_xAngle.GetPartNo(),TRUE);
	}
	for(CProcessPart *pPart=m_xLoftBom.EnumFirstPart();pPart;pPart=m_xLoftBom.EnumNextPart())
	{
		if(!pPart->IsAngle())
			continue;
		if(hashAngleByPartNo.GetValue(pPart->GetPartNo()))
			continue;
		COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pPart->GetPartNo());
		pResult->pOrgPart=NULL;
		pResult->pLoftPart=pPart;
	}
	if(m_hashCompareResultByPartNo.GetNodeNum()==0)
	{
		AfxMessageBox("�Ѵ򿪽Ǹ�DWG�ļ�û��©�����!");
		return 0;
	}
	else
		return 1;
}
//���иְ�DWG�ļ���©�ż��
int CProjectTowerType::CompareLoftAndPlateDwgs()
{
	m_hashCompareResultByPartNo.Empty();
	CHashStrList<BOOL> hashPlateByPartNo;
	for(CDwgFileInfo* pPlateDwg=dwgFileList.GetFirst();pPlateDwg;pPlateDwg=dwgFileList.GetNext())
	{
		if(pPlateDwg->IsJgDwgInfo())
			continue;
		for(CPlateProcessInfo* pPlateInfo=pPlateDwg->EnumFirstPlate();pPlateInfo;pPlateInfo=pPlateDwg->EnumNextPlate())
			hashPlateByPartNo.SetValue(pPlateInfo->xPlate.GetPartNo(),TRUE);
	}
	for(CProcessPart *pPart=m_xLoftBom.EnumFirstPart();pPart;pPart=m_xLoftBom.EnumNextPart())
	{
		if(!pPart->IsPlate())
			continue;
		if(hashPlateByPartNo.GetValue(pPart->GetPartNo()))
			continue;
		COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pPart->GetPartNo());
		pResult->pOrgPart=NULL;
		pResult->pLoftPart=pPart;
	}
	if(m_hashCompareResultByPartNo.GetNodeNum()==0)
	{
		AfxMessageBox("�Ѵ򿪸ְ�DWG�ļ�û��©�����!");
		return 0;
	}
	else
		return 1;
}
//��ӽǸ�DWGBOM��Ϣ
CDwgFileInfo* CProjectTowerType::AppendDwgBomInfo(const char* sFileName,BOOL bJgDxf)
{
	if(strlen(sFileName)<=0)
		return FALSE;
	//��DWG�ļ�
	CXhChar500 file_path;
	AcApDocument *pDoc=NULL;
	AcApDocumentIterator *pIter=acDocManager->newAcApDocumentIterator();
	for(;!pIter->done();pIter->step())
	{
		pDoc=pIter->document();
#ifdef _ARX_2007
		file_path.Copy(_bstr_t(pDoc->fileName()));
#else
		file_path.Copy(pDoc->fileName());
#endif
		if(strstr(file_path,sFileName))
			break;
	}
	if(strstr(file_path,sFileName))	//����ָ���ļ�
		acDocManager->activateDocument(pDoc);
	else
	{		//��ָ���ļ�
#ifdef _ARX_2007
		acDocManager->appContextOpenDocument(_bstr_t(sFileName));
#else
		acDocManager->appContextOpenDocument((const char*)sFileName);
#endif
	}
	//��ȡDWG�ļ���Ϣ
	CDwgFileInfo* pDwgFile=FindDwgBomInfo(sFileName);
	if(pDwgFile)
	{	
		pDwgFile->InitDwgInfo(sFileName,bJgDxf);
		return pDwgFile;
	}
	else
	{	
		pDwgFile=dwgFileList.append();
		pDwgFile->SetBelongModel(this);
		//��ȡDWG��Ϣ
		pDwgFile->InitDwgInfo(sFileName, bJgDxf);
		return pDwgFile;
		/*if(pDwgFile->InitDwgInfo(sFileName,bJgDxf))
			return pDwgFile;
		else
		{
			dwgFileList.DeleteCursor();
			dwgFileList.Clean();
			return NULL;
		}*/
	}
}
//
CDwgFileInfo* CProjectTowerType::FindDwgBomInfo(const char* sFileName)
{
	CDwgFileInfo* pDwgInfo=NULL;
	for(pDwgInfo=dwgFileList.GetFirst();pDwgInfo;pDwgInfo=dwgFileList.GetNext())
	{
		if(strcmp(pDwgInfo->GetFileName(),sFileName)==0)
			break;
	}
	return pDwgInfo;
}
//
int CProjectTowerType::CompareLoftAndAngleDwg(const char* sFileName)
{
	const double COMPARE_EPS=0.5;
	if(m_xLoftBom.GetPartNum()<=0)
	{
		logerr.Log("ȱ�ٷ���BOM��Ϣ!");
		return 2;
	}
	CDwgFileInfo* pJgDwgFile=FindDwgBomInfo(sFileName);
	if(pJgDwgFile==NULL)
	{
		logerr.Log("δ�ҵ�ָ���ĽǸ�DWG�ļ�!");
		return 2;
	}
	m_hashCompareResultByPartNo.Empty();
	CHashStrList<BOOL> hashBoolByPropName;
	CAngleProcessInfo* pJgInfo=NULL;
	CHashStrList<CAngleProcessInfo*> hashDwgAngleByPartNo;
	for(pJgInfo=pJgDwgFile->EnumFirstJg();pJgInfo;pJgInfo=pJgDwgFile->EnumNextJg())
	{
		CXhChar16 sPartNo=pJgInfo->m_xAngle.GetPartNo();
		hashDwgAngleByPartNo.SetValue(sPartNo, pJgInfo);
		CProcessPart *pLoftJg=m_xLoftBom.FindPart(sPartNo);
		if(pLoftJg==NULL)
		{	//1������DWG�����������ڷ�������
			COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(sPartNo);
			pResult->pOrgPart=&(pJgInfo->m_xAngle);
			pResult->pLoftPart=NULL;
		}
		else
		{	//2���Ա�ͬһ���Ź�������
			hashBoolByPropName.Empty();
			char cMat1 = pJgInfo->m_xAngle.cMaterial;
			char cMat2 = pLoftJg->cMaterial;
			if (cMat1 == 0)
				cMat1 = 'S';
			if (cMat2 == 0)
				cMat2 = 'S';
			if( cMat1!=cMat2||
				pJgInfo->m_xAngle.cQuality!=pLoftJg->cQuality||
				(cMat2=='A'&&!pLoftJg->m_sRelatePartNo.Equal(pJgInfo->m_xAngle.m_sRelatePartNo)))			//����
				hashBoolByPropName.SetValue("cMaterial",TRUE);
			if(fabsl(pJgInfo->m_xAngle.m_wLength-pLoftJg->m_wLength)>0)		//����--���ݿͻ�����,���ȱȽ�Ҫ��ȫһ��
				hashBoolByPropName.SetValue("m_fLength",TRUE);
			if(pJgInfo->m_xAngle.m_nDanJiNum!=pLoftJg->m_nDanJiNum)			//������
				hashBoolByPropName.SetValue("m_nDanJiNum",TRUE);
			if(stricmp(pJgInfo->m_xAngle.GetSpec(FALSE),pLoftJg->GetSpec(FALSE))!=0)	//���
				hashBoolByPropName.SetValue("spec",TRUE);
			if(hashBoolByPropName.GetNodeNum()>0)//�������
			{	
				COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(sPartNo);
				pResult->pOrgPart=&(pJgInfo->m_xAngle);
				pResult->pLoftPart=pLoftJg;
				for(BOOL *pValue=hashBoolByPropName.GetFirst();pValue;pValue=hashBoolByPropName.GetNext())
					pResult->hashBoolByPropName.SetValue(hashBoolByPropName.GetCursorKey(),*pValue);
			}
		}
	}
	for (CProcessPart *pPart = m_xLoftBom.EnumFirstPart(); pPart; pPart = m_xLoftBom.EnumNextPart())
	{
		if (!pPart->IsAngle())
			continue;
		if (hashDwgAngleByPartNo.GetValue(pPart->GetPartNo()))
			continue;
		//3������BOM������������DWG����
		COMPARE_PART_RESULT *pResult = m_hashCompareResultByPartNo.Add(pPart->GetPartNo());
		pResult->pOrgPart = NULL;
		pResult->pLoftPart = pPart;
	}
	if(m_hashCompareResultByPartNo.GetNodeNum()==0)
	{
		AfxMessageBox("�����Ǹ���Ϣ��DWG�Ǹ���Ϣ��ͬ!");
		return 0;
	}
	else
		return 1;
}
//
int CProjectTowerType::CompareLoftAndPlateDwg(const char* sFileName)
{
	const double COMPARE_EPS=0.5;
	if(m_xLoftBom.GetPartNum()<=0)
	{
		logerr.Log("ȱ�ٷ���BOM��Ϣ!");
		return 2;
	}
	CDwgFileInfo* pPlateDwgFile=FindDwgBomInfo(sFileName);
	if(pPlateDwgFile==NULL)
	{
		logerr.Log("δ�ҵ�ָ���ĸְ�DWG�ļ�!");
		return 2;
	}
	m_hashCompareResultByPartNo.Empty();
	CHashStrList<BOOL> hashBoolByPropName;
	CHashStrList<CPlateProcessInfo*> hashDwgPlateByPartNo;
	CPlateProcessInfo* pPlateInfo=NULL;
	for(pPlateInfo=pPlateDwgFile->EnumFirstPlate();pPlateInfo;pPlateInfo=pPlateDwgFile->EnumNextPlate())
	{
		CXhChar16 sPartNo=pPlateInfo->xPlate.GetPartNo();
		hashDwgPlateByPartNo.SetValue(sPartNo, pPlateInfo);
		CProcessPart *pLoftPlate=m_xLoftBom.FindPart(sPartNo);
		if(pLoftPlate==NULL)
		{	//1������DWG�����������ڷ�������
			COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(sPartNo);
			pResult->pOrgPart=&(pPlateInfo->xPlate);
			pResult->pLoftPart=NULL;
		}
		else
		{	//2���Ա�ͬһ���Ź�������
			hashBoolByPropName.Empty();
			char cMat1 = pPlateInfo->xPlate.cMaterial;
			char cMat2 = pLoftPlate->cMaterial;
			if (cMat1 == 0)
				cMat1 = 'S';
			if (cMat2 == 0)
				cMat2 = 'S';
			if( cMat1!=cMat2||
				pPlateInfo->xPlate.cQuality!=pLoftPlate->cQuality||
				(pLoftPlate->cMaterial=='A'&&!pLoftPlate->m_sRelatePartNo.Equal(pPlateInfo->xPlate.m_sRelatePartNo)))				//����
				hashBoolByPropName.SetValue("cMaterial",TRUE);
			if(stricmp(pPlateInfo->xPlate.GetSpec(FALSE),pLoftPlate->GetSpec(FALSE))!=0)	//���
				hashBoolByPropName.SetValue("spec",TRUE);
			if(hashBoolByPropName.GetNodeNum()>0)//�������
			{	
				COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(sPartNo);
				pResult->pOrgPart=&(pPlateInfo->xPlate);
				pResult->pLoftPart=pLoftPlate;
				for(BOOL *pValue=hashBoolByPropName.GetFirst();pValue;pValue=hashBoolByPropName.GetNext())
					pResult->hashBoolByPropName.SetValue(hashBoolByPropName.GetCursorKey(),*pValue);
			}
		}
	}
	for (CProcessPart *pPart = m_xLoftBom.EnumFirstPart(); pPart; pPart = m_xLoftBom.EnumNextPart())
	{
		if (!pPart->IsPlate())
			continue;
		if (hashDwgPlateByPartNo.GetValue(pPart->GetPartNo()))
			continue;
		//3������BOM������������DWG����
		COMPARE_PART_RESULT *pResult = m_hashCompareResultByPartNo.Add(pPart->GetPartNo());
		pResult->pOrgPart = NULL;
		pResult->pLoftPart = pPart;
	}
	if(m_hashCompareResultByPartNo.GetNodeNum()==0)
	{
		AfxMessageBox("�����ְ���Ϣ��DWG�ְ���Ϣ��ͬ!");
		return 0;
	}
	else
		return 1;
}
//
void CProjectTowerType::AddBomResultSheet(LPDISPATCH pSheet,int iSheet)
{
	//��У������������
	ARRAY_LIST<CXhChar16> keyStrArr;
	for (COMPARE_PART_RESULT *pResult=EnumFirstResult();pResult;pResult=EnumNextResult())//�����洢�Ľ����
	{
		if(pResult->pLoftPart)
			keyStrArr.append(pResult->pLoftPart->GetPartNo());
		else
			keyStrArr.append(pResult->pOrgPart->GetPartNo());
	}
	CQuickSort<CXhChar16>::QuickSort(keyStrArr.m_pData,keyStrArr.GetSize(),compare_func);
	//��ӱ�����
	_Worksheet excel_sheet;
	excel_sheet.AttachDispatch(pSheet,FALSE);
	excel_sheet.Select();
	CStringArray str_arr;
	str_arr.SetSize(6);
	str_arr[0]="�������";str_arr[1]="��ƹ��";str_arr[2]="����";
	str_arr[3]="����";str_arr[4]="������";str_arr[5]="������Դ";
	double col_arr[6]={15,15,15,15,15,15};
	CExcelOper::AddRowToExcelSheet(excel_sheet,1,str_arr,col_arr,TRUE);
	//�������
	char cell_start[16]="A1";
	char cell_end[16]="A1";
	int nResult=GetResultCount();
	CVariant2dArray map(nResult*2,6);//��ȡExcel���ķ�Χ
	int index=0;
	if(iSheet==1)
	{	//��һ�ֽ����ERP��TMA���е�������Ϣ��ͬ
		excel_sheet.SetName("У����");
		for(int i=0;i<keyStrArr.GetSize();i++)
		{
			COMPARE_PART_RESULT *pResult=GetResult(keyStrArr[i]);
			if(pResult==NULL || pResult->pLoftPart==NULL || pResult->pOrgPart==NULL)
				continue;
			_snprintf(cell_start,15,"A%d",index+2);
			_snprintf(cell_end,15,"A%d",index+3);
			CExcelOper::MergeRowRange(excel_sheet,cell_start,cell_end);	//�ϲ���
			map.SetValueAt(index,0,COleVariant(pResult->pOrgPart->GetPartNo()));
			map.SetValueAt(index,1,COleVariant(pResult->pOrgPart->GetSpec(FALSE)));
			map.SetValueAt(index,2,COleVariant(CBomModel::QueryMatMarkIncQuality(pResult->pOrgPart)));
			map.SetValueAt(index,3,COleVariant(CXhChar50("%d",pResult->pOrgPart->m_wLength)));
			map.SetValueAt(index,4,COleVariant((long)pResult->pOrgPart->m_nDanJiNum));
			map.SetValueAt(index,5,COleVariant(CXhChar16("ERP")));
			//
			if(pResult->hashBoolByPropName.GetValue("spec"))
			{
				map.SetValueAt(index+1,1,COleVariant(pResult->pLoftPart->GetSpec(FALSE)));
				_snprintf(cell_start,15,"B%d",index+2);
				CExcelOper::SetRangeBackColor(excel_sheet,42,cell_start);
				_snprintf(cell_start,15,"B%d",index+3);
				CExcelOper::SetRangeBackColor(excel_sheet,44,cell_start);
			}
			if(pResult->hashBoolByPropName.GetValue("cMaterial"))
			{	
				map.SetValueAt(index+1,2,COleVariant(CBomModel::QueryMatMarkIncQuality(pResult->pLoftPart)));
				_snprintf(cell_start,15,"C%d",index+2);
				CExcelOper::SetRangeBackColor(excel_sheet,42,cell_start);
				_snprintf(cell_start,15,"C%d",index+3);
				CExcelOper::SetRangeBackColor(excel_sheet,44,cell_start);
			}
			if(pResult->hashBoolByPropName.GetValue("m_fLength"))
			{	
				map.SetValueAt(index+1,3,COleVariant(CXhChar50("%d",pResult->pLoftPart->m_wLength)));
				_snprintf(cell_start,15,"D%d",index+2);
				CExcelOper::SetRangeBackColor(excel_sheet,42,cell_start);
				_snprintf(cell_start,15,"D%d",index+3);
				CExcelOper::SetRangeBackColor(excel_sheet,44,cell_start);
			}
			if(pResult->hashBoolByPropName.GetValue("m_nDanJiNum"))
			{	
				map.SetValueAt(index+1,4,COleVariant((long)pResult->pLoftPart->m_nDanJiNum));
				_snprintf(cell_start,15,"E%d",index+2);
				CExcelOper::SetRangeBackColor(excel_sheet,42,cell_start);
				_snprintf(cell_start,15,"E%d",index+3);
				CExcelOper::SetRangeBackColor(excel_sheet,44,cell_start);
			}
			map.SetValueAt(index+1,5,COleVariant(CXhChar16("TMA")));
			index+=2;
		}
	}
	else if(iSheet==2)
	{	//�ڶ��ֽ�����������������ݣ�����ԭʼ����û������
		excel_sheet.SetName("TMA��ר�й���");
		for(int i=0;i<keyStrArr.GetSize();i++)
		{
			COMPARE_PART_RESULT *pResult=GetResult(keyStrArr[i]);
			if(pResult==NULL || pResult->pLoftPart==NULL || pResult->pOrgPart)
				continue;
			map.SetValueAt(index,0,COleVariant(pResult->pLoftPart->GetPartNo()));
			map.SetValueAt(index,1,COleVariant(pResult->pLoftPart->GetSpec(FALSE)));
			map.SetValueAt(index,2,COleVariant(CBomModel::QueryMatMarkIncQuality(pResult->pLoftPart)));
			map.SetValueAt(index,3,COleVariant(CXhChar50("%d",pResult->pLoftPart->m_wLength)));
			map.SetValueAt(index,4,COleVariant((long)pResult->pLoftPart->m_nDanJiNum));
			index++;
		}
	}
	else if(iSheet==3)
	{	//�����ֽ��������ԭʼ���������ݣ���������û������
		excel_sheet.SetName("ERP��ר�й���");
		for(int i=0;i<keyStrArr.GetSize();i++)
		{
			COMPARE_PART_RESULT *pResult=GetResult(keyStrArr[i]);
			if(pResult==NULL || pResult->pLoftPart || pResult->pOrgPart==NULL)
				continue;
			map.SetValueAt(index,0,COleVariant(pResult->pOrgPart->GetPartNo()));
			map.SetValueAt(index,1,COleVariant(pResult->pOrgPart->GetSpec(FALSE)));
			map.SetValueAt(index,2,COleVariant(CBomModel::QueryMatMarkIncQuality(pResult->pOrgPart)));
			map.SetValueAt(index,3,COleVariant(CXhChar50("%d",pResult->pOrgPart->m_wLength)));
			map.SetValueAt(index,4,COleVariant((long)pResult->pOrgPart->m_nDanJiNum));
			index++;
		}
	}
	_snprintf(cell_end,15,"F%d",index+1);
	if(index>0)
		CExcelOper::SetRangeValue(excel_sheet,"A2",cell_end,map.var);
	CExcelOper::SetRangeHorizontalAlignment(excel_sheet,"A1",cell_end,COleVariant((long)3));
	CExcelOper::SetRangeBorders(excel_sheet,"A1",cell_end,COleVariant(10.5));
}
void CProjectTowerType::AddAngleResultSheet(LPDISPATCH pSheet,int iSheet)
{
	_Worksheet excel_sheet;
	excel_sheet.AttachDispatch(pSheet,FALSE);
	excel_sheet.Select();
	//��ӱ�����
	CStringArray str_arr;
	str_arr.SetSize(6);
	str_arr[0]="�������";str_arr[1]="��ƹ��";str_arr[2]="����";
	str_arr[3]="����";str_arr[4]="������";str_arr[5]="������Դ";
	double col_arr[6]={15,15,15,15,15,15};
	CExcelOper::AddRowToExcelSheet(excel_sheet,1,str_arr,col_arr,TRUE);
	//�������
	char cell_start[16]="A1";
	char cell_end[16]="A1";
	int nResult=GetResultCount();
	CVariant2dArray map(nResult*2,6);//��ȡExcel���ķ�Χ
	int index=0;
	if(iSheet==1)
	{	//��һ�ֽ����DWG��TMA���е�������Ϣ��ͬ
		excel_sheet.SetName("У����");
		for(COMPARE_PART_RESULT *pResult=EnumFirstResult();pResult;pResult=EnumNextResult())
		{	
			if(pResult->pLoftPart==NULL || pResult->pOrgPart==NULL)
				continue;
			_snprintf(cell_start,15,"A%d",index+2);
			_snprintf(cell_end,15,"A%d",index+3);
			CExcelOper::MergeRowRange(excel_sheet,cell_start,cell_end);	//�ϲ���
			map.SetValueAt(index,0,COleVariant(pResult->pOrgPart->GetPartNo()));
			map.SetValueAt(index,1,COleVariant(pResult->pOrgPart->GetSpec(FALSE)));
			map.SetValueAt(index,2,COleVariant(CBomModel::QueryMatMarkIncQuality(pResult->pOrgPart)));
			map.SetValueAt(index,3,COleVariant(CXhChar50("%d",pResult->pOrgPart->m_wLength)));
			map.SetValueAt(index,4,COleVariant((long)pResult->pOrgPart->m_nDanJiNum));
			map.SetValueAt(index,5,COleVariant(CXhChar16("DWG")));
			//
			if(pResult->hashBoolByPropName.GetValue("spec"))
			{
				map.SetValueAt(index+1,1,COleVariant(pResult->pLoftPart->GetSpec(FALSE)));
				_snprintf(cell_start,15,"B%d",index+2);
				CExcelOper::SetRangeBackColor(excel_sheet,42,cell_start);
				_snprintf(cell_start,15,"B%d",index+3);
				CExcelOper::SetRangeBackColor(excel_sheet,44,cell_start);
			}
			if(pResult->hashBoolByPropName.GetValue("cMaterial"))
			{	
				map.SetValueAt(index+1,2,COleVariant(CBomModel::QueryMatMarkIncQuality(pResult->pLoftPart)));
				_snprintf(cell_start,15,"C%d",index+2);
				CExcelOper::SetRangeBackColor(excel_sheet,42,cell_start);
				_snprintf(cell_start,15,"C%d",index+3);
				CExcelOper::SetRangeBackColor(excel_sheet,44,cell_start);
			}
			if(pResult->hashBoolByPropName.GetValue("m_fLength"))
			{	
				map.SetValueAt(index+1,3,COleVariant(CXhChar50("%d",pResult->pLoftPart->m_wLength)));
				_snprintf(cell_start,15,"D%d",index+2);
				CExcelOper::SetRangeBackColor(excel_sheet,42,cell_start);
				_snprintf(cell_start,15,"D%d",index+3);
				CExcelOper::SetRangeBackColor(excel_sheet,44,cell_start);
			}
			if(pResult->hashBoolByPropName.GetValue("m_nDanJiNum"))
			{	
				map.SetValueAt(index+1,4,COleVariant((long)pResult->pLoftPart->m_nDanJiNum));
				_snprintf(cell_start,15,"E%d",index+2);
				CExcelOper::SetRangeBackColor(excel_sheet,42,cell_start);
				_snprintf(cell_start,15,"E%d",index+3);
				CExcelOper::SetRangeBackColor(excel_sheet,44,cell_start);
			}
			map.SetValueAt(index+1,5,COleVariant(CXhChar16("TMA")));
			index+=2;
		}
	}
	else if(iSheet==2)
	{	//�ڶ��ֽ����DWG�ļ����������ݣ���������û������
		excel_sheet.SetName("DWG��ר�й���");
		index++;
		for (COMPARE_PART_RESULT *pResult=EnumFirstResult();pResult;pResult=EnumNextResult())
		{
			if(pResult->pLoftPart || pResult->pOrgPart==NULL)
				continue;
			map.SetValueAt(index,0,COleVariant(pResult->pOrgPart->GetPartNo()));
			map.SetValueAt(index,1,COleVariant(pResult->pOrgPart->GetSpec(FALSE)));
			map.SetValueAt(index,2,COleVariant(CBomModel::QueryMatMarkIncQuality(pResult->pOrgPart)));
			map.SetValueAt(index,3,COleVariant(CXhChar50("%d",pResult->pOrgPart->m_wLength)));
			map.SetValueAt(index,4,COleVariant((long)pResult->pOrgPart->m_nDanJiNum));
			index++;
		}
	}
	_snprintf(cell_end,15,"F%d",index+1);
	if(index>0)
		CExcelOper::SetRangeValue(excel_sheet,"A2",cell_end,map.var);
	CExcelOper::SetRangeHorizontalAlignment(excel_sheet,"A1",cell_end,COleVariant((long)3));
	CExcelOper::SetRangeBorders(excel_sheet,"A1",cell_end,COleVariant(10.5));
}
void CProjectTowerType::AddPlateResultSheet(LPDISPATCH pSheet,int iSheet)
{
	_Worksheet excel_sheet;
	excel_sheet.AttachDispatch(pSheet,FALSE);
	excel_sheet.Select();
	//��ӱ�����
	CStringArray str_arr;
	str_arr.SetSize(4);
	str_arr[0]="�������";str_arr[1]="��ƹ��";
	str_arr[2]="����";	  str_arr[3]="������Դ";
	double col_arr[4]={15,15,15,15};
	CExcelOper::AddRowToExcelSheet(excel_sheet,1,str_arr,col_arr,TRUE);
	//�������
	char cell_start[16]="A1";
	char cell_end[16]="A1";
	int nResult=GetResultCount();
	CVariant2dArray map(nResult*2,4);//��ȡExcel���ķ�Χ
	int index=0;
	if(iSheet==1)
	{	//��һ�ֽ����DWG��TMA���е�������Ϣ��ͬ
		excel_sheet.SetName("У����");
		for(COMPARE_PART_RESULT *pResult=EnumFirstResult();pResult;pResult=EnumNextResult())
		{	
			if(pResult->pLoftPart==NULL || pResult->pOrgPart==NULL)
				continue;
			_snprintf(cell_start,15,"A%d",index+2);
			_snprintf(cell_end,15,"A%d",index+3);
			CExcelOper::MergeRowRange(excel_sheet,cell_start,cell_end);	//�ϲ���
			map.SetValueAt(index,0,COleVariant(pResult->pOrgPart->GetPartNo()));
			map.SetValueAt(index,1,COleVariant(pResult->pOrgPart->GetSpec(FALSE)));
			map.SetValueAt(index,2,COleVariant(CBomModel::QueryMatMarkIncQuality(pResult->pOrgPart)));
			map.SetValueAt(index,3,COleVariant(CXhChar16("DWG")));
			//
			if(pResult->hashBoolByPropName.GetValue("spec"))
			{
				map.SetValueAt(index+1,1,COleVariant(pResult->pLoftPart->GetSpec(FALSE)));
				_snprintf(cell_start,15,"B%d",index+2);
				CExcelOper::SetRangeBackColor(excel_sheet,42,cell_start);
				_snprintf(cell_start,15,"B%d",index+3);
				CExcelOper::SetRangeBackColor(excel_sheet,44,cell_start);
			}
			if(pResult->hashBoolByPropName.GetValue("cMaterial"))
			{	
				map.SetValueAt(index+1,2,COleVariant(CBomModel::QueryMatMarkIncQuality(pResult->pLoftPart)));
				_snprintf(cell_start,15,"C%d",index+2);
				CExcelOper::SetRangeBackColor(excel_sheet,42,cell_start);
				_snprintf(cell_start,15,"C%d",index+3);
				CExcelOper::SetRangeBackColor(excel_sheet,44,cell_start);
			}
			map.SetValueAt(index+1,3,COleVariant(CXhChar16("TMA")));
			index+=2;
		}
	}
	else if(iSheet==2)
	{	//�ڶ��ֽ����DWG�ļ����������ݣ���������û������
		excel_sheet.SetName("DWG��ר�й���");
		for(COMPARE_PART_RESULT *pResult=EnumFirstResult();pResult;pResult = EnumNextResult())
		{
			if(pResult->pLoftPart || pResult->pOrgPart==NULL)
				continue;
			map.SetValueAt(index,0,COleVariant(pResult->pOrgPart->GetPartNo()));
			map.SetValueAt(index,1,COleVariant(pResult->pOrgPart->GetSpec(FALSE)));
			map.SetValueAt(index,2,COleVariant(CBomModel::QueryMatMarkIncQuality(pResult->pOrgPart)));
			index++;
		}
	}
	_snprintf(cell_end,15,"D%d",index+1);
	if(index>0)
		CExcelOper::SetRangeValue(excel_sheet,"A2",cell_end,map.var);
	CExcelOper::SetRangeHorizontalAlignment(excel_sheet,"A1",cell_end,COleVariant((long)3));
	CExcelOper::SetRangeBorders(excel_sheet,"A1",cell_end,COleVariant(10.5));
}
void CProjectTowerType::AddDwgLackPartSheet(LPDISPATCH pSheet)
{
	_Worksheet excel_sheet;
	excel_sheet.AttachDispatch(pSheet,FALSE);
	excel_sheet.Select();
	excel_sheet.SetName("TMA��ר�й���");
	//���ñ���
	CStringArray str_arr;
	str_arr.SetSize(6);
	str_arr[0]="�������";str_arr[1]="��ƹ��";str_arr[2]="����";
	str_arr[3]="����";str_arr[4]="������";str_arr[5]="�ӹ���";
	double col_arr[6]={15,15,15,15,15,15};
	CExcelOper::AddRowToExcelSheet(excel_sheet,1,str_arr,col_arr,TRUE);
	//�������
	char cell_start[16]="A1";
	char cell_end[16]="A1";
	int nResult=GetResultCount();
	CVariant2dArray map(nResult*2,6);//��ȡExcel���ķ�Χ
	int index=0;
	for(COMPARE_PART_RESULT *pResult=EnumFirstResult();pResult;pResult=EnumNextResult())
	{
		if(pResult==NULL || pResult->pLoftPart==NULL || pResult->pOrgPart)
			continue;
		map.SetValueAt(index,0,COleVariant(pResult->pLoftPart->GetPartNo()));
		map.SetValueAt(index,1,COleVariant(pResult->pLoftPart->GetSpec(FALSE)));
		map.SetValueAt(index,2,COleVariant(CBomModel::QueryMatMarkIncQuality(pResult->pLoftPart)));
		map.SetValueAt(index,3,COleVariant(CXhChar50("%d",pResult->pLoftPart->m_wLength)));
		map.SetValueAt(index,4,COleVariant((long)pResult->pLoftPart->m_nDanJiNum));
		map.SetValueAt(index,5,COleVariant((long)pResult->pLoftPart->feature));
		index++;
	}
	_snprintf(cell_end,15,"F%d",index+1);
	if(index>0)
		CExcelOper::SetRangeValue(excel_sheet,"A2",cell_end,map.var);
	CExcelOper::SetRangeHorizontalAlignment(excel_sheet,"A1",cell_end,COleVariant((long)3));
	CExcelOper::SetRangeBorders(excel_sheet,"A1",cell_end,COleVariant(10.5));
}
//�����ȽϽ��
void CProjectTowerType::ExportCompareResult(int iCompareType)
{
	int nSheet=2;
	if(iCompareType==COMPARE_BOM_FILE)
		nSheet=3;
	else if(iCompareType==COMPARE_ANGLE_DWGS || 
		iCompareType==COMPARE_PLATE_DWGS)
		nSheet=1;
	LPDISPATCH pWorksheets=CExcelOper::CreateExcelWorksheets(nSheet);
	ASSERT(pWorksheets!= NULL);
	Sheets excel_sheets;
	excel_sheets.AttachDispatch(pWorksheets);
	for(int iSheet=1;iSheet<=nSheet;iSheet++)
	{
		LPDISPATCH pWorksheet=excel_sheets.GetItem(COleVariant((short)iSheet));
		if(iCompareType==COMPARE_BOM_FILE)			//�ϵ��ԱȽ��		
			AddBomResultSheet(pWorksheet,iSheet);
		else if(iCompareType==COMPARE_ANGLE_DWG)	//�Ǹ�DWG�ԱȽ��
			AddAngleResultSheet(pWorksheet,iSheet);
		else if(iCompareType==COMPARE_PLATE_DWG)	//�ְ�DWG�ԱȽ��
			AddPlateResultSheet(pWorksheet,iSheet);
		else //if(iCompareType==COMPARE_ANGLE_DWGS)
			AddDwgLackPartSheet(pWorksheet);
	}
	excel_sheets.ReleaseDispatch();
}
//����ERP�ϵ��м��ţ�����ΪQ420,����ǰ��P������ΪQ345������ǰ��H��
BOOL CProjectTowerType::ModifyErpBomPartNo(BYTE ciMatCharPosType)
{
	if (m_xOrigBom.GetFileName().GetLength() <= 0)
		return FALSE;
	CExcelOperObject excelobj;
	if (!excelobj.OpenExcelFile(m_xOrigBom.GetFileName()))
		return FALSE;
	LPDISPATCH pWorksheets = excelobj.GetWorksheets();
	if(pWorksheets==NULL)
	{
		logerr.Log("ERP�ϵ��ļ���ʧ��!");
		return FALSE;
	}
	//��ȡExcelָ��Sheet���ݴ洢��sheetContentMap��
	ASSERT(pWorksheets != NULL);
	Sheets       excel_sheets;
	excel_sheets.AttachDispatch(pWorksheets);
	LPDISPATCH pWorksheet = excel_sheets.GetItem(COleVariant((short) 1));
	_Worksheet   excel_sheet;
	excel_sheet.AttachDispatch(pWorksheet);
	excel_sheet.Select();
	Range excel_usedRange,excel_range;
	excel_usedRange.AttachDispatch(excel_sheet.GetUsedRange());
	excel_range.AttachDispatch(excel_usedRange.GetRows());
	long nRowNum = excel_range.GetCount();
	excel_range.AttachDispatch(excel_usedRange.GetColumns());
	long nColNum = excel_range.GetCount();
	CVariant2dArray sheetContentMap(1,1);
	CXhChar50 cell=CExcelOper::GetCellPos(nColNum,nRowNum);
	LPDISPATCH pRange = excel_sheet.GetRange(COleVariant("A1"),COleVariant(cell));
	excel_range.AttachDispatch(pRange,FALSE);
	sheetContentMap.var=excel_range.GetValue();
	excel_usedRange.ReleaseDispatch();
	excel_range.ReleaseDispatch();
	//����ָ��sheet����
	int iPartNoCol=1,iMartCol=3;
	for(int i=1;i<=sheetContentMap.RowsCount();i++)
	{
		VARIANT value;
		CXhChar16 sPartNo,sMaterial,sNewPartNo;
		//����
		sheetContentMap.GetValueAt(i,iPartNoCol,value);
		if(value.vt==VT_EMPTY)
			continue;
		sPartNo=VariantToString(value);
		//����
		sheetContentMap.GetValueAt(i,iMartCol,value);
		sMaterial =VariantToString(value);
		//���¼�������
		if(strstr(sMaterial,"Q345") && strstr(sPartNo,"H")==NULL)
		{
			if(ciMatCharPosType==0)
				sNewPartNo.Printf("H%s",(char*)sPartNo);
			else if(ciMatCharPosType==1)
				sNewPartNo.Printf("%sH",(char*)sPartNo);
			CExcelOper::SetRangeValue(excel_sheet,iPartNoCol,i+1,sNewPartNo);
			m_xOrigBom.UpdateProcessPart(sPartNo,sNewPartNo);
		}
		if(strstr(sMaterial,"Q355") && strstr(sPartNo, "H") == NULL)
		{
			if (ciMatCharPosType == 0)
				sNewPartNo.Printf("H%s", (char*)sPartNo);
			else if (ciMatCharPosType == 1)
				sNewPartNo.Printf("%sH", (char*)sPartNo);
			CExcelOper::SetRangeValue(excel_sheet, iPartNoCol, i + 1, sNewPartNo);
			m_xOrigBom.UpdateProcessPart(sPartNo, sNewPartNo);
		}
		if(strstr(sMaterial,"Q420") && strstr(sPartNo,"P")==NULL)
		{	
			if(ciMatCharPosType==0)
				sNewPartNo.Printf("P%s",(char*)sPartNo);
			else if(ciMatCharPosType==1)
				sNewPartNo.Printf("%sP",(char*)sPartNo);
			CExcelOper::SetRangeValue(excel_sheet,iPartNoCol,i+1,sNewPartNo);
			m_xOrigBom.UpdateProcessPart(sPartNo,sNewPartNo);
		}
	}
	excel_sheet.ReleaseDispatch();
	excel_sheets.ReleaseDispatch();
	return TRUE;
}

BOOL CProjectTowerType::ModifyTmaBomPartNo(BYTE ciMatCharPosType)
{
	ARRAY_LIST<CProcessPart*> partPtrList;
	for (CProcessPart *pPart = m_xLoftBom.EnumFirstPart(); pPart; pPart = m_xLoftBom.EnumNextPart())
		partPtrList.append(pPart);
	char cMaterial;
	CXhChar16 sPartNo, sMaterial, sNewPartNo;
	for (int i = 0; i < partPtrList.GetSize(); i++)
	{
		sPartNo = partPtrList[i]->GetPartNo();
		cMaterial = partPtrList[i]->cMaterial;
		sMaterial = CProcessPart::QuerySteelMatMark(cMaterial);
		//���¼�������
		if (strstr(sMaterial, "Q345") && strstr(sPartNo, "H") == NULL)
		{
			if (ciMatCharPosType == 0)
				sNewPartNo.Printf("H%s", (char*)sPartNo);
			else if (ciMatCharPosType == 1)
				sNewPartNo.Printf("%sH", (char*)sPartNo);
			m_xLoftBom.UpdateProcessPart(sPartNo, sNewPartNo);
		}
		if (strstr(sMaterial, "Q355") && strstr(sPartNo, "H") == NULL)
		{
			if (ciMatCharPosType == 0)
				sNewPartNo.Printf("H%s", (char*)sPartNo);
			else if (ciMatCharPosType == 1)
				sNewPartNo.Printf("%sH", (char*)sPartNo);
			m_xLoftBom.UpdateProcessPart(sPartNo, sNewPartNo);
		}
		if (strstr(sMaterial, "Q420") && strstr(sPartNo, "P") == NULL)
		{
			if (ciMatCharPosType == 0)
				sNewPartNo.Printf("P%s", (char*)sPartNo);
			else if (ciMatCharPosType == 1)
				sNewPartNo.Printf("%sP", (char*)sPartNo);
			m_xLoftBom.UpdateProcessPart(sPartNo, sNewPartNo);
		}
	}
	return TRUE;
}
CPlateProcessInfo *CProjectTowerType::FindPlateInfoByPartNo(const char* sPartNo)
{
	CPlateProcessInfo *pPlateInfo = NULL;
	for (CDwgFileInfo *pDwgFile = dwgFileList.GetFirst(); pDwgFile; pDwgFile = dwgFileList.GetNext())
	{
		if (pDwgFile->IsJgDwgInfo())
			continue;
		pPlateInfo=pDwgFile->FindPlateByPartNo(sPartNo);
		if (pPlateInfo)
			break;
	}
	return pPlateInfo;
}
CAngleProcessInfo *CProjectTowerType::FindAngleInfoByPartNo(const char* sPartNo)
{
	CAngleProcessInfo *pAngleInfo = NULL;
	for (CDwgFileInfo *pDwgFile = dwgFileList.GetFirst(); pDwgFile; pDwgFile = dwgFileList.GetNext())
	{
		if (!pDwgFile->IsJgDwgInfo())
			continue;
		pAngleInfo = pDwgFile->FindAngleByPartNo(sPartNo);
		if (pAngleInfo)
			break;
	}
	return pAngleInfo;
}
//////////////////////////////////////////////////////////////////////////
//CBomModel
CBomModel::CBomModel(void)
{
	
}
CBomModel::~CBomModel(void)
{
	
}
CDwgFileInfo *CBomModel::FindDwgFile(const char* file_path)
{
	CDwgFileInfo *pDwgFile = NULL;
	for (CProjectTowerType *pPrjTowerType = m_xPrjTowerTypeList.GetFirst(); pPrjTowerType; pPrjTowerType = m_xPrjTowerTypeList.GetNext())
	{
		pDwgFile = pPrjTowerType->FindDwgBomInfo(file_path);
		if (pDwgFile)
			break;
	}
	return pDwgFile;
}
CXhChar16 CBomModel::QueryMatMarkIncQuality(CProcessPart *pPart)
{
	CXhChar16 sMatMark;
	if (pPart)
	{
		sMatMark = CProcessPart::QuerySteelMatMark(pPart->cMaterial, pPart->m_sRelatePartNo);
		if (pPart->cQuality != 0 && !sMatMark.EqualNoCase(pPart->m_sRelatePartNo))
			sMatMark.Append(pPart->cQuality);
	}
	return sMatMark;
}
void CBomModel::SendCommandToCad(CString sCmd)
{
	if(strlen(sCmd)<=0)
		return;
	COPYDATASTRUCT cmd_msg;
	cmd_msg.dwData=(DWORD)1;
	cmd_msg.cbData=(DWORD)_tcslen(sCmd)+1;
	cmd_msg.lpData=sCmd.GetBuffer(sCmd.GetLength()+1);
	SendMessage(adsw_acadMainWnd(),WM_COPYDATA,(WPARAM)adsw_acadMainWnd(),(LPARAM)&cmd_msg);
}
#endif