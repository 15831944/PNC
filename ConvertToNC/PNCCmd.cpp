#include "StdAfx.h"
#include "PNCCmd.h"
#include "CadToolFunc.h"
#include "PNCModel.h"
#include "PNCSysPara.h"
#include "PNCSysSettingDlg.h"
#include "InputMKRectDlg.h"
#include "BomModel.h"
#include "RevisionDlg.h"
#include "LicFuncDef.h"
#include "ProcBarDlg.h"
#include "folder_dialog.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

CProcBarDlg *pProcDlg = NULL;
static int prevPercent = -1;
void DisplayProcess(int percent, char *sTitle)
{
	if (percent >= 100)
	{
		if (pProcDlg != NULL)
		{
			pProcDlg->DestroyWindow();
			delete pProcDlg;
			pProcDlg = NULL;
		}
		return;
	}
	else if (pProcDlg == NULL)
		pProcDlg = new CProcBarDlg(NULL);
	if (pProcDlg->GetSafeHwnd() == NULL)
		pProcDlg->Create();
	if (percent == prevPercent)
		return;	//���ϴν���һ�²���Ҫ����
	else
		prevPercent = percent;
	if (sTitle)
		pProcDlg->SetTitle(CString(sTitle));
	else
		pProcDlg->SetTitle("����");
	pProcDlg->Refresh(percent);
}

void UpdatePartList()
{
#ifndef __UBOM_ONLY_
	CPartListDlg *pPartListDlg = g_xDockBarManager.GetPartListDlgPtr();
	if (pPartListDlg != NULL)
		pPartListDlg->UpdatePartList();
#endif
}
//////////////////////////////////////////////////////////////////////////
//������ȡ�����Ϣ,��ȡ�ɹ���֧���Ű�
//SmartExtractPlate
//////////////////////////////////////////////////////////////////////////
void SmartExtractPlate()
{
#if defined(__UBOM_) || defined(__UBOM_ONLY_)
	CDwgFileInfo *pDwgFile = NULL;
	AcApDocument* pDoc = acDocManager->curDocument();
	if (pDoc != NULL)
	{
		CString file_path = pDoc->fileName();
		pDwgFile = g_xUbomModel.FindDwgFile(file_path);
	}
	if (pDwgFile)
		SmartExtractPlate(pDwgFile->GetPncModel());
	else
		SmartExtractPlate(&model);
#else
	CString file_path;
	GetCurWorkPath(file_path);
	if (g_pncSysPara.m_bAutoLayout == CPNCSysPara::LAYOUT_SEG)
	{	//����Ԥ��ģʽ�������ϴ���ȡ�ĸְ���Ϣ�������ظ���ȡ
		if (!model.m_sWorkPath.EqualNoCase(file_path))
		{
			model.Empty();
			model.m_sWorkPath.Copy(file_path);
		}
	}
	else
		model.Empty();
	SmartExtractPlate(&model);
#endif
}

void SmartExtractPlate(CPNCModel *pModel)
{
	if (pModel == NULL)
		return;
	pModel->DisplayProcess = DisplayProcess;
	CLogErrorLife logErrLife;
	CHashSet<AcDbObjectId> selectedEntList;
	//Ĭ��ѡ�����е�ͼ�Σ�������ڵĹ���ʹ��
	SelCadEntSet(pModel->m_xAllEntIdSet, TRUE);
#ifndef __UBOM_ONLY_
	//PNC֧�ֽ����ֶ���ѡ
	if (!SelCadEntSet(selectedEntList))
		return;	
#else
	//UBOMĬ�ϴ�������ͼԪ
	for (AcDbObjectId entId = pModel->m_xAllEntIdSet.GetFirst(); entId; entId = pModel->m_xAllEntIdSet.GetNext())
		selectedEntList.SetValue(entId.asOldId(), entId);
#endif
	//�ӿ�ѡ��Ϣ����ȡ�иְ�ı�ʶ��ͳ�Ƹְ弯��
	CHashSet<AcDbObjectId> textIdHash;
	ATOM_LIST<GEPOINT> holePosList;
	AcDbEntity *pEnt = NULL;
	for (AcDbObjectId entId=selectedEntList.GetFirst(); entId.isValid();entId=selectedEntList.GetNext())
	{
		CAcDbObjLife objLife(entId);
		if((pEnt = objLife.GetEnt())==NULL)
			continue;
		if (!pEnt->isKindOf(AcDbText::desc()) && !pEnt->isKindOf(AcDbMText::desc()) && !pEnt->isKindOf(AcDbBlockReference::desc()))
			continue;
		textIdHash.SetValue(entId.asOldId(), entId);
		//
		BASIC_INFO baseInfo;
		GEPOINT dim_pos, dim_vec;
		CXhChar500 sText;
		CXhChar16 sPartNo;
		if (pEnt->isKindOf(AcDbText::desc()) || pEnt->isKindOf(AcDbMText::desc()))
		{
			sText = GetCadTextContent(pEnt);
			if(!g_pncSysPara.ParsePartNoText(pEnt,sPartNo))
				continue;
			dim_pos = GetCadTextDimPos(pEnt, &dim_vec);
		}
		else if (pEnt->isKindOf(AcDbBlockReference::desc()))
		{	//�ӿ��н����ְ���Ϣ
			AcDbBlockReference *pBlockRef = (AcDbBlockReference*)pEnt;
			if(g_pncSysPara.RecogBasicInfo(pBlockRef,baseInfo))
			{
				AcGePoint3d txt_pos = pBlockRef->position();
				dim_pos.Set(txt_pos.x, txt_pos.y, txt_pos.z);
				dim_vec.Set(cos(pBlockRef->rotation()), sin(pBlockRef->rotation()));
			}
			else
			{
				BOLT_HOLE hole;
				if (g_pncSysPara.RecogBoltHole(pEnt, hole))
					holePosList.append(GEPOINT(hole.posX, hole.posY));
				continue;
			}
			if (baseInfo.m_sPartNo.GetLength() > 0)
				sPartNo.Copy(baseInfo.m_sPartNo);
		}
		if(strlen(sPartNo) <= 0)
		{
			logerr.Log("�ְ���Ϣ%s,ʶ����Ʋ���ʶ��!", (char*)sText);
			continue;
		}
		CPlateProcessInfo *pExistPlate = pModel->GetPlateInfo(sPartNo);
		if (pExistPlate != NULL && !(pExistPlate->partNoId == entId || pExistPlate->plateInfoBlockRefId == entId))
		{	//������ͬ���������ı���Ӧ��ʵ�岻��ͬ��ʾ�����ظ� wht 19-07-22
			logerr.Log("�ְ�%s,�����ظ���ȷ��!", (char*)sText);
			continue;
		}
		CPlateProcessInfo* pPlateProcess = pModel->AppendPlate(sPartNo);
		pPlateProcess->m_bNeedExtract = TRUE;	//ѡ��CADʵ�������ǰ����ʱ����λ��Ҫ��ȡ
		pPlateProcess->dim_pos = dim_pos;
		pPlateProcess->dim_vec = dim_vec;
		if (baseInfo.m_sPartNo.GetLength() > 0)
		{
			pPlateProcess->plateInfoBlockRefId = entId;
			pPlateProcess->xPlate.SetPartNo(sPartNo);
			pPlateProcess->xPlate.cMaterial = baseInfo.m_cMat;
			pPlateProcess->xPlate.m_fThick = (float)baseInfo.m_nThick;
			pPlateProcess->xPlate.m_nProcessNum = baseInfo.m_nNum;
			pPlateProcess->xPlate.m_nSingleNum = baseInfo.m_nNum;
			pPlateProcess->xPlate.mkpos = dim_pos;
			pPlateProcess->xPlate.mkVec = dim_vec;
			if (pModel->m_sTaStampNo.GetLength() <= 0 && baseInfo.m_sTaStampNo.GetLength() > 0)
				pModel->m_sTaStampNo.Copy(baseInfo.m_sTaStampNo);
			if (pModel->m_sTaType.GetLength() <= 0 && baseInfo.m_sTaType.GetLength() > 0)
				pModel->m_sTaType.Copy(baseInfo.m_sTaType);
			if (pModel->m_sPrjCode.GetLength() <= 0 && baseInfo.m_sPrjCode.GetLength() > 0)
				pModel->m_sPrjCode.Copy(baseInfo.m_sPrjCode);
		}
		else
		{
			pPlateProcess->partNoId = entId;
			pPlateProcess->xPlate.SetPartNo(sPartNo);
			pPlateProcess->xPlate.cMaterial = 'S';
		}
	}
	if(pModel->GetPlateNum()<=0)
	{
		logerr.Log("ʶ����Ʋ���ʶ����ļ��ĸְ���Ϣ!");
		return;
	}
	//���ñ�����ȡλ�ã����ڴ�����ְ��С�����ֱ�ע�ŵ��ְ���������wht 19-02-01
	const int HOLE_SEARCH_SCOPE = 60;
	for(CPlateProcessInfo* pPlateProcess=pModel->EnumFirstPlate(TRUE);pPlateProcess;pPlateProcess=pModel->EnumNextPlate(TRUE))
	{
		for(GEPOINT *pPt=holePosList.GetFirst();pPt;pPt=holePosList.GetNext())
		{
			if(DISTANCE(pPlateProcess->dim_pos,*pPt)<HOLE_SEARCH_SCOPE)
			{
				pPlateProcess->inner_dim_pos=*pPt;
				pPlateProcess->m_bHasInnerDimPos=TRUE;
				break;
			}
		}
	}
	//��ȡ�ְ��������
	if (g_pncSysPara.m_ciRecogMode == CPNCSysPara::FILTER_BY_PIXEL)
		pModel->ExtractPlateProfileEx(selectedEntList);
	else
		pModel->ExtractPlateProfile(selectedEntList);
#ifndef __UBOM_ONLY_
	//����ְ�һ���ŵ����
	pModel->MergeManyPartNo();
	//���������պ�������¸ְ�Ļ�����Ϣ+��˨��Ϣ+��������Ϣ
	int nSum=0,nValid=0;
	for(CPlateProcessInfo* pPlateProcess=pModel->EnumFirstPlate(TRUE);pPlateProcess;pPlateProcess=pModel->EnumNextPlate(TRUE),nSum++)
	{
		pPlateProcess->ExtractPlateRelaEnts();
		if(!pPlateProcess->UpdatePlateInfo())
			logerr.Log("����%s��ѡ���˴���ı߽�,������ѡ��.(λ�ã�%s)",(char*)pPlateProcess->GetPartNo(),(char*)CXhChar50(pPlateProcess->dim_pos));
		else
			nValid++;
	}
	//����ȡ�ĸְ���Ϣ�����������ļ���
	CString file_path;
	GetCurWorkPath(file_path);
	for(CPlateProcessInfo* pPlateProcess=pModel->EnumFirstPlate(TRUE);pPlateProcess;pPlateProcess=pModel->EnumNextPlate(TRUE))
	{	//����PPI�ļ�,���浽����ǰ����·����
		if(pPlateProcess->vertexList.GetNodeNum()>3)
			pPlateProcess->CreatePPiFile(file_path);
	}
	//������ȡ�ĸְ�����--֧���Ű�
	pModel->LayoutPlates(g_pncSysPara.m_bAutoLayout);
	//�����ȡ��ͳһ���øְ���ȡ״̬ΪFALSE wht 19-06-17
	for (CPlateProcessInfo* pPlateProcess = pModel->EnumFirstPlate(TRUE); pPlateProcess; pPlateProcess = pModel->EnumNextPlate(TRUE))
	{	
		pPlateProcess->m_bNeedExtract = FALSE;
	}
	//д�������������ļ� wht 19-01-12
	CString cfg_path;
	cfg_path.Format("%sconfig.ini",file_path);
	pModel->WritePrjTowerInfoToCfgFile(cfg_path);
	//
	UpdatePartList();
	//
	AfxMessageBox(CXhChar100("����ȡ�ְ�%d�����ɹ�%d����ʧ��%d!",nSum,nValid,nSum-nValid));
#else
	//UBOMֻ��Ҫ���¸ְ�Ļ�����Ϣ
	pModel->MergeManyPartNo();
	for (AcDbObjectId objId = textIdHash.GetFirst(); objId; objId = textIdHash.GetNext())
	{
		CAcDbObjLife objLife(objId);
		AcDbEntity *pEnt = objLife.GetEnt();
		if (pEnt==NULL || !pEnt->isKindOf(AcDbText::desc()))
			continue;
		GEPOINT dim_pos=GetCadTextDimPos(pEnt);
		CPlateProcessInfo* pPlateInfo = pModel->GetPlateInfo(dim_pos);
		if(pPlateInfo==NULL)
			continue;	//
		BASIC_INFO baseInfo;
		if (g_pncSysPara.RecogBasicInfo(pEnt, baseInfo))
		{
			if (baseInfo.m_cMat > 0)
				pPlateInfo->xPlate.cMaterial = baseInfo.m_cMat;
			if (baseInfo.m_cQuality > 0)
				pPlateInfo->xPlate.cQuality = baseInfo.m_cQuality;
			if (baseInfo.m_nThick > 0)
				pPlateInfo->xPlate.m_fThick = (float)baseInfo.m_nThick;
			if (baseInfo.m_nNum > 0)
				pPlateProcess->xPlate.m_nSingleNum = pPlateInfo->xPlate.m_nProcessNum = baseInfo.m_nNum;
			if (baseInfo.m_idCadEntNum != 0)
				pPlateInfo->partNumId = MkCadObjId(baseInfo.m_idCadEntNum);
		}
		else
		{
			CXhChar100 sText = GetCadTextContent(pEnt);
			if (strstr(sText, "���") || strstr(sText, "����") || strstr(sText, "����") || strstr(sText, "����"))
				pPlateInfo->xBomPlate.siZhiWan = 1;
			if (strstr(sText,"����"))
				pPlateInfo->xBomPlate.bWeldPart = TRUE;
		}
	}
	pModel->SplitManyPartNo();
	for (CPlateProcessInfo* pPlateInfo = pModel->EnumFirstPlate(FALSE); pPlateInfo; pPlateInfo = pModel->EnumNextPlate(FALSE))
	{
		pPlateInfo->xBomPlate.sPartNo = pPlateInfo->GetPartNo();
		pPlateInfo->xBomPlate.cMaterial = pPlateInfo->xPlate.cMaterial;
		pPlateInfo->xBomPlate.cQualityLevel = pPlateInfo->xPlate.cQuality;
		pPlateInfo->xBomPlate.thick = pPlateInfo->xPlate.m_fThick;
		pPlateInfo->xBomPlate.feature1 = pPlateInfo->xPlate.m_nProcessNum;	//�ӹ���
		pPlateInfo->xBomPlate.SetPartNum(pPlateInfo->xPlate.m_nSingleNum);	//������
		if (pPlateInfo->xBomPlate.thick <= 0)
			logerr.Log("�ְ�%s��Ϣ��ȡʧ��!", (char*)pPlateInfo->xBomPlate.sPartNo);
		else
			pPlateInfo->xBomPlate.sSpec.Printf("-%.f", pPlateInfo->xBomPlate.thick);
	}
#endif
}

//////////////////////////////////////////////////////////////////////////
//�����ְ����Ϣ
//ReviseThePlate
//////////////////////////////////////////////////////////////////////////
void ManualExtractPlate()
{
#if defined(__UBOM_) || defined(__UBOM_ONLY_)
	CDwgFileInfo *pDwgFile = NULL;
	AcApDocument* pDoc = acDocManager->curDocument();
	if (pDoc != NULL)
	{
		CString file_path = pDoc->fileName();
		pDwgFile = g_xUbomModel.FindDwgFile(file_path);
	}
	if (pDwgFile)
		ManualExtractPlate(pDwgFile->GetPncModel());
	else
		ManualExtractPlate(&model);
#else
	ManualExtractPlate(&model);
#endif
}
void ManualExtractPlate(CPNCModel *pModel)
{
	if (pModel == NULL)
		return;
	CLogErrorLife logErrLife;
	//��ѡ�������ְ��������
	CHashSet<AcDbObjectId> selectedEntList;
	if (!SelCadEntSet(selectedEntList))
		return;
	//����ѡ�񼯺���ȡ�ְ�������ߣ���ʼ���������˨��Ϣ
	CPlateProcessInfo plate;
	if(!plate.InitProfileBySelEnts(selectedEntList))
	{
		logerr.Log("ѡ������������󣬹�������Ч�պ�����");
		return;
	}
	//���������պ�������»�����Ϣ+��˨��Ϣ+��������Ϣ
	plate.ExtractPlateRelaEnts();
	if(!plate.UpdatePlateInfo(TRUE))
	{
		logerr.Log("����%s����ȡ����",(char*)plate.GetPartNo());
		return;
	}
	//��ģ���б�����Ӹְ���Ϣ wht 19-12-21
	CPlateProcessInfo *pExistPlate = pModel->GetPlateInfo(plate.GetPartNo());
	if (pExistPlate == NULL)
		pExistPlate = pModel->AppendPlate(plate.GetPartNo());
	pExistPlate->InitProfileBySelEnts(selectedEntList);
	pExistPlate->ExtractPlateRelaEnts();
	pExistPlate->UpdatePlateInfo(TRUE);
#ifndef __UBOM_ONLY_
	//����PPI�ļ�,���浽����ǰ����·����
	CString file_path;
	GetCurWorkPath(file_path);
	if(pExistPlate->vertexList.GetNodeNum()>3)
		pExistPlate->CreatePPiFile(file_path);
	//������ȡ�ĸְ�����--֧���Ű�
	pModel->LayoutPlates(g_pncSysPara.m_bAutoLayout);
	//
	UpdatePartList();
#else
	pExistPlate->xBomPlate.cMaterial = pExistPlate->xPlate.cMaterial;
	pExistPlate->xBomPlate.cQualityLevel = pExistPlate->xPlate.cQuality;
	pExistPlate->xBomPlate.thick = pExistPlate->xPlate.m_fThick;
	pExistPlate->xBomPlate.sSpec.Printf("-%.f", pExistPlate->xBomPlate.thick);
	pExistPlate->xBomPlate.feature1 = pExistPlate->xPlate.m_nProcessNum;	//�ӹ���
	pExistPlate->xBomPlate.SetPartNum(pExistPlate->xPlate.m_nSingleNum);	//������
#endif
}

#ifndef __UBOM_ONLY_
//////////////////////////////////////////////////////////////////////////
//�༭�ְ���Ϣ
//CreatePartEditProcess
//WriteToClient
//////////////////////////////////////////////////////////////////////////
BOOL CreatePartEditProcess( HANDLE hClientPipeRead, HANDLE hClientPipeWrite )
{
	TCHAR cmd_str[MAX_PATH];
	GetAppPath(cmd_str);
	strcat(cmd_str,"PPE.exe -child");

	STARTUPINFO startInfo;
	memset( &startInfo, 0 , sizeof( STARTUPINFO ) );

	startInfo.cb= sizeof( STARTUPINFO );
	startInfo.dwFlags |= STARTF_USESTDHANDLES;
	startInfo.hStdError= 0;;
	startInfo.hStdInput= hClientPipeRead;
	startInfo.hStdOutput= hClientPipeWrite;

	PROCESS_INFORMATION processInfo;
	memset( &processInfo, 0, sizeof(PROCESS_INFORMATION) );
	BOOL b=CreateProcess( NULL,cmd_str,
		NULL,NULL, TRUE,CREATE_NEW_CONSOLE, NULL, NULL,&startInfo,&processInfo );
	DWORD er=GetLastError();
	return b;
}
BOOL WriteToClient(HANDLE hPipeWrite)
{
	if( hPipeWrite == INVALID_HANDLE_VALUE )
		return FALSE;
	CBuffer buffer(10000);	//10Kb
	DWORD version[2]={0,20170615};
	BYTE* pByteVer=(BYTE*)version;
	pByteVer[0]=1;
	pByteVer[1]=2;
	pByteVer[2]=0;
	pByteVer[3]=0;
	BYTE cProductType=PRODUCT_PNC;
	//1��д����ܹ�֤����Ϣ
	char APP_PATH[MAX_PATH];
	GetAppPath(APP_PATH);
	buffer.WriteByte(cProductType);
	buffer.WriteDword(version[0]);
	buffer.WriteDword(version[1]);
	buffer.WriteString(APP_PATH,MAX_PATH);
	//2��д��PPE����ģʽ��Ϣ
	CString sFilePath;
	if(GetCurWorkPath(sFilePath,FALSE)==FALSE)
	{
		buffer.WriteByte(0);
		buffer.WriteInteger(0);
	}
	else
	{
		buffer.WriteByte(6);
		CBuffer file_buffer(10000);
		file_buffer.WriteString(sFilePath);	//д��PPI�ļ�·��
		buffer.WriteDword(file_buffer.GetLength());
		buffer.Write(file_buffer.GetBufferPtr(),file_buffer.GetLength());
	}
	//4���������ܵ���д������
	return buffer.WriteToPipe(hPipeWrite,1024);
}
void SendPartEditor()
{
	CLogErrorLife logErrLife;
	//1��������һ���ܵ�: ���ڷ���������ͻ��˷�������
	SECURITY_ATTRIBUTES attrib;
	attrib.nLength = sizeof( SECURITY_ATTRIBUTES );
	attrib.bInheritHandle= true;
	attrib.lpSecurityDescriptor = NULL;
	HANDLE hPipeClientRead=NULL, hPipeSrcWrite=NULL;
	if( !CreatePipe( &hPipeClientRead, &hPipeSrcWrite, &attrib, 0 ) )
	{
		logerr.Log("���������ܵ�ʧ��!GetLastError= %d\n", GetLastError() );
		return;
	}
	HANDLE hPipeSrcWriteDup=NULL;
	if( !DuplicateHandle( GetCurrentProcess(), hPipeSrcWrite, GetCurrentProcess(), &hPipeSrcWriteDup, 0, false, DUPLICATE_SAME_ACCESS ) )
	{
		logerr.Log("���ƾ��ʧ��,GetLastError=%d\n", GetLastError() );
		return;
	}
	CloseHandle( hPipeSrcWrite );
	//2�������ڶ����ܵ������ڿͻ�����������˷�������
	HANDLE hPipeClientWrite=NULL, hPipeSrcRead=NULL;
	if( !CreatePipe( &hPipeSrcRead, &hPipeClientWrite, &attrib, 0) )
	{
		logerr.Log("�����ڶ��������ܵ�ʧ��,GetLastError=%d\n", GetLastError() );
		return;
	}
	HANDLE hPipeSrcReadDup=NULL;
	if( !DuplicateHandle( GetCurrentProcess(), hPipeSrcRead, GetCurrentProcess(), &hPipeSrcReadDup, 0, false, DUPLICATE_SAME_ACCESS ) )
	{
		logerr.Log("���Ƶڶ������ʧ��,GetLastError=%d\n", GetLastError() );
		return;
	}
	CloseHandle( hPipeSrcRead );
	//3�������ӽ���,
	if( !CreatePartEditProcess( hPipeClientRead, hPipeClientWrite ) )
	{
		logerr.Log("�����ӽ���ʧ��\n" );
		return;
	}
	//4���������ݲ���---֧�ִ��Ͷ������
	if( !WriteToClient(hPipeSrcWriteDup))
	{
		logerr.Log("���ݴ���ʧ��");
		return;
	}
}

//////////////////////////////////////////////////////////////////////////
//�ְ岼��,�Զ��Ű�
//LayoutPlates
//////////////////////////////////////////////////////////////////////////
void LayoutPlates()
{	
	CLogErrorLife logeErrLife;
	model.LayoutPlates(TRUE);
}
//////////////////////////////////////////////////////////////////////////
//ʵʱ�����ӡ��
//InsertMKRect		
//////////////////////////////////////////////////////////////////////////
void InsertMKRect()
{
	CAcModuleResourceOverride resOverride;
	CXhChar16 blkname("MK");
	AcDbObjectId blockId=SearchBlock(blkname);
	if(blockId.isNull())
	{	//�½�����¼ָ�룬���ɸ�ӡ���ͼԪ
		double txt_height=2;
		int nRectL=60,nRectW=30;
		CInputMKRectDlg dlg;
		dlg.m_nRectL=nRectL;
		dlg.m_nRectW=nRectW;
		dlg.m_fTextH=txt_height;
		if(dlg.DoModal()==IDOK)
		{
			nRectL=dlg.m_nRectL;
			nRectW=dlg.m_nRectW;
			txt_height=dlg.m_fTextH;
		}
		DRAGSET.ClearEntSet();
		AcDbBlockTable *pTempBlockTable;
		GetCurDwg()->getBlockTable(pTempBlockTable,AcDb::kForWrite);
		AcDbBlockTableRecord *pTempBlockTableRecord=new AcDbBlockTableRecord();//�������¼ָ��
#ifdef _ARX_2007
		pTempBlockTableRecord->setName((ACHAR*)_bstr_t(CXhChar16("MK")));
#else
		pTempBlockTableRecord->setName(CXhChar16("MK"));
#endif
		pTempBlockTable->add(blockId,pTempBlockTableRecord);
		pTempBlockTable->close();
		//���ɸ�ӡ���ͼԪ
		f3dPoint topLeft(0,nRectW),dim_pos(nRectL*0.5,nRectW*0.5,0);
		CreateAcadRect(pTempBlockTableRecord,topLeft,nRectL,nRectW);	//���������
		DimText(pTempBlockTableRecord,dim_pos,CXhChar16("��ӡ��"),TextStyleTable::hzfs.textStyleId,txt_height,0,AcDb::kTextCenter,AcDb::kTextVertMid);
		pTempBlockTableRecord->close();
	}
	//����ӡͼԪ����ͼ�飬��ʾ�û�����ָ��λ��
	AcDbBlockTableRecord* pBlockTableRecord=GetBlockTableRecord();
	while(1)
	{
		DRAGSET.ClearEntSet();
		f3dPoint insert_pos;
		AcDbObjectId newEntId;
		AcDbBlockReference *pBlkRef = new AcDbBlockReference;
		AcGeScale3d scaleXYZ(1.0,1.0,1.0);
		pBlkRef->setBlockTableRecord(blockId);
		pBlkRef->setPosition(AcGePoint3d(insert_pos.x,insert_pos.y,0));
		pBlkRef->setRotation(0);
		pBlkRef->setScaleFactors(scaleXYZ);
		DRAGSET.AppendAcDbEntity(pBlockTableRecord,newEntId,pBlkRef);
		pBlkRef->close();
#ifdef AFX_TARG_ENU_ENGLISH
		int retCode=DragEntSet(insert_pos,"input the insertion point\n");
#else
		int retCode=DragEntSet(insert_pos,"��������\n");
#endif
		if(retCode==RTCAN)
			break;
	}
	pBlockTableRecord->close();
#ifdef _ARX_2007
	ads_command(RTSTR,L"RE",RTNONE);
#else
	ads_command(RTSTR,"RE",RTNONE);
#endif
}
//////////////////////////////////////////////////////////////////////////
//ͨ����ȡTxt�ļ���������
//ReadVertexArrFromFile
//////////////////////////////////////////////////////////////////////////
bool ReadVertexArrFromFile(const char* filePath, fPtList &ptList)
{
	if (filePath == NULL)
		return false;
	FILE *fp = fopen(filePath, "rt");
	if (fp == NULL)
		return false;
	char line_txt[200] = { 0 };
	CXhChar200 sLine;
	f3dPoint *pPrevPt = NULL;
	while (!feof(fp))
	{
		if (fgets(line_txt, 200, fp) == NULL)
			continue;
		strcpy(sLine, line_txt);
		sLine.Replace('X', ' ');
		sLine.Replace('Y', ' ');
		sLine.Replace('I', ' ');
		sLine.Replace('J', ' ');
		strcpy(line_txt, sLine);
		char szTokens[] = " =\n";
		double x = 0, y = 0;
		char* skey = strtok(line_txt, szTokens);
		if (skey == NULL)
			continue;
		x = atof(skey);
		skey = strtok(NULL, szTokens);
		if (skey == NULL)
			y = 0;
		else
			y = atof(skey);
		f3dPoint *pPt = ptList.append();
		if (pPrevPt != NULL)
		{
			pPt->x = pPrevPt->x + x;
			pPt->y = pPrevPt->y + y;
		}
		else
		{
			pPt->x = x;
			pPt->y = y;
		}
		pPrevPt = pPt;
	}
	fclose(fp);
	return true;
}

void DrawProfileByTxtFile()
{	//1.��ȡ��������
	fPtList ptList;
	ReadVertexArrFromFile("D:\\Text.txt", ptList);
	ARRAY_LIST<f3dPoint> ptArr;
	for (f3dPoint *pPt = ptList.GetFirst(); pPt; pPt = ptList.GetNext())
	{
		ptArr.append(*pPt);
	}
	//2.��������
	DRAGSET.ClearEntSet();
	AcDbBlockTableRecord *pBlockTblRec = GetBlockTableRecord();
	CreateAcadPolyLine(pBlockTblRec, ptArr.Data(), ptArr.Count, 1);
	pBlockTblRec->close();
	ads_point insert_pos;
#ifdef AFX_TARG_ENU_ENGLISH
	int retCode = DragEntSet(insert_pos, "input the insertion point\n");
#else
	int retCode = DragEntSet(insert_pos, "��������\n");
#endif
}
//////////////////////////////////////////////////////////////////////////
//�����ı�,���ı�����ת��Ϊ�����
//ExplodeText
//////////////////////////////////////////////////////////////////////////
void ExplodeText()
{
	CLogErrorLife logErrLife;
	CAcModuleResourceOverride useThisRes;
	g_xDockBarManager.DisplayExplodeTxtDockBar();
}
#endif
//////////////////////////////////////////////////////////////////////////
//ϵͳ����
//EnvGeneralSet
//////////////////////////////////////////////////////////////////////////
void EnvGeneralSet()
{
	CAcModuleResourceOverride resOverride;
	CLogErrorLife logErrLife;
	CPNCSysSettingDlg dlg;
	dlg.DoModal();
}
//////////////////////////////////////////////////////////////////////////
//У�󹹼�������Ϣ
//RevisionPartProcess
//////////////////////////////////////////////////////////////////////////
#if defined(__UBOM_) || defined(__UBOM_ONLY_)
void RevisionPartProcess()
{
	CLogErrorLife logErrLife;
	if(g_xUbomModel.IsValidFunc(CBomModel::FUNC_DWG_COMPARE))
	{	//���ؽǸֹ��տ�
		char APP_PATH[MAX_PATH] = "", sJgCardPath[MAX_PATH] = "";
		GetAppPath(APP_PATH);
		sprintf(sJgCardPath, "%s%s", APP_PATH, (char*)g_pncSysPara.m_sJgCadName);
		if (!g_pncSysPara.InitJgCardInfo(sJgCardPath))
		{
			logerr.Log(CXhChar200("�Ǹֹ��տ���ȡʧ��(%s)!", sJgCardPath));
			return;
		}
	}
	//��ʾ�Ի���
	int nWidth = g_xUbomModel.InitBomTitle();
	g_xDockBarManager.DisplayRevisionDockBar(nWidth);
	CRevisionDlg* pRevisionDlg = g_xDockBarManager.GetRevisionDlgPtr();
	if (pRevisionDlg)
		pRevisionDlg->DisplayProcess = DisplayProcess;
}
#endif