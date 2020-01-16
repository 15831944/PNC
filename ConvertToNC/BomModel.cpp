#include "StdAfx.h"
#include "BomModel.h"
#include "ExcelOper.h"
#include "TblDef.h"
#include "DefCard.h"
#include "ArrayList.h"
#include "SortFunc.h"
#include "CadToolFunc.h"
#include "ComparePartNoString.h"

#if defined(__UBOM_) || defined(__UBOM_ONLY_)
CBomModel g_xUbomModel;

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
static BOMPART* CreateBomPart(int idClsType, const char* key, void* pContext)
{
	BOMPART *pPart = NULL;
	switch (idClsType) {
	case BOMPART::PLATE:
		pPart = new PART_PLATE();
		break;
	case BOMPART::ANGLE:
		pPart = new PART_ANGLE();
		break;
	case BOMPART::TUBE:
		pPart = new PART_TUBE;
		break;
	case BOMPART::SLOT:
		pPart = new PART_SLOT;
		break;
	default:
		pPart = new BOMPART();
		break;
	}
	return pPart;
}
static BOOL DeleteBomPart(BOMPART *pPart)
{
	if (pPart == NULL)
		return FALSE;
	switch (pPart->cPartType) {
	case BOMPART::PLATE:
		delete (PART_PLATE*)pPart;
		break;
	case BOMPART::ANGLE:
		delete (PART_ANGLE*)pPart;
		break;
	case BOMPART::TUBE:
		delete (PART_TUBE*)pPart;
		break;
	case BOMPART::SLOT:
		delete (PART_SLOT*)pPart;
		break;
	default:
		delete pPart;
		break;
	}
	return TRUE;
}
int compare_func(const CXhChar16& str1,const CXhChar16& str2)
{
	CString keyStr1(str1),keyStr2(str2);
	return ComparePartNoString(keyStr1,keyStr2);
}
//////////////////////////////////////////////////////////////////////////
//CBomFile
CBomFile::CBomFile()
{
	m_hashPartByPartNo.CreateNewAtom = CreateBomPart;
	m_hashPartByPartNo.DeleteAtom = DeleteBomPart;
	m_pProject=NULL;
}
CBomFile::~CBomFile()
{

}
//
void CBomFile::UpdateProcessPart(const char* sOldKey,const char* sNewKey)
{
	BOMPART* pPart=m_hashPartByPartNo.GetValue(sOldKey);
	if(pPart==NULL)
	{
		logerr.Log("%s�����Ҳ���",sOldKey);
		return;
	}
	pPart=m_hashPartByPartNo.ModifyKeyStr(sOldKey,sNewKey);
	if (pPart)
		pPart->sPartNo.Copy(sNewKey);
}
//����BOMSHEET����
BOOL CBomFile::ParseSheetContent(CVariant2dArray &sheetContentMap,CHashStrList<DWORD>& hashColIndex,int iStartRow)
{
	if (sheetContentMap.RowsCount() < 1)
		return FALSE;
	CLogErrorLife logLife;
	//1���ж����Ƿ�����Ҫ��
	BOOL bHasColIndex = (hashColIndex.GetNodeNum() > 2) ? TRUE : FALSE;
	BOOL bHasPartNo = FALSE, bHasMet = FALSE, bHasSpec = FALSE, bValid = FALSE;
	int iContentRow = 0, nColNum = sheetContentMap.ColsCount();
	for (int iRow = 0; iRow < 10; iRow++)
	{
		for (int iCol = 0; iCol < nColNum; iCol++)
		{
			VARIANT value;
			sheetContentMap.GetValueAt(iRow, iCol, value);
			if (value.vt == VT_EMPTY)
				continue;
			CString str(value.bstrVal);
			str.Remove('\n');
			if (CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_PART_NO, str))
			{
				bHasPartNo = TRUE;
				CXhChar16 sKey(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::INDEX_PART_NO));
				if (!bHasColIndex)
					hashColIndex.SetValue(sKey, iCol);
			}
			else if (CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_METERIAL, str))
			{
				bHasMet = TRUE;
				CXhChar16 sKey(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::INDEX_METERIAL));
				if (!bHasColIndex)
					hashColIndex.SetValue(sKey, iCol);
			}
			else if (CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_SPEC, str))
			{
				bHasSpec = TRUE;
				CXhChar16 sKey(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::INDEX_SPEC));
				if (!bHasColIndex)
					hashColIndex.SetValue(sKey, iCol);
			}
			else if (CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_LEN, str))
			{
				CXhChar16 sKey(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::INDEX_LEN));
				if (!bHasColIndex)
					hashColIndex.SetValue(sKey, iCol);
			}
			else if (CBomTblTitleCfg::IsMatchTitle(CBomTblTitleCfg::INDEX_SING_NUM, str))
			{
				CXhChar16 sKey(CBomTblTitleCfg::GetColName(CBomTblTitleCfg::INDEX_SING_NUM));
				if (!bHasColIndex)
					hashColIndex.SetValue(sKey, iCol);
			}
			if(bHasPartNo && bHasMet && bHasSpec)
			{
				bValid = TRUE;
				break;
			}
		}
		if (bValid)
		{
			iContentRow = iRow + 1;
			break;
		}
	}
	if (!bValid)
		return FALSE;
	//2.��ȡExcel���е�Ԫ���ֵ
	if (iStartRow <= 0)
		iStartRow = iContentRow;
	for(int i=iStartRow;i<= sheetContentMap.RowsCount();i++)
	{
		VARIANT value;
		int nSingleNum = 0, nProcessNum = 0;
		double fLength = 0, fWeight = 0, fSumWeight = 0;
		CXhChar100 sPartNo, sMaterial, sSpec, sNote, sReplaceSpec, sValue;
		BOOL bCutAngle = FALSE, bCutRoot = FALSE, bCutBer = FALSE, bPushFlat = FALSE;
		BOOL bKaiJiao = FALSE, bHeJiao = FALSE, bWeld = FALSE, bZhiWan = FALSE;
		//����
		DWORD *pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::T_PART_NO);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		if(value.vt==VT_EMPTY)
			continue;
		sPartNo=VariantToString(value);
		if(sPartNo.GetLength()<2)
			continue;
		//����
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_METERIAL);
		sheetContentMap.GetValueAt(i, *pColIndex, value);
		sMaterial = VariantToString(value);
		//���
		int cls_id=0;
		pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::T_SPEC);
		sheetContentMap.GetValueAt(i,*pColIndex,value);
		sSpec=VariantToString(value);
		if (strstr(sSpec, "L") || strstr(sSpec, "��"))
		{
			cls_id = BOMPART::ANGLE;
			if (strstr(sSpec, "��"))
				sSpec.Replace("��", "L");
			if (strstr(sSpec, "��"))
				sSpec.Replace("��", "*");
			if (strstr(sSpec, "x"))
				sSpec.Replace("x", "*");
			if (strstr(sSpec, "X"))
				sSpec.Replace("X", "*");
		}
		else if (strstr(sSpec, "-"))
		{
			cls_id = BOMPART::PLATE;
			if (strstr(sSpec, "x")|| strstr(sSpec, "X"))
			{
				char *skey = strtok((char*)sSpec, "x,X");
				sSpec.Copy(skey);
			}
		}
		else //if(strstr(sSpec,"��"))
			cls_id = BOMPART::TUBE;
		//����
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_PARTTYPE);
		if (pColIndex != NULL)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			CXhChar100 sPartType = VariantToString(value);
			if (strstr(sPartType, "L") || strstr(sPartType, "��") || strstr(sPartType, "�Ǹ�"))
				cls_id = BOMPART::ANGLE;
			else if (strstr(sPartType, "-") || strstr(sPartType, "�ְ�"))
				cls_id = BOMPART::PLATE;
			else
				cls_id = BOMPART::TUBE;
			//
			if (cls_id == BOMPART::ANGLE && strstr(sSpec, "L") == NULL)
			{
				sSpec.InsertBefore('L');
				if (strstr(sSpec, "��"))
					sSpec.Replace("��", "*");
				if (strstr(sSpec, "x"))
					sSpec.Replace("x", "*");
			}
			if (cls_id == BOMPART::PLATE && strstr(sSpec, "-") == NULL)
				sSpec.InsertBefore('-');
		}
		//���ù��
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_REPLACE_SPEC);
		if (pColIndex != NULL)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sReplaceSpec = VariantToString(value);
		}
		//����
		pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::T_LEN);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			fLength = (float)atof(VariantToString(value));
		}
		//������
		pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::T_SING_NUM);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			nSingleNum = atoi(VariantToString(value));
		}
		//�ӹ���
		pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::T_MANU_NUM);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			nProcessNum = atoi(VariantToString(value));
		}
		//��������
		pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::T_SING_WEIGHT);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			fWeight = atof(VariantToString(value));
		}
		//�ӹ�����
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_MANU_WEIGHT);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			fSumWeight = atof(VariantToString(value));
		}
		//��ע
		pColIndex= hashColIndex.GetValue(CBomTblTitleCfg::T_NOTES);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sNote = VariantToString(value);
		}
		//����
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_WELD);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (sValue.Equal("*"))
				bWeld = TRUE;
		}
		//����
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_ZHI_WAN);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (sValue.Equal("*"))
				bZhiWan = TRUE;
		}
		//�н�
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_CUT_ANGLE);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (sValue.Equal("*"))
				bCutAngle = TRUE;
		}
		//ѹ��
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_PUSH_FLAT);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (sValue.Equal("*"))
				bPushFlat = TRUE;
		}
		//�ٸ�
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_CUT_ROOT);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (sValue.Equal("*"))
				bCutRoot = TRUE;
		}
		//����
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_CUT_BER);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (sValue.Equal("*"))
				bCutBer = TRUE;
		}
		//����
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_KAI_JIAO);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (sValue.Equal("*"))
				bKaiJiao = TRUE;
		}
		//�Ͻ�
		pColIndex = hashColIndex.GetValue(CBomTblTitleCfg::T_HE_JIAO);
		if (pColIndex)
		{
			sheetContentMap.GetValueAt(i, *pColIndex, value);
			sValue = VariantToString(value);
			if (sValue.Equal("*"))
				bHeJiao = TRUE;
		}
		//����ϣ��
		BOMPART* pBomPart = NULL;
		if (pBomPart =m_hashPartByPartNo.GetValue(sPartNo))
			logerr.Log("�����ظ����ţ�%s", (char*)sPartNo);
		int nWidth = 0, nThick = 0;
		CProcessPart::RestoreSpec(sSpec, &nWidth, &nThick);
		if (pBomPart == NULL)
		{
			pBomPart = m_hashPartByPartNo.Add(sPartNo, cls_id);
			pBomPart->SetPartNum(nSingleNum);
			pBomPart->feature1 = nProcessNum;
			pBomPart->sPartNo.Copy(sPartNo);
			pBomPart->sSpec.Copy(sSpec);
			pBomPart->sMaterial = sMaterial;
			pBomPart->cMaterial = CProcessPart::QueryBriefMatMark(sMaterial);
			if(g_xUbomModel.m_uiCustomizeSerial!=CBomModel::ID_JiangSu_HuaDian)
				pBomPart->cQualityLevel = CProcessPart::QueryBriefQuality(sMaterial);
			pBomPart->length = fLength;
			pBomPart->fPieceWeight = fWeight;
			pBomPart->fSumWeight = fSumWeight;
			pBomPart->wide = (double)nWidth;
			pBomPart->thick = (double)nThick;
			strcpy(pBomPart->sNotes, sNote);
			if (bZhiWan)
				pBomPart->siZhiWan = 1;
			if (cls_id == BOMPART::ANGLE)
			{
				PART_ANGLE* pBomJg = (PART_ANGLE*)pBomPart;
				pBomJg->bCutAngle = bCutAngle;
				pBomJg->bCutRoot = bCutRoot;
				pBomJg->bCutBer = bCutBer;
				pBomJg->bKaiJiao = bKaiJiao;
				pBomJg->bHeJiao = bHeJiao;
				pBomJg->bWeldPart = bWeld;
				if (strstr(pBomPart->sNotes, "����"))
					pBomJg->bKaiJiao = TRUE;
				if (strstr(pBomPart->sNotes, "�Ͻ�"))
					pBomJg->bHeJiao = TRUE;
				if (strstr(pBomJg->sNotes, "�Ŷ�"))
					pBomJg->bHasFootNail = TRUE;
				if (bPushFlat)
					pBomJg->nPushFlat = 1;
			}
			else if (cls_id == BOMPART::PLATE)
			{
				PART_PLATE* pBomPlate = (PART_PLATE*)pBomPart;
				pBomPlate->bWeldPart = bWeld;
			}
		}
		else
		{	//�����ظ����ӹ������ۼӼ��� wht 19-09-15
			pBomPart->feature1 += nProcessNum;
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
	int nSheetNum = excelobj.GetWorkSheetCount();
	//��ȡ��������Ϣ
	CHashStrList<DWORD> hashColIndexByColTitle;
	g_xUbomModel.m_xTmaTblCfg.GetHashColIndexByColTitleTbl(hashColIndexByColTitle);
	int iStartRow = g_xUbomModel.m_xTmaTblCfg.m_nStartRow-1;
	//��������
	int nValidSheetCount = 0;
	m_hashPartByPartNo.Empty();
	int iAngleSheet = CExcelOper::GetExcelIndexOfSpecifySheet(&excelobj, "�Ǹ�����");
	BOOL bRetCode = FALSE;
	if (iAngleSheet > 0)
	{	//���ȶ�ȡ���Ǹ����ϡ��������յ�װ��ʽ��wht 19-09-15
		CVariant2dArray sheetContentMap(1, 1);
		CExcelOper::GetExcelContentOfSpecifySheet(&excelobj, sheetContentMap, iAngleSheet);
		if (ParseSheetContent(sheetContentMap,hashColIndexByColTitle,iStartRow))
		{
			nValidSheetCount++;
			bRetCode = TRUE;
		}
	}
	if (!bRetCode)
	{
		for (int iSheet = 1; iSheet <= nSheetNum; iSheet++)
		{	//2����������
			CVariant2dArray sheetContentMap(1, 1);
			CExcelOper::GetExcelContentOfSpecifySheet(&excelobj, sheetContentMap, iSheet);
			if (ParseSheetContent(sheetContentMap,hashColIndexByColTitle,iStartRow))
				nValidSheetCount++;
		}
	}
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
	//��ȡExcel���ݴ洢��sheetContentMap��,�����б�����������ӳ���hashColIndexByColTitle
	CVariant2dArray sheetContentMap(1,1);
	//���֧��52�У��������������ʱ����������ñ���ɫ�ᵼ���Զ���ȡ�����������󣬼���Excel�ٶ��� wht 19-12-30
	if(!CExcelOper::GetExcelContentOfSpecifySheet(m_sFileName,sheetContentMap,1,52))
	//if (!CExcelOper::GetExcelContentOfSpecifySheet(&excelobj, sheetContentMap, 1,52))
		return false;
	//��ȡ��������Ϣ
	CHashStrList<DWORD> hashColIndexByColTitle;
	g_xUbomModel.m_xErpTblCfg.GetHashColIndexByColTitleTbl(hashColIndexByColTitle);
	int iStartRow = g_xUbomModel.m_xErpTblCfg.m_nStartRow-1;
	//��������
	m_hashPartByPartNo.Empty();
	return ParseSheetContent(sheetContentMap, hashColIndexByColTitle, iStartRow);
}

CString CBomFile::GetPartNumStr()
{
	int nSumNum = m_hashPartByPartNo.GetNodeNum();
	int nJgNum = 0, nPlateNum = 0, nOtherNum = 0;
	for (BOMPART *pPart = m_hashPartByPartNo.GetFirst(); pPart; pPart = m_hashPartByPartNo.GetNext())
	{
		if (pPart->cPartType==BOMPART::ANGLE)
			nJgNum++;
		else if (pPart->cPartType==BOMPART::PLATE)
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
	file.WriteString(m_xLoftBom.m_sFileName);	//TMA�ϵ�
	file.WriteString(m_xOrigBom.m_sFileName);	//ERP�ϵ�
	//DWG�ļ���Ϣ
	DWORD n=dwgFileList.GetNodeNum();
	file.Write(&n,sizeof(DWORD));
	for(CDwgFileInfo* pDwgInfo=dwgFileList.GetFirst();pDwgInfo;pDwgInfo=dwgFileList.GetNext())
	{
		file.WriteString(pDwgInfo->m_sFileName);
		int ibValue=0;
		if(pDwgInfo->IsJgDwgInfo())
			ibValue=1;
		file.Write(&ibValue,sizeof(int));
	}
}
BOOL CProjectTowerType::IsTmaBomFile(const char* sFilePath)
{
	if (g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_AnHui_HongYuan)
	{	//���պ�Դ�ϵ��ļ��к��йؼ���
		char sFileName[MAX_PATH] = "";
		_splitpath(sFilePath, NULL, NULL, sFileName, NULL);
		if (strstr(sFileName, "TMA") || strstr(sFileName, "tma"))
			return TRUE;
		else
			return FALSE;
	}
	else
	{	//���ݶ����н���ʶ��
		if (m_xLoftBom.GetPartNum() > 0)
			return FALSE;	//���������Ѷ�ȡ
		if (g_xUbomModel.m_xTmaTblCfg.m_sColIndexArr.GetLength() <= 0)
		{
			logerr.Log("û�ж���BOM������,��ȡʧ��!");
			return FALSE;
		}
		CHashStrList<DWORD> hashColIndex;
		g_xUbomModel.m_xTmaTblCfg.GetHashColIndexByColTitleTbl(hashColIndex);
		CVariant2dArray sheetContentMap(1, 1);
		if (!CExcelOper::GetExcelContentOfSpecifySheet(sFilePath, sheetContentMap, 1))
			return FALSE;
		BOOL bValid = FALSE;
		int nColNum = hashColIndex.GetNodeNum();
		for (int iCol = 0; iCol < nColNum; iCol++)
		{
			CXhChar50 sCol(CBomTblTitleCfg::GetColName(iCol));
			DWORD* pColIndex = hashColIndex.GetValue(sCol);
			if(pColIndex==NULL)
				continue;
			BOOL bValid = FALSE;
			for (int iRow = 0; iRow < 10; iRow++)
			{
				VARIANT value;
				sheetContentMap.GetValueAt(iRow, *pColIndex, value);
				if (value.vt == VT_EMPTY)
					continue;
				CString str(value.bstrVal);
				str.Remove('\n');
				if (!bValid && CBomTblTitleCfg::IsMatchTitle(iCol, str))
				{
					bValid = TRUE;
					break;
				}
			}
			if (!bValid)
				return FALSE;
		}
	}
	return TRUE;
}
BOOL CProjectTowerType::IsErpBomFile(const char* sFilePath)
{
	if (g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_AnHui_HongYuan)
	{
		char sFileName[MAX_PATH] = "";
		_splitpath(sFilePath, NULL, NULL, sFileName, NULL);
		if (strstr(sFileName, "ERP") || strstr(sFileName, "erp"))
			return TRUE;	//���պ�Դ�ϵ��ļ��к��йؼ���
		else
			return FALSE;
	}
	else
	{	//���ݶ����н���ʶ��
		if (m_xOrigBom.GetPartNum() > 0)
			return FALSE;	//���������Ѷ�ȡ
		if (g_xUbomModel.m_xErpTblCfg.m_sColIndexArr.GetLength() <= 0)
		{
			logerr.Log("û�ж���BOM������,��ȡʧ��!");
			return FALSE;
		}
		CHashStrList<DWORD> hashColIndex;
		g_xUbomModel.m_xErpTblCfg.GetHashColIndexByColTitleTbl(hashColIndex);
		CVariant2dArray sheetContentMap(1, 1);
		if (!CExcelOper::GetExcelContentOfSpecifySheet(sFilePath, sheetContentMap, 1))
			return FALSE;
		BOOL bValid = FALSE;
		int nColNum = hashColIndex.GetNodeNum();
		for (int iCol = 0; iCol < nColNum; iCol++)
		{
			CXhChar50 sCol(CBomTblTitleCfg::GetColName(iCol));
			DWORD* pColIndex = hashColIndex.GetValue(sCol);
			if (pColIndex == NULL)
				continue;
			BOOL bValid = FALSE;
			for (int iRow = 0; iRow < 10; iRow++)
			{
				VARIANT value;
				sheetContentMap.GetValueAt(iRow, *pColIndex, value);
				if (value.vt == VT_EMPTY)
					continue;
				CString str(value.bstrVal);
				if (!bValid && CBomTblTitleCfg::IsMatchTitle(iCol, str))
				{
					bValid = TRUE;
					break;
				}
			}
			if (!bValid)
				return FALSE;
		}
	}
	return TRUE;
}
//��ʼ��BOM��Ϣ
void CProjectTowerType::InitBomInfo(const char* sFileName,BOOL bLoftBom)
{
	CXhChar100 sName;
	_splitpath(sFileName, NULL, NULL, sName, NULL);
	if(bLoftBom)
	{
		m_xLoftBom.m_sBomName.Copy(sName);
		m_xLoftBom.m_sFileName.Copy(sFileName);
		m_xLoftBom.SetBelongModel(this);
		m_xLoftBom.ImportTmaExcelFile();
	}
	else
	{	
		m_xOrigBom.m_sBomName.Copy(sName);
		m_xOrigBom.m_sFileName.Copy(sFileName);
		m_xOrigBom.SetBelongModel(this);
		m_xOrigBom.ImportErpExcelFile();
	}
}
//��ӽǸ�DWGBOM��Ϣ
CDwgFileInfo* CProjectTowerType::AppendDwgBomInfo(const char* sFileName, BOOL bJgDxf)
{
	if (strlen(sFileName) <= 0)
		return FALSE;
	//��DWG�ļ�
	CXhChar500 file_path;
	AcApDocument *pDoc = NULL;
	AcApDocumentIterator *pIter = acDocManager->newAcApDocumentIterator();
	for (; !pIter->done(); pIter->step())
	{
		pDoc = pIter->document();
#ifdef _ARX_2007
		file_path.Copy(_bstr_t(pDoc->fileName()));
#else
		file_path.Copy(pDoc->fileName());
#endif
		if (strstr(file_path, sFileName))
			break;
	}
	if (strstr(file_path, sFileName))	//����ָ���ļ�
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
	CDwgFileInfo* pDwgFile = FindDwgBomInfo(sFileName);
	if (pDwgFile)
	{
		pDwgFile->ExtractDwgInfo(sFileName, bJgDxf);
		return pDwgFile;
	}
	else
	{
		pDwgFile = dwgFileList.append();
		pDwgFile->SetBelongModel(this);
		//��ȡDWG��Ϣ
		pDwgFile->ExtractDwgInfo(sFileName, bJgDxf);
		return pDwgFile;
	}
}
//
CDwgFileInfo* CProjectTowerType::FindDwgBomInfo(const char* sFileName)
{
	CDwgFileInfo* pDwgInfo = NULL;
	for (pDwgInfo = dwgFileList.GetFirst(); pDwgInfo; pDwgInfo = dwgFileList.GetNext())
	{
		if (strcmp(pDwgInfo->m_sFileName, sFileName) == 0)
			break;
	}
	return pDwgInfo;
}
//
void CProjectTowerType::CompareData(BOMPART* pSrcPart, BOMPART* pDesPart, CHashStrList<BOOL> &hashBoolByPropName)
{
	//��ȡ��������Ϣ
	CHashStrList<DWORD> hashColIndex;
	g_xUbomModel.m_xTmaTblCfg.GetHashColIndexByColTitleTbl(hashColIndex);
	//
	hashBoolByPropName.Empty();
	if(pSrcPart->cPartType!=pDesPart->cPartType||
		stricmp(pSrcPart->sSpec, pDesPart->sSpec) != 0)
		hashBoolByPropName.SetValue("spec", TRUE);
	if (pSrcPart->cMaterial != pDesPart->cMaterial)
		hashBoolByPropName.SetValue("cMaterial", TRUE);
	if (pSrcPart->cMaterial == 'A' && !pSrcPart->sMaterial.Equal(pDesPart->sMaterial))
		hashBoolByPropName.SetValue("cMaterial", TRUE);
	if (pSrcPart->cQualityLevel != pDesPart->cQualityLevel)
		hashBoolByPropName.SetValue("cMaterial", TRUE);
	if (hashColIndex.GetValue(CBomTblTitleCfg::T_SING_NUM) &&
		pSrcPart->GetPartNum() != pDesPart->GetPartNum())
		hashBoolByPropName.SetValue("nSingNum", TRUE);
	if (hashColIndex.GetValue(CBomTblTitleCfg::T_MANU_NUM) &&
		pSrcPart->feature1 != pDesPart->feature1)
	{	//���պ�Դ���Ƚϼӹ���������Ҫ�����ӹ���
		if (g_xUbomModel.m_uiCustomizeSerial != CBomModel::ID_AnHui_HongYuan)
			hashBoolByPropName.SetValue("nManuNum", TRUE);
	}
	if(hashColIndex.GetValue(CBomTblTitleCfg::T_MANU_WEIGHT) &&
		fabs(pSrcPart->fSumWeight-pDesPart->fSumWeight)>0)
		hashBoolByPropName.SetValue("fSumWeight", TRUE);
	if (pSrcPart->cPartType == pDesPart->cPartType && pSrcPart->cPartType==BOMPART::ANGLE)
	{	//�Ǹ���Ϣ�Ա�
		PART_ANGLE* pSrcJg = (PART_ANGLE*)pSrcPart;
		PART_ANGLE* pDesJg = (PART_ANGLE*)pDesPart;
		if (hashColIndex.GetValue(CBomTblTitleCfg::T_LEN))
		{	//���ȶԱ�
			if(g_xUbomModel.m_uiCustomizeSerial==CBomModel::ID_JiangSu_HuaDian)
			{	//���ջ���Ҫ��ͼֽ���ȴ��ڷ�������
				if(pSrcPart->length > pDesPart->length)
					hashBoolByPropName.SetValue("fLength", TRUE);
			}
			else
			{	//�ж���ֵ�Ƿ����
				if (fabsl(pSrcPart->length - pDesPart->length) > 0)
					hashBoolByPropName.SetValue("fLength", TRUE);
			}
		}
		if(hashColIndex.GetValue(CBomTblTitleCfg::T_CUT_ANGLE) &&
			pSrcJg->bCutAngle!=pDesJg->bCutAngle)
			hashBoolByPropName.SetValue("CutAngle", TRUE);
		if(hashColIndex.GetValue(CBomTblTitleCfg::T_PUSH_FLAT) &&
			pSrcJg->nPushFlat!=pDesJg->nPushFlat)
			hashBoolByPropName.SetValue("PushFlat", TRUE);
		if(hashColIndex.GetValue(CBomTblTitleCfg::T_CUT_BER) &&
			pSrcJg->bCutBer!=pDesJg->bCutBer)
			hashBoolByPropName.SetValue("CutBer", TRUE);
		if(hashColIndex.GetValue(CBomTblTitleCfg::T_CUT_ROOT) &&
			pSrcJg->bCutRoot!=pDesJg->bCutRoot)
			hashBoolByPropName.SetValue("CutRoot", TRUE);
		if(hashColIndex.GetValue(CBomTblTitleCfg::T_KAI_JIAO) &&
			pSrcJg->bKaiJiao!=pDesJg->bKaiJiao)
			hashBoolByPropName.SetValue("KaiJiao", TRUE);
		if(hashColIndex.GetValue(CBomTblTitleCfg::T_HE_JIAO) &&
			pSrcJg->bHeJiao!=pDesJg->bHeJiao)
			hashBoolByPropName.SetValue("HeJiao", TRUE);
		if(hashColIndex.GetValue(CBomTblTitleCfg::T_NOTES) &&
			pSrcJg->bHasFootNail!=pDesJg->bHasFootNail)
			hashBoolByPropName.SetValue("FootNail", TRUE);
		if (hashColIndex.GetValue(CBomTblTitleCfg::T_WELD) &&
			pSrcPart->bWeldPart != pDesPart->bWeldPart)
			hashBoolByPropName.SetValue("Weld", TRUE);
		if (hashColIndex.GetValue(CBomTblTitleCfg::T_ZHI_WAN) &&
			pSrcPart->siZhiWan != pDesPart->siZhiWan)
			hashBoolByPropName.SetValue("ZhiWan", TRUE);
	}
	if (pSrcPart->cPartType == pDesPart->cPartType && pSrcPart->cPartType == BOMPART::ANGLE)
	{	//�ְ���Ϣ�Ա�
		if (hashColIndex.GetValue(CBomTblTitleCfg::T_WELD) &&
			pSrcPart->bWeldPart != pDesPart->bWeldPart)
			hashBoolByPropName.SetValue("Weld", TRUE);
		if (hashColIndex.GetValue(CBomTblTitleCfg::T_ZHI_WAN) &&
			pSrcPart->siZhiWan != pDesPart->siZhiWan)
			hashBoolByPropName.SetValue("ZhiWan", TRUE);
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
	for(BOMPART *pLoftPart=m_xLoftBom.EnumFirstPart();pLoftPart;pLoftPart=m_xLoftBom.EnumNextPart())
	{	
		BOMPART *pOrgPart = m_xOrigBom.FindPart(pLoftPart->sPartNo);
		if (pOrgPart == NULL)
		{	//1.���ڷ���������������ԭʼ����
			COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pLoftPart->sPartNo);
			pResult->pOrgPart=NULL;
			pResult->pLoftPart=pLoftPart;
		}
		else 
		{	//2. �Ա�ͬһ���Ź�������
			hashBoolByPropName.Empty();
			CompareData(pLoftPart, pOrgPart, hashBoolByPropName);
			if(hashBoolByPropName.GetNodeNum()>0)//�������
			{	
				COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pLoftPart->sPartNo);
				pResult->pOrgPart=pOrgPart;
				pResult->pLoftPart=pLoftPart;
				for(BOOL *pValue=hashBoolByPropName.GetFirst();pValue;pValue=hashBoolByPropName.GetNext())
					pResult->hashBoolByPropName.SetValue(hashBoolByPropName.GetCursorKey(),*pValue);
			}
		}
	}
	//2.3 ���������ԭʼ��,�����Ƿ���©�����
	for(BOMPART *pPart=m_xOrigBom.EnumFirstPart();pPart;pPart=m_xOrigBom.EnumNextPart())
	{
		if(m_xLoftBom.FindPart(pPart->sPartNo))
			continue;
		COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pPart->sPartNo);
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
			hashAngleByPartNo.SetValue(pJgInfo->m_xAngle.sPartNo,TRUE);
	}
	for(BOMPART *pPart=m_xLoftBom.EnumFirstPart();pPart;pPart=m_xLoftBom.EnumNextPart())
	{
		if(pPart->cPartType!=BOMPART::ANGLE)
			continue;
		if(hashAngleByPartNo.GetValue(pPart->sPartNo))
			continue;
		COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pPart->sPartNo);
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
	for(BOMPART *pPart=m_xLoftBom.EnumFirstPart();pPart;pPart=m_xLoftBom.EnumNextPart())
	{
		if(pPart->cPartType!=BOMPART::PLATE)
			continue;
		if(hashPlateByPartNo.GetValue(pPart->sPartNo))
			continue;
		COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pPart->sPartNo);
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
//
int CProjectTowerType::CompareLoftAndAngleDwg(const char* sFileName)
{
	const double COMPARE_EPS = 0.5;
	if (m_xLoftBom.GetPartNum() <= 0)
	{
		logerr.Log("ȱ�ٷ���BOM��Ϣ!");
		return 2;
	}
	CDwgFileInfo* pDwgFile = FindDwgBomInfo(sFileName);
	if (pDwgFile == NULL)
	{
		logerr.Log("δ�ҵ�ָ���ĽǸ�DWG�ļ�!");
		return 2;
	}
	//���бȶ�
	m_hashCompareResultByPartNo.Empty();
	CHashStrList<BOOL> hashBoolByPropName;
	CAngleProcessInfo* pJgInfo = NULL;
	for (pJgInfo = pDwgFile->EnumFirstJg(); pJgInfo; pJgInfo = pDwgFile->EnumNextJg())
	{
		BOMPART *pDwgJg = &(pJgInfo->m_xAngle);
		BOMPART *pLoftJg=m_xLoftBom.FindPart(pJgInfo->m_xAngle.sPartNo);
		if(pLoftJg==NULL)
		{	//1������DWG�����������ڷ�������
			COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pJgInfo->m_xAngle.sPartNo);
			pResult->pOrgPart = pDwgJg;
			pResult->pLoftPart = NULL;
		}
		else
		{	//2���Ա�ͬһ���Ź�������
			CompareData(pLoftJg, pDwgJg, hashBoolByPropName);
			if(hashBoolByPropName.GetNodeNum()>0)//�������
			{	
				COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pDwgJg->sPartNo);
				pResult->pOrgPart= pDwgJg;
				pResult->pLoftPart=pLoftJg;
				for(BOOL *pValue=hashBoolByPropName.GetFirst();pValue;pValue=hashBoolByPropName.GetNext())
					pResult->hashBoolByPropName.SetValue(hashBoolByPropName.GetCursorKey(),*pValue);
			}
		}
	}
	for (BOMPART *pPart = m_xLoftBom.EnumFirstPart(); pPart; pPart = m_xLoftBom.EnumNextPart())
	{
		if (pPart->cPartType!=BOMPART::ANGLE)
			continue;
		if (pDwgFile->FindAngleByPartNo(pPart->sPartNo))
			continue;
		//3������BOM������������DWG����
		COMPARE_PART_RESULT *pResult = m_hashCompareResultByPartNo.Add(pPart->sPartNo);
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
	CDwgFileInfo* pDwgFile=FindDwgBomInfo(sFileName);
	if(pDwgFile==NULL)
	{
		logerr.Log("δ�ҵ�ָ���ĸְ�DWG�ļ�!");
		return 2;
	}
	//�������ݱȶ�
	m_hashCompareResultByPartNo.Empty();
	CHashStrList<BOOL> hashBoolByPropName;
	CPlateProcessInfo* pPlateInfo = NULL;
	for (pPlateInfo = pDwgFile->EnumFirstPlate(); pPlateInfo; pPlateInfo = pDwgFile->EnumNextPlate())
	{
		PART_PLATE* pDwgPlate = &(pPlateInfo->xBomPlate);
		BOMPART *pLoftPlate=m_xLoftBom.FindPart(pDwgPlate->sPartNo);
		if(pLoftPlate==NULL)
		{	//1������DWG�����������ڷ�������
			COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pDwgPlate->sPartNo);
			pResult->pOrgPart = pDwgPlate;
			pResult->pLoftPart = NULL;
		}
		else
		{	//2���Ա�ͬһ���Ź�������
			CompareData(pLoftPlate, pDwgPlate, hashBoolByPropName);
			if(hashBoolByPropName.GetNodeNum()>0)//�������
			{	
				COMPARE_PART_RESULT *pResult=m_hashCompareResultByPartNo.Add(pDwgPlate->sPartNo);
				pResult->pOrgPart = pDwgPlate;
				pResult->pLoftPart = pLoftPlate;
				for(BOOL *pValue=hashBoolByPropName.GetFirst();pValue;pValue=hashBoolByPropName.GetNext())
					pResult->hashBoolByPropName.SetValue(hashBoolByPropName.GetCursorKey(),*pValue);
			}
		}
	}
	for (BOMPART *pPart = m_xLoftBom.EnumFirstPart(); pPart; pPart = m_xLoftBom.EnumNextPart())
	{
		if (pPart->cPartType != BOMPART::PLATE)
			continue;
		if(pDwgFile->FindPlateByPartNo(pPart->sPartNo))
			continue;
		//3������BOM������������DWG����
		COMPARE_PART_RESULT *pResult = m_hashCompareResultByPartNo.Add(pPart->sPartNo);
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
void CProjectTowerType::AddBomResultSheet(LPDISPATCH pSheet, ARRAY_LIST<CXhChar16>& keyStrArr)
{
	//���ñ���
	_Worksheet excel_sheet;
	excel_sheet.AttachDispatch(pSheet, FALSE);
	excel_sheet.Select();
	excel_sheet.SetName("У����");
	//�����ʾ��
	double col_arr[6] = { 10,10,10,10,10,20 };
	CStringArray str_arr;
	str_arr.SetSize(6);
	str_arr[0] = "�������"; str_arr[1] = "��ƹ��"; str_arr[2] = "����";
	str_arr[3] = "����"; str_arr[4] = "������"; str_arr[5] = "������Դ";
	CExcelOper::AddRowToExcelSheet(excel_sheet, 1, str_arr, col_arr, TRUE);
	//�������
	CXhChar100 sOrgBom = m_xOrigBom.m_sBomName;
	CXhChar100 sTmaBom = m_xLoftBom.m_sBomName;
	int index = 0, nCol = 6, nResult = GetResultCount();
	CVariant2dArray map(nResult * 2, nCol);
	for (int i = 0; i < keyStrArr.GetSize(); i++)
	{
		COMPARE_PART_RESULT *pResult = GetResult(keyStrArr[i]);
		if (pResult == NULL || pResult->pLoftPart == NULL || pResult->pOrgPart == NULL)
			continue;
		map.SetValueAt(index, 0, COleVariant(pResult->pOrgPart->sPartNo));
		map.SetValueAt(index, 1, COleVariant(pResult->pOrgPart->sSpec));
		map.SetValueAt(index, 2, COleVariant(CBomModel::QueryMatMarkIncQuality(pResult->pOrgPart)));
		map.SetValueAt(index, 3, COleVariant(CXhChar50("%.1f", pResult->pOrgPart->length)));
		map.SetValueAt(index, 4, COleVariant((long)pResult->pOrgPart->GetPartNum()));
		map.SetValueAt(index, 5, COleVariant(sOrgBom));
		//
		CExcelOper::MergeRowRange(excel_sheet, CXhChar16("A%d", index + 2), CXhChar16("A%d", index + 3));
		if (pResult->hashBoolByPropName.GetValue("spec"))
		{
			map.SetValueAt(index + 1, 1, COleVariant(pResult->pLoftPart->sSpec));
			CExcelOper::SetRangeBackColor(excel_sheet, 42, CXhChar16("B%d",index + 2));
			CExcelOper::SetRangeBackColor(excel_sheet, 44, CXhChar16("B%d", index + 3));
		}
		if (pResult->hashBoolByPropName.GetValue("cMaterial"))
		{
			map.SetValueAt(index + 1, 2, COleVariant(CBomModel::QueryMatMarkIncQuality(pResult->pLoftPart)));
			CExcelOper::SetRangeBackColor(excel_sheet, 42, CXhChar16("C%d", index + 2));
			CExcelOper::SetRangeBackColor(excel_sheet, 44, CXhChar16("C%d", index + 3));
		}
		if (pResult->hashBoolByPropName.GetValue("fLength"))
		{
			map.SetValueAt(index + 1, 3, COleVariant(CXhChar50("%.1f", pResult->pLoftPart->length)));
			CExcelOper::SetRangeBackColor(excel_sheet, 42, CXhChar16("D%d", index + 2));
			CExcelOper::SetRangeBackColor(excel_sheet, 44, CXhChar16("D%d", index + 3));
		}
		if (pResult->hashBoolByPropName.GetValue("nSingNum"))
		{
			map.SetValueAt(index + 1, 4, COleVariant((long)pResult->pLoftPart->GetPartNum()));
			CExcelOper::SetRangeBackColor(excel_sheet, 42, CXhChar16("E%d", index + 2));
			CExcelOper::SetRangeBackColor(excel_sheet, 44, CXhChar16("E%d", index + 3));
		}
		map.SetValueAt(index + 1, 5, COleVariant(sTmaBom));
		index += 2;
	}
	CXhChar16 sCellE("F%d", index + 1);
	CExcelOper::SetRangeValue(excel_sheet, "A2", sCellE, map.var);
	CExcelOper::SetRangeHorizontalAlignment(excel_sheet, "A1", sCellE, COleVariant((long)3));
	CExcelOper::SetRangeBorders(excel_sheet, "A1", sCellE, COleVariant(10.5));
}
void CProjectTowerType::AddAngleResultSheet(LPDISPATCH pSheet, ARRAY_LIST<CXhChar16>& keyStrArr)
{
	//���ñ���
	_Worksheet excel_sheet;
	excel_sheet.AttachDispatch(pSheet, FALSE);
	excel_sheet.Select();
	excel_sheet.SetName("У����");
	//�����ʾ��
	int index = 0, nCol = 0, nResult = GetResultCount();
	CStringArray str_arr;
	ARRAY_LIST<double> col_arr;
	if (g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_SiChuan_ChengDu)
	{
		nCol = 15;
		str_arr.SetSize(15);
		str_arr[0] = "����"; str_arr[1] = "���"; str_arr[2] = "����";
		str_arr[3] = "����"; str_arr[4] = "����"; str_arr[5] = "����";
		str_arr[6] = "����"; str_arr[7] = "�н�"; str_arr[8] = "���";
		str_arr[9] = "����"; str_arr[10] = "�ٽ�"; str_arr[11] = "����";
		str_arr[12] = "�Ͻ�"; str_arr[13] = "���Ŷ�"; str_arr[14] = "������Դ";
		col_arr.SetSize(15);
		for (int i = 0; i < 15; i++)
			col_arr[i] = 10;
	}
	else
	{
		nCol = 6;
		str_arr.SetSize(6);
		str_arr[0] = "�������"; str_arr[1] = "��ƹ��"; str_arr[2] = "����";
		str_arr[3] = "����"; str_arr[4] = "������"; str_arr[5] = "������Դ";
		col_arr.SetSize(6);
		for (int i = 0; i < 6; i++)
			col_arr[i] = 10;
	}
	CExcelOper::AddRowToExcelSheet(excel_sheet, 1, str_arr, col_arr.m_pData, TRUE);
	//�������
	CVariant2dArray map(nResult * 2, nCol);
	for (int i = 0; i < keyStrArr.GetSize(); i++)
	{
		COMPARE_PART_RESULT *pResult = GetResult(keyStrArr[i]);
		if (pResult == NULL || pResult->pLoftPart == NULL || pResult->pOrgPart == NULL)
			continue;
		map.SetValueAt(index, 0, COleVariant(pResult->pOrgPart->sPartNo));
		map.SetValueAt(index, 1, COleVariant(pResult->pOrgPart->sSpec));
		map.SetValueAt(index, 2, COleVariant(CBomModel::QueryMatMarkIncQuality(pResult->pOrgPart)));
		map.SetValueAt(index, 3, COleVariant(CXhChar50("%.0f", pResult->pOrgPart->length)));
		map.SetValueAt(index, 4, COleVariant((long)pResult->pOrgPart->GetPartNum()));
		map.SetValueAt(index, 5, COleVariant(CXhChar16("DWG")));
		if (g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_SiChuan_ChengDu)
		{
			PART_ANGLE* pOrgJg = (PART_ANGLE*)pResult->pOrgPart;
			map.SetValueAt(index, 5, pOrgJg->bWeldPart ? COleVariant("*") : COleVariant(""));
			map.SetValueAt(index, 6, pOrgJg->siZhiWan > 0 ? COleVariant("*") : COleVariant(""));
			map.SetValueAt(index, 7, pOrgJg->bCutAngle ? COleVariant("*") : COleVariant(""));
			map.SetValueAt(index, 8, pOrgJg->nPushFlat > 0 ? COleVariant("*") : COleVariant(""));
			map.SetValueAt(index, 9, pOrgJg->bCutBer ? COleVariant("*") : COleVariant(""));
			map.SetValueAt(index, 10, pOrgJg->bCutRoot ? COleVariant("*") : COleVariant(""));
			map.SetValueAt(index, 11, pOrgJg->bKaiJiao ? COleVariant("*") : COleVariant(""));
			map.SetValueAt(index, 12, pOrgJg->bHeJiao ? COleVariant("*") : COleVariant(""));
			map.SetValueAt(index, 13, pOrgJg->bHasFootNail ? COleVariant("*") : COleVariant(""));
			map.SetValueAt(index, 14, COleVariant(CXhChar16("DWG")));
		}
		//
		CExcelOper::MergeRowRange(excel_sheet, CXhChar16("A%d", index + 2), CXhChar16("A%d", index + 3));
		if (pResult->hashBoolByPropName.GetValue("spec"))
		{
			map.SetValueAt(index + 1, 1, COleVariant(pResult->pLoftPart->sSpec));
			CExcelOper::SetRangeBackColor(excel_sheet, 42, CXhChar16("B%d", index + 2));
			CExcelOper::SetRangeBackColor(excel_sheet, 44, CXhChar16("B%d", index + 3));
		}
		if (pResult->hashBoolByPropName.GetValue("cMaterial"))
		{
			map.SetValueAt(index + 1, 2, COleVariant(CBomModel::QueryMatMarkIncQuality(pResult->pLoftPart)));
			CExcelOper::SetRangeBackColor(excel_sheet, 42, CXhChar16("C%d", index + 2));
			CExcelOper::SetRangeBackColor(excel_sheet, 44, CXhChar16("C%d", index + 3));
		}
		if (pResult->hashBoolByPropName.GetValue("fLength"))
		{
			map.SetValueAt(index + 1, 3, COleVariant(CXhChar50("%.0f", pResult->pLoftPart->length)));
			CExcelOper::SetRangeBackColor(excel_sheet, 42, CXhChar16("D%d", index + 2));
			CExcelOper::SetRangeBackColor(excel_sheet, 44, CXhChar16("D%d", index + 3));
		}
		if (pResult->hashBoolByPropName.GetValue("nSingNum"))
		{
			map.SetValueAt(index + 1, 4, COleVariant((long)pResult->pLoftPart->GetPartNum()));
			CExcelOper::SetRangeBackColor(excel_sheet, 42, CXhChar16("E%d", index + 2));
			CExcelOper::SetRangeBackColor(excel_sheet, 44, CXhChar16("E%d", index + 3));
		}
		if (g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_SiChuan_ChengDu)
		{
			PART_ANGLE* pLoftJg = (PART_ANGLE*)pResult->pLoftPart;
			if (pResult->hashBoolByPropName.GetValue("Weld"))
			{
				map.SetValueAt(index + 1, 5, pLoftJg->bWeldPart ? COleVariant("*") : COleVariant(""));
				CExcelOper::SetRangeBackColor(excel_sheet, 42, CXhChar16("F%d", index + 2));
				CExcelOper::SetRangeBackColor(excel_sheet, 44, CXhChar16("F%d", index + 3));
			}
			if (pResult->hashBoolByPropName.GetValue("ZhiWan"))
			{
				map.SetValueAt(index + 1, 6, pLoftJg->siZhiWan > 0 ? COleVariant("*") : COleVariant(""));
				CExcelOper::SetRangeBackColor(excel_sheet, 42, CXhChar16("G%d", index + 2));
				CExcelOper::SetRangeBackColor(excel_sheet, 44, CXhChar16("G%d", index + 3));
			}
			if (pResult->hashBoolByPropName.GetValue("CutAngle"))
			{
				map.SetValueAt(index + 1, 7, pLoftJg->bCutAngle ? COleVariant("*") : COleVariant(""));
				CExcelOper::SetRangeBackColor(excel_sheet, 42, CXhChar16("H%d", index + 2));
				CExcelOper::SetRangeBackColor(excel_sheet, 44, CXhChar16("H%d", index + 3));
			}
			if (pResult->hashBoolByPropName.GetValue("PushFlat"))
			{
				map.SetValueAt(index + 1, 8, pLoftJg->nPushFlat > 0 ? COleVariant("*") : COleVariant(""));
				CExcelOper::SetRangeBackColor(excel_sheet, 42, CXhChar16("I%d", index + 2));
				CExcelOper::SetRangeBackColor(excel_sheet, 44, CXhChar16("I%d", index + 3));
			}
			if (pResult->hashBoolByPropName.GetValue("CutBer"))
			{
				map.SetValueAt(index + 1, 9, pLoftJg->bCutBer ? COleVariant("*") : COleVariant(""));
				CExcelOper::SetRangeBackColor(excel_sheet, 42, CXhChar16("J%d", index + 2));
				CExcelOper::SetRangeBackColor(excel_sheet, 44, CXhChar16("J%d", index + 3));
			}
			if (pResult->hashBoolByPropName.GetValue("CutRoot"))
			{
				map.SetValueAt(index + 1, 10, pLoftJg->bCutRoot ? COleVariant("*") : COleVariant(""));
				CExcelOper::SetRangeBackColor(excel_sheet, 42, CXhChar16("K%d", index + 2));
				CExcelOper::SetRangeBackColor(excel_sheet, 44, CXhChar16("K%d", index + 3));
			}
			if (pResult->hashBoolByPropName.GetValue("KaiJiao"))
			{
				map.SetValueAt(index + 1, 11, pLoftJg->bKaiJiao ? COleVariant("*") : COleVariant(""));
				CExcelOper::SetRangeBackColor(excel_sheet, 42, CXhChar16("L%d", index + 2));
				CExcelOper::SetRangeBackColor(excel_sheet, 44, CXhChar16("L%d", index + 3));
			}
			if (pResult->hashBoolByPropName.GetValue("HeJiao"))
			{
				map.SetValueAt(index + 1, 12, pLoftJg->bHeJiao ? COleVariant("*") : COleVariant(""));
				CExcelOper::SetRangeBackColor(excel_sheet, 42, CXhChar16("M%d", index + 2));
				CExcelOper::SetRangeBackColor(excel_sheet, 44, CXhChar16("M%d", index + 3));
			}
			if (pResult->hashBoolByPropName.GetValue("FootNail"))
			{
				map.SetValueAt(index + 1, 13, pLoftJg->bHasFootNail ? COleVariant("*") : COleVariant(""));
				CExcelOper::SetRangeBackColor(excel_sheet, 42, CXhChar16("N%d", index + 2));
				CExcelOper::SetRangeBackColor(excel_sheet, 44, CXhChar16("N%d", index + 3));
			}
			map.SetValueAt(index + 1, 14, COleVariant(CXhChar16("Xls")));
		}
		else
			map.SetValueAt(index + 1, 5, COleVariant(CXhChar16("Xls")));
		index += 2;
	}
	CXhChar16 sCellE("%C%d", 'A' + nCol - 1, index + 1);
	if (index > 0)
		CExcelOper::SetRangeValue(excel_sheet, "A2", sCellE, map.var);
	CExcelOper::SetRangeHorizontalAlignment(excel_sheet, "A1", sCellE, COleVariant((long)3));
	CExcelOper::SetRangeBorders(excel_sheet, "A1", sCellE, COleVariant(10.5));
}
void CProjectTowerType::AddPlateResultSheet(LPDISPATCH pSheet, ARRAY_LIST<CXhChar16>& keyStrArr)
{
	//���ñ���
	_Worksheet excel_sheet;
	excel_sheet.AttachDispatch(pSheet, FALSE);
	excel_sheet.Select();
	excel_sheet.SetName("У����");
	//�����ʾ��
	CStringArray str_arr;
	str_arr.SetSize(7);
	str_arr[0] = "����"; str_arr[1] = "���"; str_arr[2] = "����";
	str_arr[3] = "����"; str_arr[4] = "����"; str_arr[5] = "����";str_arr[6] = "������Դ";
	double col_arr[7] = { 10,10,10,10,10,10,10 };
	CExcelOper::AddRowToExcelSheet(excel_sheet, 1, str_arr, col_arr, TRUE);
	//�������
	int index = 0, nCol = 7, nResult = GetResultCount();
	CVariant2dArray map(nResult * 2, nCol);
	for (int i = 0; i < keyStrArr.GetSize(); i++)
	{
		COMPARE_PART_RESULT *pResult = GetResult(keyStrArr[i]);
		if (pResult == NULL || pResult->pLoftPart == NULL || pResult->pOrgPart == NULL)
			continue;
		map.SetValueAt(index, 0, COleVariant(pResult->pOrgPart->sPartNo));
		map.SetValueAt(index, 1, COleVariant(pResult->pOrgPart->sSpec));
		map.SetValueAt(index, 2, COleVariant(CBomModel::QueryMatMarkIncQuality(pResult->pOrgPart)));
		map.SetValueAt(index, 3, COleVariant((long)pResult->pOrgPart->GetPartNum()));
		map.SetValueAt(index, 4, pResult->pOrgPart->bWeldPart ? COleVariant("*") : COleVariant(""));
		map.SetValueAt(index, 5, pResult->pOrgPart->siZhiWan > 0 ? COleVariant("*") : COleVariant(""));
		map.SetValueAt(index, 6, COleVariant(CXhChar16("DWG")));
		//
		CExcelOper::MergeRowRange(excel_sheet, CXhChar16("A%d", index + 2), CXhChar16("A%d", index + 3));
		if (pResult->hashBoolByPropName.GetValue("spec"))
		{
			map.SetValueAt(index + 1, 1, COleVariant(pResult->pLoftPart->sSpec));
			CExcelOper::SetRangeBackColor(excel_sheet, 42, CXhChar16("B%d", index + 2));
			CExcelOper::SetRangeBackColor(excel_sheet, 44, CXhChar16("B%d", index + 3));
		}
		if (pResult->hashBoolByPropName.GetValue("cMaterial"))
		{
			map.SetValueAt(index + 1, 2, COleVariant(CBomModel::QueryMatMarkIncQuality(pResult->pLoftPart)));
			CExcelOper::SetRangeBackColor(excel_sheet, 42, CXhChar16("C%d", index + 2));
			CExcelOper::SetRangeBackColor(excel_sheet, 44, CXhChar16("C%d", index + 3));
		}
		if (pResult->hashBoolByPropName.GetValue("nSingNum"))
		{
			map.SetValueAt(index + 1, 3, COleVariant((long)pResult->pLoftPart->GetPartNum()));
			CExcelOper::SetRangeBackColor(excel_sheet, 42, CXhChar16("D%d", index + 2));
			CExcelOper::SetRangeBackColor(excel_sheet, 44, CXhChar16("D%d", index + 3));
		}
		if (pResult->hashBoolByPropName.GetValue("Weld"))
		{
			map.SetValueAt(index + 1, 4, COleVariant((long)pResult->pLoftPart->GetPartNum()));
			CExcelOper::SetRangeBackColor(excel_sheet, 42, CXhChar16("E%d", index + 2));
			CExcelOper::SetRangeBackColor(excel_sheet, 44, CXhChar16("E%d", index + 3));
		}
		if (pResult->hashBoolByPropName.GetValue("ZhiWan"))
		{
			map.SetValueAt(index + 1, 5, COleVariant((long)pResult->pLoftPart->GetPartNum()));
			CExcelOper::SetRangeBackColor(excel_sheet, 42, CXhChar16("F%d", index + 2));
			CExcelOper::SetRangeBackColor(excel_sheet, 44, CXhChar16("F%d", index + 3));
		}
		map.SetValueAt(index + 1, 6, COleVariant(CXhChar16("Excle")));
		index += 2;
	}
	CXhChar16 sCellE("%C%d", 'A' + nCol - 1, index + 1);
	if (index > 0)
		CExcelOper::SetRangeValue(excel_sheet, "A2", sCellE, map.var);
	CExcelOper::SetRangeHorizontalAlignment(excel_sheet, "A1", sCellE, COleVariant((long)3));
	CExcelOper::SetRangeBorders(excel_sheet, "A1", sCellE, COleVariant(10.5));
}
//
void CProjectTowerType::AddCompareResultSheet(LPDISPATCH pSheet, int iSheet, int iCompareType)
{
	//��У������������
	ARRAY_LIST<CXhChar16> keyStrArr;
	for (COMPARE_PART_RESULT *pResult = EnumFirstResult(); pResult; pResult = EnumNextResult())
	{
		if (pResult->pLoftPart)
			keyStrArr.append(pResult->pLoftPart->sPartNo);
		else
			keyStrArr.append(pResult->pOrgPart->sPartNo);
	}
	CQuickSort<CXhChar16>::QuickSort(keyStrArr.m_pData, keyStrArr.GetSize(), compare_func);
	//����Excel
	int index = 0, nCol = 4, nResult = GetResultCount();
	double col_arr[4] = { 15,15,15,15 };
	CStringArray str_arr;
	str_arr.SetSize(4);
	str_arr[0] = "����"; str_arr[1] = "���"; str_arr[2] = "����"; str_arr[3] = "����";
	if (iSheet == 1)
	{	//��һ�ֽ����ERP��TMA���е�������Ϣ��ͬ
		if (iCompareType == COMPARE_BOM_FILE)
			AddBomResultSheet(pSheet, keyStrArr);
		else if (iCompareType == COMPARE_ANGLE_DWG)
			AddAngleResultSheet(pSheet, keyStrArr);
		else if (iCompareType == COMPARE_PLATE_DWG)
			AddPlateResultSheet(pSheet, keyStrArr);
	}
	else if (iSheet == 2)
	{	//�ڶ��ֽ����ԭʼ���������ݣ���������û������
		_Worksheet excel_sheet;
		excel_sheet.AttachDispatch(pSheet, FALSE);
		excel_sheet.Select();
		CExcelOper::AddRowToExcelSheet(excel_sheet, 1, str_arr, col_arr, TRUE);
		if (iCompareType == COMPARE_BOM_FILE)
		{
			if (g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_AnHui_HongYuan)
				excel_sheet.SetName("ERP����");
			else
				excel_sheet.SetName(CXhChar500("%s����",(char*)m_xOrigBom.m_sBomName));
		}
		else
			excel_sheet.SetName("DWG���");
		//�������
		CVariant2dArray map(nResult * 2, nCol);
		for (int i = 0; i < keyStrArr.GetSize(); i++)
		{
			COMPARE_PART_RESULT *pResult = GetResult(keyStrArr[i]);
			if (pResult == NULL || pResult->pLoftPart || pResult->pOrgPart == NULL)
				continue;
			map.SetValueAt(index, 0, COleVariant(pResult->pOrgPart->sPartNo));
			map.SetValueAt(index, 1, COleVariant(pResult->pOrgPart->sSpec));
			map.SetValueAt(index, 2, COleVariant(CBomModel::QueryMatMarkIncQuality(pResult->pOrgPart)));
			map.SetValueAt(index, 3, COleVariant(CXhChar50("%.0f", pResult->pOrgPart->length)));
			index++;
		}
		CXhChar16 sCellE("%C%d", 'A' + nCol - 1, index + 2);
		CExcelOper::SetRangeValue(excel_sheet, "A2", sCellE, map.var);
		CExcelOper::SetRangeHorizontalAlignment(excel_sheet, "A1", sCellE, COleVariant((long)3));
		CExcelOper::SetRangeBorders(excel_sheet, "A1", sCellE, COleVariant(10.5));
	}
	else if (iSheet == 3)
	{	//�����ֽ�����������������ݣ�ԭʼ����û������
		_Worksheet excel_sheet;
		excel_sheet.AttachDispatch(pSheet, FALSE);
		excel_sheet.Select();
		CExcelOper::AddRowToExcelSheet(excel_sheet, 1, str_arr, col_arr, TRUE);
		if (iCompareType == COMPARE_BOM_FILE)
		{
			if(g_xUbomModel.m_uiCustomizeSerial == CBomModel::ID_AnHui_HongYuan)
				excel_sheet.SetName("ERP��ȱ��");
			else
				excel_sheet.SetName(CXhChar100("%s����",(char*)m_xLoftBom.m_sBomName));
		}
		else
			excel_sheet.SetName("DWGȱ��");
		//�������
		CVariant2dArray map(nResult * 2, nCol);
		for (int i = 0; i < keyStrArr.GetSize(); i++)
		{
			COMPARE_PART_RESULT *pResult = GetResult(keyStrArr[i]);
			if (pResult == NULL || pResult->pLoftPart == NULL || pResult->pOrgPart)
				continue;
			map.SetValueAt(index, 0, COleVariant(pResult->pLoftPart->sPartNo));
			map.SetValueAt(index, 1, COleVariant(pResult->pLoftPart->sSpec));
			map.SetValueAt(index, 2, COleVariant(CBomModel::QueryMatMarkIncQuality(pResult->pLoftPart)));
			map.SetValueAt(index, 3, COleVariant(CXhChar50("%.0f", pResult->pLoftPart->length)));
			index++;
		}
		CXhChar16 sCellE("%C%d", 'A' + nCol - 1, index + 2);
		CExcelOper::SetRangeValue(excel_sheet, "A2", sCellE, map.var);
		CExcelOper::SetRangeHorizontalAlignment(excel_sheet, "A1", sCellE, COleVariant((long)3));
		CExcelOper::SetRangeBorders(excel_sheet, "A1", sCellE, COleVariant(10.5));
	}
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
		map.SetValueAt(index,0,COleVariant(pResult->pLoftPart->sPartNo));
		map.SetValueAt(index,1,COleVariant(pResult->pLoftPart->sSpec));
		map.SetValueAt(index,2,COleVariant(CBomModel::QueryMatMarkIncQuality(pResult->pLoftPart)));
		map.SetValueAt(index,3,COleVariant(CXhChar50("%.0f",pResult->pLoftPart->length)));
		map.SetValueAt(index,4,COleVariant((long)pResult->pLoftPart->GetPartNum()));
		map.SetValueAt(index,5,COleVariant((long)pResult->pLoftPart->feature1));
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
	int nSheet=3;
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
		if (nSheet == 3)
			AddCompareResultSheet(pWorksheet, iSheet, iCompareType);
		if (nSheet == 1)
			AddDwgLackPartSheet(pWorksheet);
	}
	excel_sheets.ReleaseDispatch();
}
//����ERP�ϵ��м��ţ�����ΪQ420,����ǰ��P������ΪQ345������ǰ��H��
BOOL CProjectTowerType::ModifyErpBomPartNo(BYTE ciMatCharPosType)
{
	if (m_xOrigBom.m_sFileName.GetLength() <= 0)
		return FALSE;
	CExcelOperObject excelobj;
	if (!excelobj.OpenExcelFile(m_xOrigBom.m_sFileName))
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
	excel_range.AttachDispatch(pRange);
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
	ARRAY_LIST<BOMPART*> partPtrList;
	for (BOMPART *pPart = m_xLoftBom.EnumFirstPart(); pPart; pPart = m_xLoftBom.EnumNextPart())
		partPtrList.append(pPart);
	char cMaterial;
	CXhChar16 sPartNo, sMaterial, sNewPartNo;
	for (int i = 0; i < partPtrList.GetSize(); i++)
	{
		sPartNo = partPtrList[i]->sPartNo;
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
UINT CBomModel::m_uiCustomizeSerial = 0;
CBomModel::CBomModel(void)
{
	
}
CBomModel::~CBomModel(void)
{
	
}
void CBomModel::InitBomTblCfg()
{
	char file_name[MAX_PATH] = "", line_txt[MAX_PATH] = "", key_word[100] = "";
	GetAppPath(file_name);
	strcat(file_name, "ubom.cfg");
	FILE *fp = fopen(file_name, "rt");
	if (fp == NULL)
		return;
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
		if (_stricmp(key_word, "CLIENT_ID") == 0)
		{
			skey = strtok(NULL, "=,;");
			CBomModel::m_uiCustomizeSerial = atoi(skey);
		}
		else if (_stricmp(key_word, "TMA_BOM") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL && strlen(skey) > 0)
			{
				m_xTmaTblCfg.m_sColIndexArr.Copy(skey);
				m_xTmaTblCfg.m_sColIndexArr.Replace(" ", "");
				//��ȡ����
				skey = strtok(NULL, ";");
				if (skey != NULL)
					m_xTmaTblCfg.m_nColCount = atoi(skey);
				//������ʼ��
				skey = strtok(NULL, ";");
				if (skey != NULL)
					m_xTmaTblCfg.m_nStartRow = atoi(skey);
			}
		}
		else if (_stricmp(key_word, "ERP_BOM") == 0)
		{
			skey = strtok(NULL, ";");
			if (skey != NULL && strlen(skey) > 0)
			{
				m_xErpTblCfg.m_sColIndexArr.Copy(skey);
				m_xErpTblCfg.m_sColIndexArr.Replace(" ", "");
				//��ȡ����
				skey = strtok(NULL, ";");
				if(skey != NULL)
					m_xErpTblCfg.m_nColCount = atoi(skey);
				//������ʼ��
				skey = strtok(NULL, ";");
				if (skey != NULL)
					m_xErpTblCfg.m_nStartRow = atoi(skey);
			}
		}
	}
	fclose(fp);
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
CXhChar16 CBomModel::QueryMatMarkIncQuality(BOMPART *pPart)
{
	CXhChar16 sMatMark;
	if (pPart)
	{
		sMatMark = CProcessPart::QuerySteelMatMark(pPart->cMaterial, pPart->sMaterial);
		if (pPart->cQualityLevel != 0 && !sMatMark.EqualNoCase(pPart->sMaterial))
			sMatMark.Append(pPart->cQualityLevel);
	}
	return sMatMark;
}
#endif