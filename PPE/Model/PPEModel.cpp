#include "stdafx.h"
#include "PPEModel.h"
#include "ArrayList.h"
#include "SortFunc.h"
#include "ComparePartNoString.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

CPPEModel model;
static CProcessPart* CreatePart(int idPartClsType,const char* key,void* pContext)
{
	CProcessPart* pPart=NULL;
	if(idPartClsType==CProcessPart::TYPE_LINEANGLE)
		pPart=new CProcessAngle();
	else if(idPartClsType==CProcessPart::TYPE_PLATE)
		pPart=new CProcessPlate();
	else
		pPart=new CProcessPart();	
	return pPart;
}
static BOOL DestroyPart(CProcessPart* pPart)
{
	if(pPart->m_cPartType==CProcessPart::TYPE_LINEANGLE)
		delete (CProcessAngle*)pPart;
	else if(pPart->m_cPartType==CProcessPart::TYPE_PLATE)
		delete (CProcessPlate*)pPart;
	else
		delete (CProcessPart*)pPart;
	return TRUE;
}
//
static CLogFile emptylogfile;
ILog2File* CPPEModel::log2file=NULL;	//错误日志文件
ISysPara* CPPEModel::sysPara=NULL;
ILog2File* CPPEModel::Log2File()
{
	if(log2file)
		return log2file;
	else
		return &emptylogfile;
}

CPPEModel::CPPEModel(void)
{
	m_hashPartByPartNo.CreateNewAtom=CreatePart;
	m_hashPartByPartNo.DeleteAtom=DestroyPart;
	DisplayProcess=NULL;
}

CPPEModel::~CPPEModel(void)
{
	Empty();
}
void CPPEModel::Empty()
{
	m_hashPartByPartNo.Empty();
}
BOOL CPPEModel::FromBuffer(CBuffer &buffer)
{
	Empty();
	buffer.SeekToBegin();
	if(buffer.GetLength()<=0)
		return FALSE;

	CXhChar50 sDocType,sVersion;
	buffer.ReadString(sDocType);
	if(!sDocType.EqualNoCase("工艺资料管理系统"))
	{
		MessageBox(NULL,"数据块格式不对，读取失败！","PPE",MB_OK);
		return false;
	}
	long buffer_len=buffer.GetLength();
	if(DisplayProcess!=NULL)
		DisplayProcess(ftoi(buffer.GetCursorPosition()/(buffer_len*0.01)),NULL);
	buffer.ReadString(sVersion);
	long nVersion=FromStringVersion(sVersion);
	//基本信息
	buffer.ReadString(m_sCompanyName);
	buffer.ReadString(m_sPrjCode);
	buffer.ReadString(m_sPrjName);
	buffer.ReadString(m_sTaType);	
	buffer.ReadString(m_sTaAlias);	
	buffer.ReadString(m_sTaStampNo);
	buffer.ReadString(m_sOperator);
	buffer.ReadString(m_sAuditor);
	buffer.ReadString(m_sCritic);	

	int i=0,n=0;
	CXhChar16 sPartNo;
	//读取构件
	buffer.ReadInteger(&n);
	for(i=0;i<n;i++)
	{
		BYTE cPartType=CProcessPart::RetrievedPartTypeAndLabelFromBuffer(buffer,sPartNo,17);
		CProcessPart *pPart=AddPart(sPartNo,cPartType);
		pPart->FromPPIBuffer(buffer);
		if(DisplayProcess!=NULL)
			DisplayProcess(ftoi(buffer.GetCursorPosition()/(buffer_len*0.01)),NULL);
	}
	InitPlateMcsAndCutPt(false);
	return TRUE;
}
void CPPEModel::ToBuffer(CBuffer &buffer,const char *file_version)//,bool bIncPartPattern/*=false*/)
{
	buffer.WriteString("工艺资料管理系统");
	buffer.WriteString(file_version);	
	//基本信息
	buffer.WriteString(m_sCompanyName);	//设计单位
	buffer.WriteString(m_sPrjCode);		//工程编号
	buffer.WriteString(m_sPrjName);		//工程名称
	buffer.WriteString(m_sTaType);		//塔型
	buffer.WriteString(m_sTaAlias);		//代号
	buffer.WriteString(m_sTaStampNo);	//钢印号
	buffer.WriteString(m_sOperator);	//操作员（制表人）
	buffer.WriteString(m_sAuditor);		//审核人
	buffer.WriteString(m_sCritic);		//评审人
	//构件列表
	BUFFERPOP stack(&buffer,m_hashPartByPartNo.GetNodeNum());
	buffer.WriteInteger(m_hashPartByPartNo.GetNodeNum());
	for(CProcessPart *pPart=m_hashPartByPartNo.GetFirst();pPart;pPart=m_hashPartByPartNo.GetNext())
	{
		pPart->ToPPIBuffer(buffer);
		stack.Increment();
	}
	if(!stack.VerifyAndRestore())
		Log2File()->Log("构件数量出现错误！");
}
BOOL LoadBufferFromFile(char* sFileName,CBuffer &buffer)
{
	FILE *fp=fopen(sFileName,"rb");
	if(fp==NULL)
		return FALSE;
	try{
		long file_len;
		fread(&file_len,sizeof(long),1,fp);
		buffer.ClearContents();
		buffer.Write(NULL,file_len);
		fread(buffer.GetBufferPtr(),file_len,1,fp);
		fclose(fp);
	}catch (CFileException*){
		return FALSE;
	}
	catch (CException*){
		return FALSE;
	}
	return TRUE;
}
BOOL CPPEModel::InitModelByFolderPath(const char *folder_path)
{
	m_sFolderPath.Copy(folder_path);
	m_hashPartByPartNo.Empty();
	//使用SetCurrentDirectory设置工作路径后，使用相对路径写文件
	//可以避免文件夹中带"."无法保存或者路径名太长无法保存的问题 wht 19-04-24
	SetCurrentDirectory(m_sFolderPath);
	CXhChar200 sFilename;
	//sFilename.Printf("%s\\*.ppi",(char*)m_sFolderPath);
	sFilename.Printf(".\\*.ppi");
	WIN32_FIND_DATA FindFileData;
	HANDLE hFindFile = FindFirstFile(sFilename, &FindFileData);
	if(hFindFile==INVALID_HANDLE_VALUE)
		return FALSE;
	CBuffer buffer;
	do{
		//sFilename.Printf("%s\\%s",(char*)m_sFolderPath, FindFileData.cFileName);
		sFilename.Printf(".\\%s", FindFileData.cFileName);
		if(!LoadBufferFromFile(sFilename,buffer))
			continue;
		buffer.SeekToBegin();
		CXhChar16 sPartNo;
		BYTE cPartType=CProcessPart::RetrievedPartTypeAndLabelFromBuffer(buffer,sPartNo);
		buffer.SeekToBegin();
		CProcessPart *pPart=AddPart(sPartNo,cPartType,sFilename);
		pPart->FromPPIBuffer(buffer);
	}while(FindNextFile(hFindFile,&FindFileData));
	FindClose(hFindFile);
	InitPlateMcsAndCutPt(true);
	return TRUE;
}
CXhChar500 CPPEModel::GetPartFilePath(const char *sPartNo)
{
	CXhChar500 *pFilePath=m_hashFilePathByPartNo.GetValue(sPartNo);
	if(pFilePath)
		return *pFilePath;
	else
		return CXhChar500();
}
CProcessPart* CPPEModel::AddPart(const char *sPartNo,BYTE cType,const char *sFilePath/*=NULL*/)
{
	if(sFilePath)
		m_hashFilePathByPartNo.SetValue(sPartNo,sFilePath);
	CProcessPart *pPart=m_hashPartByPartNo.Add(sPartNo,cType);
	pPart->SetKey(m_hashPartByPartNo.GetNodeNum());
	return pPart;
}
BOOL CPPEModel::DeletePart(const char *sPartNo)
{
	if(sPartNo==NULL)
		return FALSE;
	m_hashFilePathByPartNo.DeleteNode(sPartNo);
	return m_hashPartByPartNo.DeleteNode(sPartNo);
}

typedef CProcessPart* CProcessPartPtr;
extern char* SearchChar(char* srcStr,char ch,bool reverseOrder/*=false*/);
static int CompareProcessPart(const CProcessPartPtr &part1,const CProcessPartPtr &part2)
{
	CXhChar16 sPartNo1=part1->GetPartNo();
	CXhChar16 sPartNo2=part2->GetPartNo();
	char* szExt1=SearchChar(sPartNo1,'#',true);
	if(szExt1)	//件号中含有句柄
		*szExt1=0;
	char* szExt2=SearchChar(sPartNo2,'#',true);
	if(szExt2)	//件号中含有句柄
		*szExt2=0;
	return ComparePartNoString(sPartNo1,sPartNo2);
}
static int CompareFileName(const CProcessPartPtr &part1,const CProcessPartPtr &part2)
{
	CString str1(part1->GetPartNo()),str2(part2->GetPartNo());
	return str1.Compare(str2);
}
void CPPEModel::GetSortedAngleSetAndPlateSet(CXhPtrSet<CProcessAngle> &angleSet,CXhPtrSet<CProcessPlate> &plateSet)
{
	angleSet.Empty();
	plateSet.Empty();
	ARRAY_LIST<CProcessPart*> anglePtrArr;
	ARRAY_LIST<CProcessPart*> platePtrArr;
	for(CProcessPart *pPart=m_hashPartByPartNo.GetFirst();pPart;pPart=m_hashPartByPartNo.GetNext())
	{
		if(pPart->IsPlate())
			platePtrArr.append(pPart);
		else if(pPart->IsAngle())
			anglePtrArr.append(pPart);
	}
	if(anglePtrArr.GetSize()>0)
	{
		CQuickSort<CProcessPart*>::QuickSort(anglePtrArr.m_pData,anglePtrArr.GetSize(),CompareProcessPart);
		for(CProcessPart **ppPart=anglePtrArr.GetFirst();ppPart;ppPart=anglePtrArr.GetNext())
			angleSet.append((CProcessAngle*)*ppPart);
	}
	if(platePtrArr.GetSize()>0)
	{
#if defined(__PNC_)
		CQuickSort<CProcessPart*>::QuickSort(platePtrArr.m_pData,platePtrArr.GetSize(),CompareFileName);
#else
		CQuickSort<CProcessPart*>::QuickSort(platePtrArr.m_pData,platePtrArr.GetSize(),CompareProcessPart);
#endif
		
		for(CProcessPart **ppPart=platePtrArr.GetFirst();ppPart;ppPart=platePtrArr.GetNext())
			plateSet.append((CProcessPlate*)*ppPart);
	}
}
void CPPEModel::ModifyPartNo(const char* sOldPartNo,const char* sNewPartNo)
{
	CProcessPart *pPart=model.FromPartNo(sOldPartNo);
	if(pPart==NULL)
		return;
	pPart->SetPartNo(sNewPartNo);
	//修改对应哈希表的键值
	m_hashPartByPartNo.ModifyKeyStr(sOldPartNo,sNewPartNo);
	m_hashFilePathByPartNo.ModifyKeyStr(sOldPartNo,sNewPartNo);
}
BOOL CPPEModel::IsAllDeformedProfile()
{
	for(CProcessPart *pPart=EnumPartFirst();pPart;pPart=EnumPartNext())
	{
		if(!pPart->IsPlate())
			continue;
		CProcessPlate* pPlate=(CProcessPlate*)pPart;
		if(!pPlate->m_bIncDeformed)
			return FALSE;
	}
	return TRUE;
}
void CPPEModel::InitPlateMcsAndCutPt(bool bSaveToFile/*=false*/)
{	//初始化钢板加工坐标系及切入点
	bool bInitFarOrg=false;
	if(sysPara)
		bInitFarOrg=(sysPara->GetCutInitPosFarOrg()==TRUE);
	for(CProcessPart *pPart=EnumPartFirst();pPart;pPart=EnumPartNext())
	{
		if(!pPart->IsPlate())
			continue;
		CProcessPlate* pPlate=(CProcessPlate*)pPart;
		pPlate->InitMCS();
		pPlate->InitCutPt(bInitFarOrg);
		if(bSaveToFile)
			SavePartToFile(pPlate);
	}
}
bool CPPEModel::SavePartToFile(CProcessPart *pPart)
{
	if(pPart==NULL)
		return false;
	CXhChar500 sFilePath=model.GetPartFilePath(pPart->GetPartNo());
	if(sFilePath.Length()<=0)
		return false;
	if (sFilePath.StartWith('.') && m_sFolderPath.GetLength()>0)
	{	//使用SetCurrentDirectory设置工作路径后，使用相对路径写文件
		//可以避免文件夹中带"."无法保存或者路径名太长无法保存的问题 wht 19-04-24
		//查询构件路径为相对路径时需要设置当前工作路径，否则ppi文件保存路径错误导致修改无效 wht 19-10-05
		SetCurrentDirectory(m_sFolderPath);
	}
	FILE *fp=fopen(sFilePath,"wb");
	if(fp==NULL)
		return false;
	CBuffer buffer;
	pPart->ToPPIBuffer(buffer);
	long file_len=buffer.GetLength();
	fwrite(&file_len,sizeof(long),1,fp);
	fwrite(buffer.GetBufferPtr(),buffer.GetLength(),1,fp);
	fclose(fp);
	return true;
}
void CPPEModel::ReadPrjTowerInfoFromCfgFile(const char* cfg_file_path)
{
	if(cfg_file_path==NULL||strlen(cfg_file_path)<=0)
		return;
	FILE *fp=fopen(cfg_file_path,"rt");
	if(fp==NULL)
		return;
	CXhChar500 sLine;
	char line_txt[MAX_PATH],key_word[100];
	while(!feof(fp))
	{
		if(fgets(line_txt,MAX_PATH,fp)==NULL)
			break;
		char sText[MAX_PATH];
		strcpy(sText,line_txt);
		sLine=sText;
		sLine.Replace('=',' ');
		sprintf(line_txt,"%s",sLine);
		char *skey=strtok((char*)sText,"=,;");
		strncpy(key_word,skey,100);
		//常规设置
		if(_stricmp(key_word,"PROJECT_NAME")==0)
			sscanf(line_txt,"%s %s",&key_word,&m_sPrjName);
		else if(_stricmp(key_word,"PROJECT_CODE")==0)
			sscanf(line_txt,"%s %s",&key_word,&m_sPrjCode);
		else if(_stricmp(key_word,"TOWER_NAME")==0)
			sscanf(line_txt,"%s %s",&key_word,&m_sTaType);
		else if(_stricmp(key_word,"TOWER_CODE")==0)
			sscanf(line_txt,"%s %s",&key_word,&m_sTaAlias);
		else if(_stricmp(key_word,"STAMP_NO")==0)
			sscanf(line_txt,"%s %s",&key_word,&m_sTaStampNo);
	}
	fclose(fp);
}