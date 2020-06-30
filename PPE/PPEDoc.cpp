// PPEDoc.cpp : implementation of the CPPEDoc class
//

#include "stdafx.h"
#include "PPE.h"
#include "PPEDoc.h"
#include "PPEView.h"
#include "ProcessPart.h"
#include "NewPartFileDlg.h"
#include "PartLib.h"
#include "MainFrm.h"
#include "PartTreeDlg.h"
#include "SysPara.h"
#include "PPEModel.h"
#include "NcPart.h"
#include "folder_dialog.h"
#include "LicFuncDef.h"
#include "ParseAdaptNo.h"
#include "folder_dialog.h"
#include "direct.h"
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////
//
CXhChar200 GetValidFileName(CProcessPart *pPart, const char* sPartNoPrefix = NULL)
{
	CXhChar200 sFileName;
	CXhChar500 sFilePath = model.GetPartFilePath(pPart->GetPartNo());
	if (model.file_format.IsValidFormat() && pPart->IsPlate())
	{	//�����û����ƣ���ȡ�ļ���
		CProcessPlate* pPlate = (CProcessPlate*)pPart;
		size_t nSplit = model.file_format.m_sSplitters.size();
		size_t nKeySize = model.file_format.m_sKeyMarkArr.size();
		for (size_t i = 0; i < nKeySize; i++)
		{
			CXhChar50 sValue;
			if (model.file_format.m_sKeyMarkArr[i].Equal(CPPEModel::KEY_PART_NO))
				sValue.Copy(pPart->GetPartNo());
			else if (model.file_format.m_sKeyMarkArr[i].Equal(CPPEModel::KEY_PART_MAT))
				CProcessPart::QuerySteelMatMark(pPart->cMaterial, sValue);
			else if (model.file_format.m_sKeyMarkArr[i].Equal(CPPEModel::KEY_PART_THICK) &&
				pPlate->GetThick() > 0)
				sValue.Printf("%.0f", pPlate->GetThick());
			else if (model.file_format.m_sKeyMarkArr[i].Equal(CPPEModel::KEY_SINGLE_NUM) &&
				pPlate->m_nSingleNum > 0)
				sValue.Printf("%d", pPlate->m_nSingleNum);
			else if (model.file_format.m_sKeyMarkArr[i].Equal(CPPEModel::KEY_PROCESS_NUM) &&
				pPlate->m_nProcessNum > 0)
				sValue.Printf("%d", pPlate->m_nProcessNum);
			else if (model.file_format.m_sKeyMarkArr[i].Equal(CPPEModel::KEY_TA_TYPE) &&
				model.m_sTaType.GetLength() > 0)
				sValue.Copy(model.m_sTaType);
			//
			if (sValue.GetLength() > 0)
			{
				if (sFileName.GetLength() > 0 && nSplit >= i &&
					model.file_format.m_sSplitters[i - 1].GetLength() > 0)
					sFileName.Append(model.file_format.m_sSplitters[i - 1]);
				sFileName.Append(sValue);
			}
		}
		//�ļ���ʽĩβ������ָ���
		if (nKeySize > 0 && nSplit == nKeySize && model.file_format.m_sSplitters[nSplit - 1].GetLength() > 0)
			sFileName.Append(model.file_format.m_sSplitters[nSplit - 1]);
	}
	else if (sFilePath.Length() > 0)
	{	//����ppi�ļ���ȡ·������ȡ�ļ���
		char drive[8], dir[MAX_PATH], fname[MAX_PATH], ext[10];
		_splitpath(sFilePath, drive, dir, fname, ext);
		sFileName.Copy(fname);
	}
	else
	{	//���ݹ������������ļ���
		CString sSpec = pPart->GetSpec();
		if (sPartNoPrefix)
			sFileName.Printf("%s%s#%s", sPartNoPrefix, (char*)pPart->GetPartNo(), sSpec.Trim());
		else
			sFileName.Printf("%s#%s", (char*)pPart->GetPartNo(), sSpec.Trim());
	}
	return sFileName;
}

void MakeDirectory(char *path)
{
	char bak_path[MAX_PATH], drive[MAX_PATH];
	strcpy(bak_path, path);
	char *dir = strtok(bak_path, "/\\");
	if (strlen(dir) == 2 && dir[1] == ':')
	{
		strcpy(drive, dir);
		strcat(drive, "\\");
		_chdir(drive);
		dir = strtok(NULL, "/\\");
	}
	while (dir)
	{
		_mkdir(dir);
		_chdir(dir);
		dir = strtok(NULL, "/\\");
	}
}
CXhChar500 GetValidWorkDir(const char* work_dir, const char* sub_folder = NULL)
{
	CXhChar500 sWorkDir;
	if (work_dir == NULL || strlen(work_dir) <= 0 || access(work_dir, 0) == -1)
	{	//ָ������·��
		char ss[MAX_PATH];
		GetSysPath(ss);
		CString sFolder(ss);
		if (!InvokeFolderPickerDlg(sFolder))
			return sWorkDir;
		sWorkDir.Copy(sFolder);
	}
	else
		sWorkDir.Copy(work_dir);
	sWorkDir.Append("\\");
	if (model.m_sTaType.Length() > 0)
	{
		sWorkDir.Append(model.m_sTaType);
		sWorkDir.Append("\\");
		if (access(sWorkDir, 0) == -1)
			MakeDirectory(sWorkDir);
	}
	if (sub_folder != NULL && strlen(sub_folder) > 0)
	{
		sWorkDir.Append(sub_folder);
		sWorkDir.Append("\\");
		if (access(sWorkDir, 0) == -1)
			MakeDirectory(sWorkDir);
	}
	return sWorkDir;
}
//
static bool CompareProcessPart(CProcessPart *pOrgPart, CProcessPart *pCurPart)
{
	if (pOrgPart->IsEqual(pCurPart))
	{
		if (!pOrgPart->mkpos.IsEqual(pCurPart->mkpos.x, pCurPart->mkpos.y, pCurPart->mkpos.z))
			return false;
		else if (pOrgPart->IsPlate() && ((CProcessPlate*)pOrgPart)->mcsFlg.wFlag != ((CProcessPlate*)pCurPart)->mcsFlg.wFlag)
			return false;
		else
			return true;
	}
	else
		return false;
}
static void GetNeedReturnPartSet(CXhPtrSet<CProcessPart> *pPartSet)
{
	CPPEModel orgModel;
	theApp.m_xPPEModelBuffer.SeekToBegin();
	orgModel.FromBuffer(theApp.m_xPPEModelBuffer);
	for (CProcessPart *pPart = model.EnumPartFirst(); pPart; pPart = model.EnumPartNext())
	{
		CProcessPart *pOrgPart = orgModel.FromPartNo(pPart->GetPartNo());
		if (pPart->m_dwInheritPropFlag == CProcessPart::PATTERN_OVERWRITE ||	//����ģʽ��������洢
			(pPart->m_dwInheritPropFlag > 0 &&
			(pOrgPart == NULL || !CompareProcessPart(pOrgPart, pPart))))
			pPartSet->append(pPart);
	}
}
char restore_JG_guige(char* guige, double &wing_wide, double &wing_thick)
{
	char mark, material;	//symbol='L' mark='X';
	sscanf(guige, "%lf%c%lf%c", &wing_wide, &mark, &wing_thick, &material);
	material = (char)toupper(material);//�Ǹֲ���A3(S)��Q345(H)

	return material;
}
/////////////////////////////////////////////////////////////////////////////
// CPPEDoc
IMPLEMENT_DYNCREATE(CPPEDoc, CDocument)

BEGIN_MESSAGE_MAP(CPPEDoc, CDocument)
	//{{AFX_MSG_MAP(CPPEDoc)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_COMMAND(ID_FILE_SAVE_DXF, OnFileSaveDxf)
	ON_COMMAND(ID_FILE_SAVE_TTP, OnFileSaveTtp)
	ON_COMMAND(ID_FILE_SAVE_WKF, OnFileSaveWkf)
	ON_COMMAND(ID_FILE_SAVE_PBJ, OnFileSavePbj)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_PBJ,OnUpdateFileSavePbj)
	ON_COMMAND(ID_FILE_SAVE_PMZ, OnFileSavePmz)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_PMZ,OnUpdateFileSavePmz)
	ON_COMMAND(ID_FILE_SAVE_TXT,OnFileSaveTxt)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_TXT,OnUpdateFileSaveTxt)
	ON_COMMAND(ID_FILE_SAVE_NC,OnFileSaveNc)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_NC,OnUpdateFileSaveNc)
	ON_COMMAND(ID_FILE_SAVE_CNC,OnFileSaveCnc)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_CNC,OnUpdateFileSaveCnc)
	ON_COMMAND(ID_GEN_ANGLE_NC_FILE, &CPPEDoc::OnGenAngleNcFile)
	ON_UPDATE_COMMAND_UI(ID_GEN_ANGLE_NC_FILE,OnUpdateGenAngleNcFile)
	ON_COMMAND(ID_NEW_FILE, &CPPEDoc::OnNewFile)
	ON_COMMAND(ID_FLAME_NC_DATA, OnCreateFlameNcData)
	ON_COMMAND(ID_PLASMA_NC_DATA, OnCreatePlasmaNcData)
	ON_COMMAND(ID_PUNCH_NC_DATA, OnCreatePunchNcData)
	ON_COMMAND(ID_DRILL_NC_DATA, OnCreateDrillNcData)
	ON_COMMAND(ID_LASER_NC_DATA, OnCreateLaserNcData)
	ON_COMMAND(ID_CREATE_PLATE_NC_DATA, OnCreatePlateNcData)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPPEDoc construction/destruction

CPPEDoc::CPPEDoc()
{
	
}

CPPEDoc::~CPPEDoc()
{
}

BOOL CPPEDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)
	SetTitle("�����༭��");
	return TRUE;
}

void CPPEDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CPPEDoc diagnostics

#ifdef _DEBUG
void CPPEDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CPPEDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPPEDoc commands

CView* CPPEDoc::GetView(const CRuntimeClass *pClass)
{
	CView *pView;
	POSITION position;
	position = GetFirstViewPosition();
	for(;;)
	{
		if(position==NULL)
		{
			pView = NULL;
			break;
		}
		pView = GetNextView(position);
		if(pView->IsKindOf(pClass))
			break;
	}
	return pView;
}

void CPPEDoc::OpenFolder(const char* sFolderPath)
{
	if (strlen(sFolderPath) <= 0)
		return;
	model.InitModelByFolderPath(sFolderPath);
	CPartTreeDlg *pPartTreeDlg = ((CMainFrame*)AfxGetMainWnd())->GetPartTreePage();
	if (pPartTreeDlg)
		pPartTreeDlg->InitTreeView();
	//���ļ���֮�󣬸������ó�ʼ���׾�����ֵ wht 19-04-09
	CPPEView *pPPEView = theApp.GetView();
	if (pPPEView)
	{	//����ǰ��������Ϊ�գ����������򿪶���ļ��е�������
		pPPEView->SetCurProcessPart(NULL);
	}
}
void CPPEDoc::OnFileOpen() 
{
	CString sFolder= GetPathName();
	if (sFolder.GetLength() <= 0)
		sFolder.Format("%s", APP_PATH);
	if (InvokeFolderPickerDlg(sFolder))
	{
		OpenFolder(sFolder);
		SetPathName(sFolder);
	}
}
void CPPEDoc::OnNewFile()
{
	CNewPartFileDlg newFileDlg;
	if (newFileDlg.DoModal() != IDOK)
		return;
	CXhChar500 sFolderPath = GetValidWorkDir(model.GetFolderPath());
	CProcessAngle processAngle;
	CProcessPlate processPlate;
	CProcessPart *pTempPart = NULL;
	if (newFileDlg.m_bImprotNcFile)
	{
#ifdef __PNC_
		processPlate.cMaterial = 'S';
		pTempPart = &processPlate;
		CFileDialog dlg(TRUE, "cnc", NULL, NULL, "TXT�ļ�(*.txt)|*.txt|CNC�ļ�(*.cnc)|*.cnc|All Files (*.*)|*.*||");
		if (dlg.DoModal() != IDOK)
			return;
		CNCPart::ImprotPlateCncOrTextFile(&processPlate, dlg.GetPathName());
#endif
	}
	else
	{
		//1����ʼ�����սǸ���Ϣ
		pTempPart = &processAngle;
		processAngle.SetPartNo(newFileDlg.m_sPartNo.GetBuffer());
		processAngle.cMaterial = QuerySteelBriefMatMark(newFileDlg.m_sMaterial.GetBuffer());
		double fWidth = 0, fThick = 0, fLen = 0;
		restore_JG_guige(newFileDlg.m_sSpec.GetBuffer(), fWidth, fThick);
		processAngle.m_fWidth = (float)fWidth;
		processAngle.m_fThick = (float)fThick;
		processAngle.m_wLength = (WORD)atof(newFileDlg.m_sLen);
		CXhChar100 sSpec("L%s", newFileDlg.m_sSpec);
		processAngle.SetSpec(sSpec);
	}
	//2�����Ǹ���Ϣ���浽ppi�ļ���
	CString sFileName;
	sFileName.Format("%s#%s%c.ppi", (char*)pTempPart->GetPartNo(), (char*)pTempPart->GetSpec(), pTempPart->cMaterial);
	CXhChar200 sFilePath("%s%s", (char*)sFolderPath, sFileName);
	CProcessPart *pNewPart = model.AddPart(pTempPart->GetPartNo(), pTempPart->m_cPartType, sFilePath);
	pTempPart->ClonePart(pNewPart);
	CNCPart::CreatePPIFile(pNewPart, sFilePath);
	//3��ˢ�½���
	CPartTreeDlg *pPartTreeDlg = ((CMainFrame*)AfxGetMainWnd())->GetPartTreePage();
	if (pPartTreeDlg)
		pPartTreeDlg->InsertTreeItem(sFileName.GetBuffer(), pNewPart);
	CPPEView *pView = theApp.GetView();
	if (pView)
	{
		pView->UpdateCurWorkPartByPartNo(pNewPart->GetPartNo());
		pView->Refresh();
		pView->UpdatePropertyPage();
	}
}
BOOL CPPEDoc::WriteToParentProcess()
{
	HANDLE hPipeWrite=NULL;
	hPipeWrite= GetStdHandle( STD_OUTPUT_HANDLE );

	if( hPipeWrite == INVALID_HANDLE_VALUE )
	{
		AfxMessageBox("��ȡ�ܵ������Ч\n");
		return FALSE;
	}
	CProcessPart *pProcessPart=NULL;
	CPPEView *pView=(CPPEView*)GetView(RUNTIME_CLASS(CPPEView));
	if(pView==NULL||(pProcessPart=pView->GetCurProcessPart())==NULL)
		return FALSE;
	CBuffer buffer(10000);	//10kb
	//�������ܵ���д�����ݷ��ظ�������
	buffer.WriteByte(1);		//���ݼ�����ʶ
	buffer.WriteInteger(0);		//���չ������泤��
	if(!theApp.starter.IsMultiPartsMode())
	{
		buffer.WriteDword(1);
		pProcessPart->ToPPIBuffer(buffer);
	}
	else
	{
		CXhPtrSet<CProcessPart> partSet;
		GetNeedReturnPartSet(&partSet);
		DWORD dwPartNum=partSet.GetNodeNum();
		buffer.WriteDword((DWORD)dwPartNum);
		for(CProcessPart *pPart=partSet.GetFirst();pPart;pPart=partSet.GetNext())
			pPart->ToPPIBuffer(buffer);
	}
	buffer.SeekPosition(1);		//д��ʵ�ʹ��չ������泤��
	buffer.WriteInteger(buffer.GetLength()-5);
	buffer.WriteToPipe(hPipeWrite,1024);
	((CMainFrame*)theApp.m_pMainWnd)->CloseProcess();
	return TRUE;
}
void CPPEDoc::OnFileSave() 
{
	if(theApp.starter.IsDuplexMode())
		WriteToParentProcess();
}
//////////////////////////////////////////////////////////////////////////
//�ļ�����
void CPPEDoc::InitPlateGroupByThickMat(CHashStrList<PLATE_GROUP> &hashPlateByThickMat)
{
	hashPlateByThickMat.Empty();
	for (CProcessPart *pPart = model.EnumPartFirst(); pPart; pPart = model.EnumPartNext())
	{
		if (!pPart->IsPlate())
			continue;
		int thick = (int)(pPart->GetThick());
		CXhChar16 sMat;
		QuerySteelMatMark(pPart->cMaterial, sMat);
		CXhChar50 sKey("%s_%d", (char*)sMat, thick);
		PLATE_GROUP* pPlateGroup = hashPlateByThickMat.GetValue(sKey);
		if (pPlateGroup == NULL)
		{
			pPlateGroup = hashPlateByThickMat.Add(sKey);
			pPlateGroup->thick = thick;
			pPlateGroup->cMaterial = pPart->cMaterial;
			pPlateGroup->sKey.Copy(sKey);
		}
		pPlateGroup->plateSet.append((CProcessPlate*)pPart);
	}
}
CXhChar16 CPPEDoc::GetSubFolder(int iNcMode)
{
	CXhChar16 sSubFolder;
	if (CNCPart::FLAME_MODE == iNcMode)
		sSubFolder.Copy("�����и�");
	else if (CNCPart::PLASMA_MODE == iNcMode)
		sSubFolder.Copy("�������и�");
	else if (CNCPart::PUNCH_MODE == iNcMode)
		sSubFolder.Copy("�崲�ӹ�");
	else if (CNCPart::DRILL_MODE == iNcMode)
		sSubFolder.Copy("�괲�ӹ�");
	else if (CNCPart::LASER_MODE == iNcMode)
		sSubFolder.Copy("���⸴�ϻ�");
	return sSubFolder;
}
CXhChar16 CPPEDoc::GetFileSuffix(int iNcFileType)
{
	CXhChar16 sNcExt;
	if (CNCPart::PLATE_DXF_FILE == iNcFileType)		//�ְ�DXF�����ļ�
		sNcExt.Copy("dxf");
	else if (CNCPart::PLATE_NC_FILE == iNcFileType)	//�ְ�NC�����ļ�
		sNcExt.Copy("nc");
	else if (CNCPart::PLATE_TXT_FILE == iNcFileType)	//�ְ�TXT�����ļ�
		sNcExt.Copy("txt");
	else if (CNCPart::PLATE_CNC_FILE == iNcFileType)	//�ְ�CNC�����ļ�
		sNcExt.Copy("cnc");
	else if (CNCPart::PLATE_PMZ_FILE == iNcFileType) //�ְ�PMZ�����ļ�
		sNcExt.Copy("pmz");
	else if (CNCPart::PLATE_PBJ_FILE == iNcFileType) //�ְ�PBJ�����ļ�
		sNcExt.Copy("pbj");
	else if (CNCPart::PLATE_TTP_FILE == iNcFileType)	//�ְ�TTP�����ļ�
		sNcExt.Copy("ttp");
	else if (CNCPart::PLATE_WKF_FILE == iNcFileType)	//���Ϸ���WKF�ļ�
		sNcExt.Copy("wkf");
	return sNcExt;
}
bool CPPEDoc::CreatePlateNcFiles(CHashStrList<PLATE_GROUP> &hashPlateByThickMat, char* thickSetStr,
	const char* mainFolder, int iNcMode, int iNcFileType, BOOL bIsPmzCheck/*=FALSE*/)
{
	CXhChar16 sSubFolder = GetSubFolder(iNcMode);
	CXhChar16 sNcExt = GetFileSuffix(iNcFileType);
	if (sSubFolder.GetLength() <= 0 || sNcExt.GetLength() <= 0)
		return false;
	CXhChar16 sNcFileFolder = sNcExt;
	sNcFileFolder.ToUpper();
	if (CNCPart::PLATE_PMZ_FILE == iNcFileType && bIsPmzCheck)
		sNcFileFolder.Append("Ԥ��");	//�ְ�PMZԤ���ļ�
	CHashList<SEGI> thickHash;
	if (thickSetStr)
		GetSegNoHashTblBySegStr(thickSetStr, thickHash);
	//
	CXhChar500 sNcFileDir, sFilePath;
	CXhChar100 sTitle("����%s��%s�ļ�����", (char*)sSubFolder, (char*)sNcFileFolder);
	int index = 0, num = model.PartCount();
	model.DisplayProcess(index, sTitle);
	for (PLATE_GROUP *pPlateGroup = hashPlateByThickMat.GetFirst(); pPlateGroup; pPlateGroup = hashPlateByThickMat.GetNext())
	{
		if (thickHash.GetNodeNum() > 0 && thickHash.GetValue(pPlateGroup->thick) == NULL)
			continue;
		if (g_sysPara.nc.m_iDxfMode == 1)
		{	//����ȴ����ļ�Ŀ¼
			CXhChar16 sMat;
			QuerySteelMatMark(pPlateGroup->cMaterial, sMat);
			sNcFileDir.Printf("%s\\%s\\%s\\���-%d-%s", (char*)mainFolder, (char*)sSubFolder, (char*)sNcFileFolder, pPlateGroup->thick, (char*)sMat);
		}
		else
			sNcFileDir.Printf("%s\\%s\\%s", (char*)mainFolder, (char*)sSubFolder, (char*)sNcFileFolder);
		if (access(sNcFileDir, 0) == -1)
			MakeDirectory(sNcFileDir);
		for (CProcessPlate *pSrcPlate = pPlateGroup->plateSet.GetFirst(); pSrcPlate; pSrcPlate = pPlateGroup->plateSet.GetNext())
		{
			index++;
			model.DisplayProcess(ftoi(100 * index / num), sTitle);
			CProcessPlate *pPlate = (CProcessPlate*)model.FromPartNo(pSrcPlate->GetPartNo(), iNcMode);
			CXhChar100  sFileName = GetValidFileName(pSrcPlate);
			if (sFileName.Length() <= 0 || pPlate == NULL)
				continue;
			sFilePath.Printf("%s\\%s.%s", (char*)sNcFileDir, (char*)sFileName, (char*)sNcExt);
			if (CNCPart::PLATE_DXF_FILE == iNcFileType)			//�ְ�DXF�����ļ�
				CNCPart::CreatePlateDxfFile(pPlate, sFilePath, iNcMode);
#ifdef __PNC_
			else if (CNCPart::PLATE_NC_FILE == iNcFileType)		//�ְ�NC�����ļ�
				CNCPart::CreatePlateNcFile(pPlate, sFilePath);
			else if (CNCPart::PLATE_TXT_FILE == iNcFileType)	//�ְ�TXT�����ļ�
				CNCPart::CreatePlateTxtFile(pPlate, sFilePath);
			else if (CNCPart::PLATE_CNC_FILE == iNcFileType)	//�ְ�CNC�����ļ�
				CNCPart::CreatePlateCncFile(pPlate, sFilePath);
			else if (CNCPart::PLATE_PMZ_FILE == iNcFileType)	//�ְ�PMZ�����ļ�
			{
				if (bIsPmzCheck)
					CNCPart::CreatePlatePmzCheckFile(pPlate, sFilePath);
				else
					CNCPart::CreatePlatePmzFile(pPlate, sFilePath);
			}
			else if (CNCPart::PLATE_PBJ_FILE == iNcFileType)	//�ְ�PBJ�����ļ�
				CNCPart::CreatePlatePbjFile(pPlate, sFilePath);
#endif
			else if (CNCPart::PLATE_TTP_FILE == iNcFileType)	//�ְ�TTP�����ļ�
				CNCPart::CreatePlateTtpFile(pPlate, sFilePath);
			else if (CNCPart::PLATE_WKF_FILE == iNcFileType)	//���Ϸ���WKF�ļ�
				CNCPart::CreatePlateWkfFile(pPlate, sFilePath);
		}
	}
	model.DisplayProcess(100, sTitle);
	return true;
}
void CPPEDoc::CreatePlateNcFiles(int iFileType)
{
#ifdef __PNC_
	BYTE iNcMode = 0;
	if (iFileType == CNCPart::PLATE_TXT_FILE ||
		iFileType == CNCPart::PLATE_CNC_FILE)
		iNcMode = CNCPart::FLAME_MODE;	//�����и�
	else if (iFileType == CNCPart::PLATE_NC_FILE)
		iNcMode = CNCPart::PLASMA_MODE;	//�������и�
	else if (iFileType == CNCPart::PLATE_PBJ_FILE ||
		iFileType == CNCPart::PLATE_WKF_FILE||
		iFileType == CNCPart::PLATE_TTP_FILE)
		iNcMode = CNCPart::PUNCH_MODE;	//�崲�ӹ�
	else if (iFileType == CNCPart::PLATE_PMZ_FILE)
		iNcMode = CNCPart::DRILL_MODE;	//�괲�ӹ�	
	else
		return;
	CXhChar100 sThick;
	if (iNcMode == CNCPart::FLAME_MODE)
		sThick = g_sysPara.nc.m_xFlamePara.m_sThick;
	else if (iNcMode == CNCPart::PLASMA_MODE)
		sThick = g_sysPara.nc.m_xPlasmaPara.m_sThick;
	else if (iNcMode == CNCPart::PUNCH_MODE)
		sThick = g_sysPara.nc.m_xPunchPara.m_sThick;
	else if (iNcMode == CNCPart::DRILL_MODE)
		sThick = g_sysPara.nc.m_xDrillPara.m_sThick;
	//���ݰ��Ըְ���з���
	CHashStrList<PLATE_GROUP> hashPlateByThickMat;
	InitPlateGroupByThickMat(hashPlateByThickMat);
	if (hashPlateByThickMat.GetNodeNum() <= 0)
		return;
	//���ɸְ�����NC����
	CXhChar500 sFolder = model.GetFolderPath();
	CNCPart::m_bDeformedProfile = TRUE;	//PNC��ȡ�ĸְ�Ĭ���ѿ��ǻ�������
	CreatePlateNcFiles(hashPlateByThickMat, sThick, sFolder, iNcMode, iFileType);
	if (CNCPart::PLATE_PMZ_FILE == iFileType && g_sysPara.pmz.m_bPmzCheck)	//����괲�ӹ�PMZԤ���ʽ�ļ� wht 19-07-02
		CreatePlateNcFiles(hashPlateByThickMat, g_sysPara.nc.m_xDrillPara.m_sThick, sFolder, iNcMode, CNCPart::PLATE_PMZ_FILE, TRUE);
	//
	CXhChar16 sSubFolder = GetSubFolder(iNcMode);
	CXhChar16 sNcExt = GetFileSuffix(iFileType);
	sNcExt.ToUpper();
	CXhChar500 sNcFileDir("%s\\%s\\%s", (char*)sFolder, (char*)sSubFolder, (char*)sNcExt);
	ShellExecute(NULL, "open", NULL, NULL, sNcFileDir, SW_SHOW);
#endif
}
void CPPEDoc::CreatePlateDxfFiles()
{
	//PNC�û�֧�ֶ���ģʽ�µ�DXF�ļ�
	CHashStrList<PLATE_GROUP> hashPlateByThickMat;
	InitPlateGroupByThickMat(hashPlateByThickMat);
	if (hashPlateByThickMat.GetNodeNum() <= 0)
		return;
	CXhChar500 sFolder = model.GetFolderPath();
	CNCPart::m_bDeformedProfile = TRUE;	//PNC��ȡ�ĸְ�Ĭ���ѿ��ǻ�������
	if (g_sysPara.IsValidNcFlag(CNCPart::FLAME_MODE))
		CreatePlateNcFiles(hashPlateByThickMat, g_sysPara.nc.m_xFlamePara.m_sThick, sFolder, CNCPart::FLAME_MODE, CNCPart::PLATE_DXF_FILE);
	if (g_sysPara.IsValidNcFlag(CNCPart::PLASMA_MODE))
		CreatePlateNcFiles(hashPlateByThickMat, g_sysPara.nc.m_xPlasmaPara.m_sThick, sFolder, CNCPart::PLASMA_MODE, CNCPart::PLATE_DXF_FILE);
	if (g_sysPara.IsValidNcFlag(CNCPart::PUNCH_MODE))
		CreatePlateNcFiles(hashPlateByThickMat, g_sysPara.nc.m_xPunchPara.m_sThick, sFolder, CNCPart::PUNCH_MODE, CNCPart::PLATE_DXF_FILE);
	if (g_sysPara.IsValidNcFlag(CNCPart::DRILL_MODE))
		CreatePlateNcFiles(hashPlateByThickMat, g_sysPara.nc.m_xDrillPara.m_sThick, sFolder, CNCPart::DRILL_MODE, CNCPart::PLATE_DXF_FILE);
	if (g_sysPara.IsValidNcFlag(CNCPart::LASER_MODE))
		CreatePlateNcFiles(hashPlateByThickMat, NULL, sFolder, CNCPart::LASER_MODE, CNCPart::PLATE_DXF_FILE);
	ShellExecute(NULL, "open", NULL, NULL, sFolder, SW_SHOW);
}
void CPPEDoc::CreatePlateFiles(int iFileType)
{
	CXhChar16 sExt;
	if (iFileType == CNCPart::PLATE_DXF_FILE)
		sExt.Copy("dxf");
	else if (iFileType == CNCPart::PLATE_TTP_FILE)
		sExt.Copy("ttp");
	else if (iFileType == CNCPart::PLATE_WKF_FILE)
		sExt.Copy("wkf");
	else
		return;
	CXhChar16 sFileFolder = sExt;
	sFileFolder.ToUpper();
	CXhChar200 sWorkDir = GetValidWorkDir(model.GetFolderPath(), sFileFolder);
	if (sWorkDir.Length() <= 0)
		return;
	for (CProcessPart *pPart = model.EnumPartFirst(); pPart; pPart = model.EnumPartNext())
	{
		if (!pPart->IsPlate())
			continue;
		CProcessPlate* pPlate = (CProcessPlate*)pPart;
		CXhChar200 sFileName = GetValidFileName(pPlate);
		if (sFileName.Length() <= 0)
			continue;
		CXhChar200 sFilePath("%s\\%s.%s", (char*)sWorkDir, (char*)sFileName,(char*)sExt);
		if (iFileType == CNCPart::PLATE_DXF_FILE)
			CNCPart::CreatePlateDxfFile(pPlate, sFilePath, CNCPart::PUNCH_MODE);
		else if (iFileType == CNCPart::PLATE_TTP_FILE)
			CNCPart::CreatePlateTtpFile(pPlate, sFilePath);
		else if (iFileType == CNCPart::PLATE_WKF_FILE)
			CNCPart::CreatePlateWkfFile(pPlate, sFilePath);
	}
	ShellExecute(NULL, "open", NULL, NULL, sWorkDir, SW_SHOW);
}
//
void CPPEDoc::OnFileSaveDxf()
{
	CNCPart::m_bDeformedProfile=TRUE;
#ifdef __PNC_
	CreatePlateDxfFiles();
#else
	if (AfxMessageBox("DXF�ļ����Ƿ��ǻ�������", MB_YESNO) == IDYES)
		CNCPart::m_bDeformedProfile = TRUE;
	else
		CNCPart::m_bDeformedProfile = FALSE;
	CreatePlateFiles(CNCPart::PLATE_DXF_FILE);
#endif
}
void CPPEDoc::OnFileSaveTtp() 
{
#ifdef __PNC_
	CreatePlateNcFiles(CNCPart::PLATE_TTP_FILE);
#else
	CreatePlateFiles(CNCPart::PLATE_TTP_FILE);
#endif
}
void CPPEDoc::OnFileSaveWkf()
{
#ifdef __PNC_
	CreatePlateNcFiles(CNCPart::PLATE_WKF_FILE);
#else
	CreatePlateFiles(CNCPart::PLATE_WKF_FILE);
#endif
}

void CPPEDoc::OnFileSavePbj()
{
#ifdef __PNC_
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_PBJ_FILE))
		return;
	CreatePlateNcFiles(CNCPart::PLATE_PBJ_FILE);
#endif
}
void CPPEDoc::OnUpdateFileSavePbj(CCmdUI *pCmdUI)
{
	BOOL bEnable=FALSE;
#ifdef __PNC_
	if(VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_PBJ_FILE))
		bEnable=TRUE;
#endif
	pCmdUI->Enable(bEnable);
}
void CPPEDoc::OnFileSavePmz()
{
#ifdef __PNC_
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_PBJ_FILE))
		return;
	CreatePlateNcFiles(CNCPart::PLATE_PMZ_FILE);
#endif
}
void CPPEDoc::OnUpdateFileSavePmz(CCmdUI *pCmdUI)
{
	BOOL bEnable=FALSE;
#ifdef __PNC_
	if(VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_PBJ_FILE))
		bEnable=TRUE;
#endif
	pCmdUI->Enable(bEnable);
}
void CPPEDoc::OnFileSaveTxt()
{
#ifdef __PNC_
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
		return;
	CreatePlateNcFiles(CNCPart::PLATE_TXT_FILE);
#endif
}
void CPPEDoc::OnUpdateFileSaveTxt(CCmdUI *pCmdUI)
{
	BOOL bEnable=FALSE;
#ifdef __PNC_
	if(VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
		bEnable=TRUE;
#endif
	pCmdUI->Enable(bEnable);
}
void CPPEDoc::OnFileSaveNc()
{
#ifdef __PNC_
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
		return;
	CreatePlateNcFiles(CNCPart::PLATE_NC_FILE);
#endif
}
void CPPEDoc::OnUpdateFileSaveNc(CCmdUI *pCmdUI)
{
	BOOL bEnable=FALSE;
#ifdef __PNC_
	if(VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
		bEnable=TRUE;
#endif
	pCmdUI->Enable(bEnable);
}
void CPPEDoc::OnFileSaveCnc()
{
#ifdef __PNC_
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
		return;
	CreatePlateNcFiles(CNCPart::PLATE_CNC_FILE);
#endif
}
void CPPEDoc::OnUpdateFileSaveCnc(CCmdUI *pCmdUI)
{
	BOOL bEnable=FALSE;
#ifdef __PNC_
	if(VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
		bEnable=TRUE;
#endif
	pCmdUI->Enable(bEnable);
}
//�����и�����NC����
void CPPEDoc::OnCreateFlameNcData()
{
#ifdef __PNC_
	if (!g_sysPara.IsValidNcFlag(CNCPart::FLAME_MODE))
	{
		AfxMessageBox("δ���û����и����ģʽ������ϵͳ����=>���=>�������������[�ְ�NCģʽ]��");
		return;
	}
	//���ݰ��Ըְ���з���
	CHashStrList<PLATE_GROUP> hashPlateByThickMat;
	InitPlateGroupByThickMat(hashPlateByThickMat);
	if (hashPlateByThickMat.GetNodeNum() <= 0)
		return;
	//ָ�����·��
	CXhChar500 sFolder = model.GetFolderPath();
	CString ss(sFolder);
	if (InvokeFolderPickerDlg(ss))
		sFolder.Copy(ss);
	//���ɸְ�����NC����
	CNCPart::m_bDeformedProfile = TRUE;	//PNC��ȡ�ĸְ�Ĭ���ѿ��ǻ�������
	CXhChar100 sThick = g_sysPara.nc.m_xFlamePara.m_sThick;
	if (g_sysPara.nc.m_xFlamePara.IsValidFile(CNCPart::PLATE_TXT_FILE))
		CreatePlateNcFiles(hashPlateByThickMat, sThick, sFolder, CNCPart::FLAME_MODE, CNCPart::PLATE_TXT_FILE);
	if (g_sysPara.nc.m_xFlamePara.IsValidFile(CNCPart::PLATE_CNC_FILE))
		CreatePlateNcFiles(hashPlateByThickMat, sThick, sFolder, CNCPart::FLAME_MODE, CNCPart::PLATE_CNC_FILE);
	if (g_sysPara.nc.m_xFlamePara.IsValidFile(CNCPart::PLATE_DXF_FILE))
		CreatePlateNcFiles(hashPlateByThickMat, sThick, sFolder, CNCPart::FLAME_MODE, CNCPart::PLATE_DXF_FILE);
	ShellExecute(NULL, "open", NULL, NULL, sFolder, SW_SHOW);
#endif
}
void CPPEDoc::OnCreatePlasmaNcData()
{
#ifdef __PNC_
	if (!g_sysPara.IsValidNcFlag(CNCPart::PLASMA_MODE))
	{
		AfxMessageBox("δ���õ������и����ģʽ������ϵͳ����=>���=>�������������[�ְ�NCģʽ]��");
		return;
	}
	//���ݰ��Ըְ���з���
	CHashStrList<PLATE_GROUP> hashPlateByThickMat;
	InitPlateGroupByThickMat(hashPlateByThickMat);
	if (hashPlateByThickMat.GetNodeNum() <= 0)
		return;
	//ָ�����·��
	CXhChar500 sFolder = model.GetFolderPath();
	CString ss(sFolder);
	if (InvokeFolderPickerDlg(ss))
		sFolder.Copy(ss);
	//���ɸְ�����NC����
	CNCPart::m_bDeformedProfile = TRUE;	//PNC��ȡ�ĸְ�Ĭ���ѿ��ǻ�������
	CXhChar100 sThick = g_sysPara.nc.m_xPlasmaPara.m_sThick;
	if (g_sysPara.nc.m_xPlasmaPara.IsValidFile(CNCPart::PLATE_NC_FILE))
		CreatePlateNcFiles(hashPlateByThickMat, sThick, sFolder, CNCPart::PLASMA_MODE, CNCPart::PLATE_NC_FILE);
	if (g_sysPara.nc.m_xPlasmaPara.IsValidFile(CNCPart::PLATE_DXF_FILE))
		CreatePlateNcFiles(hashPlateByThickMat, sThick, sFolder, CNCPart::PLASMA_MODE, CNCPart::PLATE_DXF_FILE);
	ShellExecute(NULL, "open", NULL, NULL, sFolder, SW_SHOW);
#endif
}
//���ɰ崲�ӹ�NC����
void CPPEDoc::OnCreatePunchNcData()
{
#ifdef __PNC_
	if(!g_sysPara.IsValidNcFlag(CNCPart::PUNCH_MODE))
	{
		AfxMessageBox("δ���ó崲�ӹ����ģʽ������ϵͳ����=>���=>�������������[�ְ�NCģʽ]��");
		return;
	}
	//���ݰ��Ըְ���з���
	CHashStrList<PLATE_GROUP> hashPlateByThickMat;
	InitPlateGroupByThickMat(hashPlateByThickMat);
	if (hashPlateByThickMat.GetNodeNum() <= 0)
		return;
	//ָ�����·��
	CXhChar500 sFolder = model.GetFolderPath();
	CString ss(sFolder);
	if (InvokeFolderPickerDlg(ss))
		sFolder.Copy(ss);
	//���ɸְ�����NC����
	CNCPart::m_bDeformedProfile = TRUE;	//PNC��ȡ�ĸְ�Ĭ���ѿ��ǻ�������
	CXhChar100 sThick = g_sysPara.nc.m_xPunchPara.m_sThick;
	if (g_sysPara.nc.m_xPunchPara.IsValidFile(CNCPart::PLATE_PBJ_FILE))
		CreatePlateNcFiles(hashPlateByThickMat, sThick, sFolder, CNCPart::PUNCH_MODE, CNCPart::PLATE_PBJ_FILE);
	if (g_sysPara.nc.m_xPunchPara.IsValidFile(CNCPart::PLATE_WKF_FILE))
		CreatePlateNcFiles(hashPlateByThickMat, sThick, sFolder, CNCPart::PUNCH_MODE, CNCPart::PLATE_WKF_FILE);
	if (g_sysPara.nc.m_xPunchPara.IsValidFile(CNCPart::PLATE_TTP_FILE))
		CreatePlateNcFiles(hashPlateByThickMat, sThick, sFolder, CNCPart::PUNCH_MODE, CNCPart::PLATE_TTP_FILE);
	if (g_sysPara.nc.m_xPunchPara.IsValidFile(CNCPart::PLATE_DXF_FILE))
		CreatePlateNcFiles(hashPlateByThickMat, sThick, sFolder, CNCPart::PUNCH_MODE, CNCPart::PLATE_DXF_FILE);
	ShellExecute(NULL, "open", NULL, NULL, sFolder, SW_SHOW);
#endif
}
void CPPEDoc::OnCreateDrillNcData()
{
#ifdef __PNC_
	if (!g_sysPara.IsValidNcFlag(CNCPart::DRILL_MODE))
	{
		AfxMessageBox("δ�����괲�ӹ����ģʽ������ϵͳ����=>���=>�������������[�ְ�NCģʽ]��");
		return;
	}
	//���ݰ��Ըְ���з���
	CHashStrList<PLATE_GROUP> hashPlateByThickMat;
	InitPlateGroupByThickMat(hashPlateByThickMat);
	if (hashPlateByThickMat.GetNodeNum() <= 0)
		return;
	//ָ�����·��
	CXhChar500 sFolder = model.GetFolderPath();
	CString ss(sFolder);
	if (InvokeFolderPickerDlg(ss))
		sFolder.Copy(ss);
	//���ɸְ�����NC����
	CNCPart::m_bDeformedProfile = TRUE;	//PNC��ȡ�ĸְ�Ĭ���ѿ��ǻ�������
	CXhChar100 sThick = g_sysPara.nc.m_xDrillPara.m_sThick;
	if (g_sysPara.nc.m_xDrillPara.IsValidFile(CNCPart::PLATE_PMZ_FILE))
		CreatePlateNcFiles(hashPlateByThickMat, sThick, sFolder, CNCPart::DRILL_MODE, CNCPart::PLATE_PMZ_FILE);
	if (g_sysPara.pmz.m_bPmzCheck)
		CreatePlateNcFiles(hashPlateByThickMat, sThick, sFolder, CNCPart::DRILL_MODE, CNCPart::PLATE_PMZ_FILE, TRUE);
	if (g_sysPara.nc.m_xDrillPara.IsValidFile(CNCPart::PLATE_DXF_FILE))
		CreatePlateNcFiles(hashPlateByThickMat, sThick, sFolder, CNCPart::DRILL_MODE, CNCPart::PLATE_DXF_FILE);
	ShellExecute(NULL, "open", NULL, NULL, sFolder, SW_SHOW);
#endif
}
//���ɼ��⹤��NC����
void CPPEDoc::OnCreateLaserNcData()
{
#ifdef __PNC_
	//���ݰ��Ըְ���з���
	CHashStrList<PLATE_GROUP> hashPlateByThickMat;
	InitPlateGroupByThickMat(hashPlateByThickMat);
	if (hashPlateByThickMat.GetNodeNum() <= 0)
		return;
	CXhChar500 sFolder = model.GetFolderPath();
	CString ss(sFolder);
	if (InvokeFolderPickerDlg(ss))
		sFolder.Copy(ss);
	//���ɸְ�����NC����
	CNCPart::m_bDeformedProfile = TRUE;	//PNC��ȡ�ĸְ�Ĭ���ѿ��ǻ�������
	CreatePlateNcFiles(hashPlateByThickMat, NULL, sFolder, CNCPart::LASER_MODE, CNCPart::PLATE_DXF_FILE);
	//
	ShellExecute(NULL, "open", NULL, NULL, sFolder, SW_SHOW);
#endif
}
//һ���������úõ�����NC����
void CPPEDoc::OnCreatePlateNcData()
{
#ifdef __PNC_
	//���ݰ��Ըְ���з���
	CHashStrList<PLATE_GROUP> hashPlateByThickMat;
	InitPlateGroupByThickMat(hashPlateByThickMat);
	if (hashPlateByThickMat.GetNodeNum() <= 0)
		return;
	CXhChar500 sFolder = model.GetFolderPath();
	CString ss(sFolder);
	if (InvokeFolderPickerDlg(ss))
		sFolder.Copy(ss);
	//���ɸְ�����NC����
	CNCPart::m_bDeformedProfile = TRUE;	//PNC��ȡ�ĸְ�Ĭ���ѿ��ǻ�������
	int nValidBranch = 0;
	if (g_sysPara.IsValidNcFlag(CNCPart::FLAME_MODE))
	{	//�����и�����û������ȷ�Χ���˸ְ�
		CXhChar100 sThick = g_sysPara.nc.m_xFlamePara.m_sThick;
		if (g_sysPara.nc.m_xFlamePara.IsValidFile(CNCPart::PLATE_TXT_FILE))
		{
			CreatePlateNcFiles(hashPlateByThickMat, sThick, sFolder, CNCPart::FLAME_MODE, CNCPart::PLATE_TXT_FILE);
			nValidBranch++;
		}
		if (g_sysPara.nc.m_xFlamePara.IsValidFile(CNCPart::PLATE_CNC_FILE))
		{
			CreatePlateNcFiles(hashPlateByThickMat, sThick, sFolder, CNCPart::FLAME_MODE, CNCPart::PLATE_CNC_FILE);
			nValidBranch++;
		}
		if (g_sysPara.nc.m_xFlamePara.IsValidFile(CNCPart::PLATE_DXF_FILE))
		{
			CreatePlateNcFiles(hashPlateByThickMat, sThick, sFolder, CNCPart::FLAME_MODE, CNCPart::PLATE_DXF_FILE);
			nValidBranch++;
		}
	}
	if (g_sysPara.IsValidNcFlag(CNCPart::PLASMA_MODE))
	{	//�������и�����û������ȷ�Χ���˸ְ�
		CXhChar100 sThick = g_sysPara.nc.m_xPlasmaPara.m_sThick;
		if (g_sysPara.nc.m_xPlasmaPara.IsValidFile(CNCPart::PLATE_NC_FILE))
		{
			CreatePlateNcFiles(hashPlateByThickMat, sThick, sFolder, CNCPart::PLASMA_MODE, CNCPart::PLATE_NC_FILE);
			nValidBranch++;
		}
		if (g_sysPara.nc.m_xPlasmaPara.IsValidFile(CNCPart::PLATE_DXF_FILE))
		{
			CreatePlateNcFiles(hashPlateByThickMat, sThick, sFolder, CNCPart::PLASMA_MODE, CNCPart::PLATE_DXF_FILE);
			nValidBranch++;
		}
	}
	if (g_sysPara.IsValidNcFlag(CNCPart::PUNCH_MODE))
	{	//�崲�������û������ȷ�Χ���˸ְ�
		CXhChar100 sThick = g_sysPara.nc.m_xPunchPara.m_sThick;
		if (g_sysPara.nc.m_xPunchPara.IsValidFile(CNCPart::PLATE_PBJ_FILE))
		{
			CreatePlateNcFiles(hashPlateByThickMat, sThick, sFolder, CNCPart::PUNCH_MODE, CNCPart::PLATE_PBJ_FILE);
			nValidBranch++;
		}
		if (g_sysPara.nc.m_xPunchPara.IsValidFile(CNCPart::PLATE_WKF_FILE))
		{
			CreatePlateNcFiles(hashPlateByThickMat, sThick, sFolder, CNCPart::PUNCH_MODE, CNCPart::PLATE_WKF_FILE);
			nValidBranch++;
		}
		if (g_sysPara.nc.m_xPunchPara.IsValidFile(CNCPart::PLATE_TTP_FILE))
		{
			CreatePlateNcFiles(hashPlateByThickMat, sThick, sFolder, CNCPart::PUNCH_MODE, CNCPart::PLATE_TTP_FILE);
			nValidBranch++;
		}
		if (g_sysPara.nc.m_xPunchPara.IsValidFile(CNCPart::PLATE_DXF_FILE))
		{
			CreatePlateNcFiles(hashPlateByThickMat, sThick, sFolder, CNCPart::PUNCH_MODE, CNCPart::PLATE_DXF_FILE);
			nValidBranch++;
		}
	}
	if (g_sysPara.IsValidNcFlag(CNCPart::DRILL_MODE))
	{	//�괲�������û������ȷ�Χ���˸ְ�
		CXhChar100 sThick = g_sysPara.nc.m_xDrillPara.m_sThick;
		if (g_sysPara.nc.m_xDrillPara.IsValidFile(CNCPart::PLATE_PMZ_FILE))
		{
			CreatePlateNcFiles(hashPlateByThickMat, sThick, sFolder, CNCPart::DRILL_MODE, CNCPart::PLATE_PMZ_FILE);
			nValidBranch++;
		}
		if (g_sysPara.pmz.m_bPmzCheck)	//����괲�ӹ�PMZԤ���ʽ�ļ� wht 19-07-02
		{
			CreatePlateNcFiles(hashPlateByThickMat, sThick, sFolder, CNCPart::DRILL_MODE, CNCPart::PLATE_PMZ_FILE, TRUE);
			nValidBranch++;
		}
		if (g_sysPara.nc.m_xDrillPara.IsValidFile(CNCPart::PLATE_DXF_FILE))
		{
			CreatePlateNcFiles(hashPlateByThickMat, sThick, sFolder, CNCPart::DRILL_MODE, CNCPart::PLATE_DXF_FILE);
			nValidBranch++;
		}
	}
	if (g_sysPara.IsValidNcFlag(CNCPart::LASER_MODE))
	{	//���⴦��
		CreatePlateNcFiles(hashPlateByThickMat, NULL, sFolder, CNCPart::LASER_MODE, CNCPart::PLATE_DXF_FILE);
		nValidBranch++;
	}
	if (nValidBranch == 0)
		AfxMessageBox("δ���øְ�NC���ģʽ������ϵͳ����=>���=>�������������[�ְ�NCģʽ]��");
	else
		ShellExecute(NULL, "open", NULL, NULL, sFolder, SW_SHOW);
#endif
}
void CPPEDoc::OnGenAngleNcFile()
{
#ifndef __PNC_
	CLogErrorLife logLife;
	if(g_sysPara.nc.m_sNcDriverPath.GetLength()<=0)
	{
		logerr.Log("û��ѡ��Ǹ�NC�����ļ�");
		return;
	}
	CXhPtrSet<CProcessAngle> angleSet;
	for (CProcessPart *pPart = model.EnumPartFirst(); pPart; pPart = model.EnumPartNext())
	{
		if (pPart->IsAngle())
			angleSet.append((CProcessAngle*)pPart);
	}
	CXhChar500 sWorkDir = GetValidWorkDir(model.GetFolderPath());
	CNCPart::CreateAngleNcFiles(&model, angleSet,g_sysPara.nc.m_sNcDriverPath.GetBuffer(), "", sWorkDir);
#endif
}
void CPPEDoc::OnUpdateGenAngleNcFile(CCmdUI *pCmdUI)
{
#ifndef __PNC_
	pCmdUI->Enable(TRUE);
#else
	pCmdUI->Enable(FALSE);
#endif
}
