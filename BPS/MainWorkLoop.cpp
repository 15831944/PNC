// WorkLoop.cpp: implementation of the CMainWorkLoop class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XhLicServ.h"
#include "MainWorkLoop.h"
#include "ServiceModule.h"
#include "direct.h"
#include "LicSvc.h"
#include "Buffer.h"
#include "XhCharString.h"
#include "ArrayList.h"
#include "aes.h"
#include "BigInt.h"
#include "..\..\XhLmd\MD5.h"
#include "Communication.h"
/*#include "FileIO.h"
#include "PermModifyList.h"*/

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
long CMainWorkLoop::m_nMaxHandle=0x1F;
SECURITY_ATTRIBUTES CMainWorkLoop::sa;
CHashPtrList<CLicService> CMainWorkLoop::hashLicSrvs;
CHashListEx<ENDUSER> CMainWorkLoop::hashEndUsers;
CSuperHashList<IProductAcl> CMainWorkLoop::hashProductSrvs;
UINT CMainWorkLoop::cursor_pipeline_no=0;
CMainWorkLoop _WorkLoop;
char *PIPE_PATH="\\\\.\\Pipe\\XhLicServ";
BOOL CMainWorkLoop::m_bQuitNow=FALSE;
//
static DWORD BINARYWORD[32]={ 0X00000001,0X00000002,0X00000004,0X00000008,0X00000010,0X00000020,0X00000040,0X00000080,
	0X00000100,0X00000200,0X00000400,0X00000800,0X00001000,0X00002000,0X00004000,0X00008000,
	0X00010000,0X00020000,0X00040000,0X00080000,0X00100000,0X00200000,0X00400000,0X00800000,
	0X01000000,0X02000000,0X04000000,0X08000000,0X10000000,0X20000000,0X40000000,0X80000000};
DWORD GetBinaryWord(int index)
{
	if(index<0||index>=32)
		return 0;
	else
		return BINARYWORD[index];
}
//������ѭ��
void GetSysPath(char* App_Path,char *src_path=NULL)
{
	char drive[4];
	char dir[MAX_PATH];
	char fname[MAX_PATH];
	char ext[MAX_PATH];

	if(src_path)
		_splitpath(src_path,drive,dir,fname,ext);
	else
		_splitpath(__argv[0],drive,dir,fname,ext);
	strcpy(App_Path,drive);
	strcat(App_Path,dir);
	_chdir(App_Path);
}
AUTHENTICATION* IProductAcl::FromIdentity(const BYTE ident_bytes[16],ENDUSER* pMatchedEndUser/*=NULL*/)
{
	IDENTITY identity;
	memcpy(identity.bytes,ident_bytes,16);
	AUTHENTICATION* users=ActiveUsers();
	for(BYTE i=0;i<MaxUsers();i++)
	{
		ENDUSER* pEndUser=CMainWorkLoop::hashEndUsers.GetValue(users[i].dwEndUserSerial);
		if(pEndUser==NULL)
			continue;
		if(pEndUser->identity.i64[0]==identity.i64[0]&&pEndUser->identity.i64[1]==identity.i64[1])
		{
			if(pMatchedEndUser)
				*pMatchedEndUser=*pEndUser;
			return &users[i];
		}
	}
	return NULL;
}
BOOL IProductAcl::RemoveIdentity(const BYTE ident_bytes[16])
{
	IDENTITY identity;
	memcpy(identity.bytes,ident_bytes,16);
	AUTHENTICATION* users=ActiveUsers();
	CUserDatabase userdb;
	for(BYTE i=0;i<MaxUsers();i++)
	{
		ENDUSER* pEndUser=CMainWorkLoop::hashEndUsers.GetValue(users[i].dwEndUserSerial);
		if(pEndUser==NULL)
			continue;
		if(pEndUser->identity.i64[0]==identity.i64[0]&&pEndUser->identity.i64[1]==identity.i64[1])
		{
			users[i].dwEndUserSerial=0;
			DWORD dwFlag=GetBinaryWord(cProductType-1);
			if(pEndUser->dwAcceptProductFlag==dwFlag)
			{
				CMainWorkLoop::hashEndUsers.DeleteNode(pEndUser->serial);
				//�������ļ�ɾ���û�
				userdb.DelUser(pEndUser->identity.bytes);
			}
			else
				pEndUser->dwAcceptProductFlag^=dwFlag;
			//�������ļ�ɾ��
			userdb.SetProductACL(this);
			return TRUE;
		}
	}
	return FALSE;
}
AUTHENTICATION* IProductAcl::AddAuthentication(AUTHENTICATION& authenticaion)
{
	AUTHENTICATION *pUser=NULL,*users=ActiveUsers();
	short iFirstIdlePlace=-1;
	for(BYTE i=0;i<MaxUsers();i++)
	{
		if(users[i].dwEndUserSerial==authenticaion.dwEndUserSerial)
			return &users[i];
		if(iFirstIdlePlace<0&&users[i].dwEndUserSerial==0)
			iFirstIdlePlace=i;
	}
	if(iFirstIdlePlace>=0)
		pUser=&users[iFirstIdlePlace];
	if(pUser)
		*pUser=authenticaion;
	return pUser;
}
IProductAcl* CreateACLInstance(int nMaxUsers,DWORD key,void* pContext);
IProductAcl* IProductAcl::CreateACLInstance(int nMaxUsers,DWORD key,void* pContext)
{
	if(nMaxUsers<=0)
		return NULL;
	else if(nMaxUsers<=2)
		return new PRODUCT_ACL2((BYTE)key);
	else if(nMaxUsers<=3)
		return new PRODUCT_ACL3((BYTE)key);
	else if(nMaxUsers<=5)
		return new PRODUCT_ACL5((BYTE)key);
	else if(nMaxUsers<=10)
		return new PRODUCT_ACL10((BYTE)key);
	else if(nMaxUsers<=20)
		return new PRODUCT_ACL20((BYTE)key);
	else if(nMaxUsers<=30)
		return new PRODUCT_ACL30((BYTE)key);
	else if(nMaxUsers<=50)
		return new PRODUCT_ACL50((BYTE)key);
	else if(nMaxUsers<=80)
		return new PRODUCT_ACL80((BYTE)key);
	else //if(nMaxUsers<=100)
		return new PRODUCT_ACL100((BYTE)key);
}
BOOL IProductAcl::DeleteACLInstance(IProductAcl *pAcl)				//����ص�����
{
	if(pAcl==NULL)
		return FALSE;
	int nMaxUsers=pAcl->MaxUsers();
	if(nMaxUsers<=2)
		delete (PRODUCT_ACL2*)pAcl;
	else if(nMaxUsers<=3)
		delete (PRODUCT_ACL3*)pAcl;
	else if(nMaxUsers<=5)
		delete (PRODUCT_ACL5*)pAcl;
	else if(nMaxUsers<=10)
		delete (PRODUCT_ACL10*)pAcl;
	else if(nMaxUsers<=20)
		delete (PRODUCT_ACL20*)pAcl;
	else if(nMaxUsers<=30)
		delete (PRODUCT_ACL30*)pAcl;
	else if(nMaxUsers<=50)
		delete (PRODUCT_ACL50*)pAcl;
	else if(nMaxUsers<=80)
		delete (PRODUCT_ACL80*)pAcl;
	else if(nMaxUsers<=100)
		delete (PRODUCT_ACL100*)pAcl;
	else
		return false;
	return true;
}

char *PIPE_CORE_NAME="\\\\.\\Pipe\\XhLicServ\\CorePipe";
UINT CMainWorkLoop::StartPipeTaskListen(LPVOID lpPara)
{
	return StartTaskListen(lpPara,TRUE);
}
UINT CMainWorkLoop::StartSocketTaskListen(LPVOID lpPara)
{
	return StartTaskListen(lpPara,FALSE);
}
UINT CMainWorkLoop::StartTaskListen(LPVOID lpPara,BOOL bPipeConnect)
{
	logevent.SetWarningLevel(CLogFile::WARNING_LEVEL_ALL);	//��ͣ�����ʾ��Ϣ���Դ��Ժ����ƴ�����־���ϵͳ
	CCommunicationObject commCore;
	if(bPipeConnect)
		commCore.InitPipeConnect(PIPE_CORE_NAME,true,&CMainWorkLoop::sa);
	else 
	{
		HKEY hKey=NULL;
		DWORD dwDataType,dwReadBytes;
		CXhChar50 szSubKey("Software\\Xerofox\\XhLicServ\\SrvLicenses");
		if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,szSubKey,0,KEY_READ|KEY_WRITE,&hKey)!= ERROR_SUCCESS||hKey==NULL)
		{
			logevent.Log("δ�鵽�ź������Ʒ��Ȩ��������ע����Ϣ");
			return FALSE;
		}
		DWORD dwPortNumber=0;
		if(RegQueryValueEx(hKey,"PortNumber",NULL,&dwDataType,(BYTE*)&dwPortNumber,&dwReadBytes)!= ERROR_SUCCESS)
		{
			logevent.Log("δ�鵽�˿ں�");
			return FALSE;
		}
		CXhChar50 sIP=CSocketObject::GetMachineIpStr(),sRegIP;
		dwReadBytes=1024;	//��һ���㹻�����������ܶ�ȡʧ�ܣ����ش������234��û���㹻�ĳ��� wht 17-02-22
		if(RegQueryValueEx(hKey,"IP",NULL,&dwDataType,(BYTE*)(char*)sRegIP,&dwReadBytes)!= ERROR_SUCCESS)
		{
			logevent.Log("δ�鵽IP��ַ");
			return FALSE;
		}
		else
			sIP.Copy(sRegIP);
		RegCloseKey(hKey);
		commCore.InitSocketConnect(sIP,(USHORT)dwPortNumber,true);
	}
	if(!commCore.IsValidConnection())
		logevent.Log(commCore.GetErrorStr());
	BYTE buffer[2048]={'\0'};
	LICSERV_MSGBUF msg;
	msg.msg_length=0;
	msg.lpBuffer=buffer;
	DWORD nBytesRead;
	bool first=true;
	while(1)
	{
		if(CMainWorkLoop::m_bQuitNow)
			break;
		commCore.Connect();
		CXhChar50 sError=commCore.GetErrorStr();
		if(sError.Length()>0)
		{
			logevent.Log(sError);
			break;
		}
		//int i=GetLastError();
		commCore.Read((void*)&msg,9,&nBytesRead);
		//if(msg.msg_length>0)
		//{
		//	ReadFile(hCorePipe,buffer,msg.msg_length,&nBytesRead,NULL);
		//	buffer[nBytesRead]='\0';
		//}
		if(msg.command_id==LICSERV_MSGBUF::APPLY_FOR_LICENSE)	//�����ն�ʹ����Ȩ
			CMainWorkLoop::ApplyForLicense(commCore,msg);
		else if(msg.command_id==LICSERV_MSGBUF::RETURN_LICENSE)
			CMainWorkLoop::ReturnLicense(commCore,msg);
		else if(msg.command_id==LICSERV_MSGBUF::LICENSES_MODIFIED)
			CMainWorkLoop::LicenseValidate(commCore,msg);
		commCore.Disconnect();
	}
	return 0;
}
CMainWorkLoop::CMainWorkLoop()
{
	PSECURITY_DESCRIPTOR    pSD;
									   // create a security NULL security
									   // descriptor, one that allows anyone
									   // to write to the pipe... WARNING
									   // entering NULL as the last attribute
									   // of the CreateNamedPipe() will
									   // indicate that you wish all
									   // clients connecting to it to have
									   // all of the same security attributes
									   // as the user that started the
									   // pipe server.

	pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR,
			   SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
	{
		LocalFree((HLOCAL)pSD);
		logevent.Log("InitializeSecurityDescriptor failed!");
		return;
	}
							   // add a NULL disc. ACL to the
							   // security descriptor.
	if (!SetSecurityDescriptorDacl(pSD, TRUE, (PACL) NULL, FALSE))
	 {
		LocalFree((HLOCAL)pSD);
		logevent.Log("SetSecurityDescriptorDacl failed!");
		return;
	 }

	CMainWorkLoop::sa.nLength = sizeof(sa);
	CMainWorkLoop::sa.lpSecurityDescriptor = pSD;
	CMainWorkLoop::sa.bInheritHandle = TRUE;
	hashProductSrvs.CreateNewAtom=IProductAcl::CreateACLInstance;
	hashProductSrvs.DeleteAtom=IProductAcl::DeleteACLInstance;
}

CMainWorkLoop::~CMainWorkLoop()
{
	//for(ENDUSER *pUser=hashEndUsers.EnumObjectFirst();pUser;pUser=hashEndUsers.EnumObjectNext())
	//	CloseHandle(pUser->m_hClientListenPipe);
	hashEndUsers.Empty();
	hashProductSrvs.Empty();
	m_bQuitNow=TRUE;
	if(CMainWorkLoop::sa.lpSecurityDescriptor)
		LocalFree((HLOCAL)CMainWorkLoop::sa.lpSecurityDescriptor);
	CMainWorkLoop::sa.lpSecurityDescriptor=NULL;
}
bool CMainWorkLoop::InitDatabaseFromFile()
{
	ENDUSER enduser;
	hashEndUsers.Empty();
	BOOL itered;
	CUserDatabase userdb;
	for(itered=userdb.QueryFirstUser(&enduser);itered;itered=userdb.QueryNextUser(&enduser))
		hashEndUsers.Append(enduser);

	IProductAcl acl;
	for(itered=userdb.QueryFirstProductACL(&acl);itered;itered=userdb.QueryNextProductACL(&acl))
	{
		IProductAcl *pACL=hashProductSrvs.Add(acl.cProductType,acl.ciMaxUsers);
		pACL->cProductType=acl.cProductType;
		pACL->ciMaxUsers=acl.ciMaxUsers;
		userdb.QueryProductACL(pACL);
	}
	return true;
}

bool CMainWorkLoop::LoadProductSvcFile(const char* svcfilename,BYTE* pciProductType/*=NULL*/)
{
	FILE* fp=fopen(svcfilename,"rb");
	if(fp==NULL)
	{
		logevent.Log("svcfile open failed");
		return false;
	}
	//��ȡsvc�ļ�
	UINT version,uDocType=2834534456;	//2834534456Ϊһ�������ʾ�ź���˾ V2C�ļ�
	//XHVERSION version;
	fread(&uDocType,4,1,fp);
	fread(&version,4,1,fp);
	char header_bytes[75],plain_bytes[75]={0};
	fread(header_bytes,75,1,fp);
	//Vendor 2 Customer RSA��Կ��
	char* V2C_RSA_n   = "922D3A4BFCC5716EAFBF72617FE99C240754E6CB3732D1E55B0E37E2807A1AFC37D08DCA59BBF7519F10435AEDD67F65586DA1D8033F98ED24C43F7B8077B9C4EBB3D84E3D59B3BFD168E7";
	//RSA�Ĺ�Կd ed��1(mod ��(n)),ŷ��������(n)=(p-1)*(q-1)
	char* V2C_RSA_d   = "B24E38EFEADCD2C091007E8C8261F3D280A98C880FF55AD659782D173BD9CF1BA5C149A7FD6E0BF47CE2A97CF0E4F9C0782E16D53CBD5FD282D568145A1B086A73386A99435DCDD913B9E5";
	CLicService::DecryptByRSA(header_bytes,plain_bytes,75,V2C_RSA_d,V2C_RSA_n);
	SVCHEADER header;
	memcpy(&header,plain_bytes,sizeof(SVCHEADER));
	CLicService* pLicServ=hashLicSrvs.Add(header.ciProductType);
	pLicServ->svcheader=header;
	pLicServ->coredata1.Resize(688);	//640+48=688 Bytes
	fread(pLicServ->coredata1,688,1,fp);
	BYTE aeskey[16],pool[16],output[16];
	DWORD i,indexarr[16];
	MakeLocalRandOrder(header.identity,16,indexarr,16);
	for(i=0;i<16;i++)
		aeskey[i]=header.aeskey[indexarr[i]];
	aes_context context;
	aes_set_key(&context,aeskey,128);
	for(i=0;i<688;i+=16)
	{
		memcpy(pool,pLicServ->coredata1+i,16);
		aes_decrypt(&context,pool,(BYTE*)output);
		memcpy(pLicServ->coredata1+i,output,16);
	}
	//1.���ķ�ʽ�洢V2C�ļ�ͷ��Ϣ
	//��֤���ܹ����ͼ���Ȩ��Ʒ
	//��֤���ܹ�����װ������
	//��֤��������ָ��
	//��ʼ����ǰ��Ʒ����Ȩ����
	char cProductType=header.ciProductType;
	if(pciProductType)
		*pciProductType=header.ciProductType;
	IProductAcl* pNowAcl=hashProductSrvs.GetValue(cProductType);
	if(pNowAcl==NULL)
	{
		pNowAcl=hashProductSrvs.Add(cProductType,header.ciMaxUsers);
		pNowAcl->cProductType=cProductType;
	}
	else if(pNowAcl->MaxUsers()!=header.ciMaxUsers)
	{
		int minUsers=min(header.ciMaxUsers,pNowAcl->MaxUsers());
		DYN_ARRAY<AUTHENTICATION> endusers(minUsers);
		memcpy(endusers,pNowAcl->ActiveUsers(),sizeof(AUTHENTICATION)*minUsers);
		hashProductSrvs.DeleteNode(cProductType);
		pNowAcl=hashProductSrvs.Add(cProductType,header.ciMaxUsers);
		memcpy(pNowAcl->ActiveUsers(),endusers,sizeof(AUTHENTICATION)*minUsers);
	}
	//LoadProductSvcFile�ɹ����ѿ�ȷ��products[cApplyProductType-1]!=NULL
	fclose(fp);
	return true;
}
static char* SearchChar(char* srcStr,char ch,bool reverseOrder/*=false*/)
{
	if(!reverseOrder)
		return strchr(srcStr,ch);
	else
	{
		int len=strlen(srcStr);
		for(int i=len-1;i>=0;i--)
		{
			if(srcStr[i]==ch)
				return &srcStr[i];
		}
	}
	return NULL;
}
DWORD CMainWorkLoop::GenerateClientLicenseFile(BYTE cProductType,BYTE enduser_identity[16],BYTE* details,
			char* exportLicBytes/*=NULL*/,WORD wRenewDays/*=15*/)
{
	if(cProductType>PRODUCT_IBOM)
	{
		logevent.Log("�����Ʒ����#%d��֧��",cProductType);
		return FALSE;
	}
	CLicService* pLicSrv=hashLicSrvs.Add(cProductType);
	if(!pLicSrv->IsInited())
	{
		CXhChar200 licfile;
		HKEY hKey=NULL;
		DWORD dwDataType,dwReadBytes;
		CXhChar50 szSubKey("Software\\Xerofox\\XhLicServ\\SrvLicenses");
		if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,szSubKey,0,KEY_READ|KEY_WRITE,&hKey)!= ERROR_SUCCESS||hKey==NULL)
		{
			logevent.Log("δ�鵽�ź������Ʒ��Ȩ��������ע����Ϣ");
			return FALSE;
		}
		if(RegQueryValueEx(hKey,PRODUCT_NAME[cProductType-1],NULL,&dwDataType,(BYTE*)(char*)licfile,&dwReadBytes)!= ERROR_SUCCESS)
		{
			logevent.Log("δ�鵽�������ļ���ע��·��");
			return FALSE;
		}
		RegCloseKey(hKey);
		//CXhChar200 licfile=theApp.GetProfileStringA("SrvLicenses",PRODUCT_NAME[cProductType-1],"");
		char* comma=SearchChar(licfile,'.',true);
		if(comma)
			strcpy(comma,".lic");
		else
		{
			logevent.Log("��֤���ļ�����'%s'����",(char*)licfile);
			return false;
		}
		if(!pLicSrv->InitFromPrimaryLicenseFile(licfile))
		{
			logevent.Log("��֤�鵼��ʧ��",cProductType);
			return false;
		}
	}
	return pLicSrv->RenewClientLicense(enduser_identity,details,exportLicBytes,wRenewDays);
}
bool CMainWorkLoop::LoadProductSvcFile(BYTE cProductType)
{
	if(cProductType>PRODUCT_IBOM)
	{
		logevent.Log("product=%d",cProductType);
		return false;
	}
	//name.Append(".svc");
	HKEY hKey=NULL;
	DWORD dwDataType,dwReadBytes;
	CXhChar50 szSubKey("Software\\Xerofox\\XhLicServ\\SrvLicenses");
	CXhChar200 svcfile;
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,szSubKey,0,KEY_READ|KEY_WRITE,&hKey)!= ERROR_SUCCESS||hKey==NULL)
		return FALSE;
	if(RegQueryValueEx(hKey,PRODUCT_NAME[cProductType-1],NULL,&dwDataType,(BYTE*)(char*)svcfile,&dwReadBytes)!= ERROR_SUCCESS)
		return FALSE;
	RegCloseKey(hKey);
	//CString svcfile=theApp.GetProfileStringA("SrvLicenses",PRODUCT_NAME[cProductType-1],"");
	//if(svcfile.IsEmpty())
	//	return false;
	return LoadProductSvcFile(svcfile);
}
UINT CMainWorkLoop::IssueEndUserLicFile(BYTE cProductType,BYTE identity[16],BYTE* license)
{
	return 0;
}
BOOL CMainWorkLoop::ApplyForLicense(CCommunicationObject &commCore,LICSERV_MSGBUF &msg)
{
	DWORD nBytesRead,nBytesWritten;
	DWORD alignClockTime;	//�ն˼������У׼ʱ��
	IDENTITY identity;
	ENDUSER enduser,*pEnduser;
	CHAR_ARRAY msg_contents(msg.msg_length-32);
	CBuffer buffer(msg_contents,msg.msg_length-32);
	commCore.Read(msg_contents,msg.msg_length-32,&nBytesRead);
	char status=0,cApplyProductType=0;
	CXhChar50 hostname,hostipstr;
	CXhChar100 szBiosSerialNumber;
	buffer.ReadString(enduser.ipstr,51);
	buffer.ReadString(enduser.hostname,51);
	WORD wDetailsLen=0;
	BYTE details[512]={0};
	buffer.ReadWord(&wDetailsLen);
	buffer.Read(details,wDetailsLen);
	buffer.ReadByte(&cApplyProductType);
	buffer.ReadDword(&alignClockTime);
	CCommunicationObject *pCommListen=&commCore;
	CCommunicationObject commListen;
	if(commCore.IsPipeConnect())
	{	//��ǰ��ʱ����һ�����ܵ�����ͻ���д��䷢������Ȩ����
		CXhChar200 ListenPipeName(CXhChar50("%s\\%s_ListenPipe",PIPE_PATH,(char*)enduser.hostname));
		commListen.InitPipeConnect(ListenPipeName,TRUE,&sa);
		pCommListen=&commListen;
		if(!pCommListen->IsValidConnection())
			logevent.Log("%s�ܵ�����ʧ��,%s",(char*)ListenPipeName,(char*)pCommListen->GetErrorStr());
	}
	commCore.Read(identity.bytes,32,&nBytesRead);
	CTime now=time(NULL);
	if(abs((__int64)now.GetTime()-(__int64)alignClockTime)>3600)
	{
		status=-1;	//�ն�����������Ȩ������ʱ�Ӵ��ڹ���ƫ�δͨ��ʱ��У׼
		CTime now_c=alignClockTime;
		logevent.Log("status=%d,Server:%4d-%2d-%2d %2d:%2d:%2d Client:%4d-%2d-%2d %2d:%2d:%2d ",status,
			now.GetYear(),now.GetMonth(),now.GetDay(),now.GetHour(),now.GetMinute(),now.GetSecond(),
			now_c.GetYear(),now_c.GetMonth(),now_c.GetDay(),now_c.GetHour(),now_c.GetMinute(),now_c.GetSecond());
		return pCommListen->Write(&status,1,&nBytesWritten);
	}
	//������ȡ�ն˱�ʶָ����Ϣ����������������Ȩ
	if(!LoadProductSvcFile(cApplyProductType))
	{
		status=-2;	//��������ȱ�ٶ���Ӧ��Ʒ������Ȩ�����ļ�
		logevent.Log("status=%d",status);
		return pCommListen->Write(&status,1,&nBytesWritten);
	}
	//LoadProductSvcFile�ɹ����ѿ�ȷ��products[cApplyProductType-1]!=NULL
	DecryptFingerprint(identity);
	DecryptFingerprintDetails(details,wDetailsLen);
	
	memcpy(enduser.identity.bytes,identity.bytes,16);
	for(pEnduser=hashEndUsers.GetFirst();pEnduser;pEnduser=hashEndUsers.GetNext())
	{
		if(pEnduser->identity.i64[0]==enduser.identity.i64[0]&&pEnduser->identity.i64[1]==enduser.identity.i64[1])
			break;
	}
	CUserDatabase userDb;
	if(pEnduser==NULL)
		pEnduser=hashEndUsers.Append(enduser,0);
	pEnduser->dwAcceptProductFlag|=GetBinaryWord(cApplyProductType-1);
	userDb.SetUser(*pEnduser);

	AUTHENTICATION *pAuthent=NULL;
	IProductAcl* pAcl=hashProductSrvs.GetValue(cApplyProductType);
	pAuthent=pAcl->FromIdentity(identity.bytes);
	if(pAuthent==NULL)
	{
		AUTHENTICATION auth;
		auth.dwEndUserSerial=pEnduser->serial;
		pAuthent=pAcl->AddAuthentication(auth);
	}
	if(pAuthent==NULL)
	{
		status=-3;	//������������Ӧ��Ʒ������Ȩ��������Ȩ��������
		logevent.Log("status=%d",status);
		return pCommListen->Write(&status,1,&nBytesWritten);
	}
	pAuthent->dwTimeStart=(DWORD)time(NULL);
	pAuthent->dwLastRunTime=pAuthent->dwTimeStart;
	pAuthent->dwTimeEnd=pAuthent->dwTimeStart+1296000;//15*24*3600;	//��Ч��15��
	CTime day(pAuthent->dwTimeEnd);
	if(day.GetDayOfWeek()==1||day.GetDayOfWeek()==7)
		pAuthent->dwTimeEnd+=172800;//48*3600�ܿ���ĩ�Ӻ�����
	pAuthent->dwExpireTimes=7;		//ʱ��������ʱ���ߴλ���
	userDb.SetProductACL(pAcl);

	DWORD dwLicContentsLen=GenerateClientLicenseFile(cApplyProductType,identity.bytes,NULL,NULL);
	CHAR_ARRAY licbytes(dwLicContentsLen);
	if(dwLicContentsLen>0)
	{
		status=1;
		GenerateClientLicenseFile(cApplyProductType,identity.bytes,details+4,licbytes);
	}
	else
	{
		status=0;
		logevent.Log("status=%d",status);
	}
	pCommListen->Write(&status,1,&nBytesWritten);
	pCommListen->GetErrorStr();
	if(status==1)
	{
		pCommListen->Write(&dwLicContentsLen,4,&nBytesWritten);
		return pCommListen->Write(licbytes,licbytes.Size(),&nBytesWritten);
	}
	else
		return FALSE;
}

BOOL CMainWorkLoop::ReturnLicense(CCommunicationObject &commCore,LICSERV_MSGBUF &msg)
{
	DWORD nBytesRead,nBytesWritten;
	CHAR_ARRAY msg_contents(msg.msg_length);
	CBuffer buffer(msg_contents,msg.msg_length);
	commCore.Read(msg_contents,msg.msg_length-32,&nBytesRead);

	IDENTITY identity;
	CXhChar50 hostname;
	buffer.Read(identity.bytes,32);
	buffer.ReadString(hostname,51);
	DecryptFingerprint(identity);
	//��ǰ��ʱ����һ�����ܵ�����ͻ���д��䷢������Ȩ����
	CXhChar200 ListenPipeName(CXhChar50("%s\\%s_ListenPipe",PIPE_PATH,hostname));
	NAMEDPIPE_LIFE pipelife(ListenPipeName);
	char status,cApplyProductType=0;
	buffer.ReadByte(&cApplyProductType);
	IProductAcl* pAcl=hashProductSrvs.GetValue(cApplyProductType);
	if(pAcl==NULL)
	{
		status=-1;	//��������ȱ����Ӧ��Ȩ
		return WriteFile(pipelife.PipeHandle(),&status,1,&nBytesWritten,NULL);
	}
	BOOL removed=FALSE;
	if(pAcl->FromIdentity(identity.bytes))
	{
		removed=pAcl->RemoveIdentity(identity.bytes);
		status=removed?1:-2;	//��Ȩ����ʧ��
		return WriteFile(pipelife.PipeHandle(),&status,1,&nBytesWritten,NULL);
	}
	status=0;
	return WriteFile(pipelife.PipeHandle(),&status,1,&nBytesWritten,NULL);
}

void CMainWorkLoop::LicenseValidate(CCommunicationObject &commCore,LICSERV_MSGBUF &msg)
{	//���¼���Database
	//��ǰ��ʱ����һ�����ܵ�����ͻ���д��䷢������Ȩ����
	CXhChar200 ListenPipeName(CXhChar50("%s\\Self_ListenPipe",PIPE_PATH));
	NAMEDPIPE_LIFE pipelife(ListenPipeName);
	DWORD nBytesRead,nBytesWritten;
	CBuffer buffer((char*)msg.lpBuffer,msg.msg_length);
	commCore.Read(msg.lpBuffer,msg.msg_length,&nBytesRead);
	ENDUSER enduser;
	BYTE ciSrvUICmd=0,cProductType=0;
	buffer.SeekToBegin();
	buffer.ReadByte(&ciSrvUICmd);
	buffer.ReadByte(&cProductType);
	buffer.Read(enduser.identity.bytes,32);
	buffer.ReadString(enduser.hostname,51);
	if(msg.msg_length>0)
	{
		//if(ciSrvUICmd==0)	//��������Ʒ��Ȩ����
		//else 
		if(ciSrvUICmd==1)	//�Ƴ�����Ʒ��Ȩ����
		{	//��ǰ��ʱ����һ�����ܵ�����ͻ���д��䷢������Ȩ����
			char status=0;
			if(cProductType>0&&cProductType<PRODUCT_IBOM)
			{
				if(hashProductSrvs.GetValue(cProductType))
				{
					status=1;
					hashProductSrvs.DeleteNode(cProductType);
				}
			}
			WriteFile(pipelife.PipeHandle(),&status,1,&nBytesWritten,NULL);
		}
		else if(ciSrvUICmd==2)	//�Ƴ�ָ���ն��û���ָ����Ʒ���ն�ʹ����Ȩ
		{
			char status=0;
			if(cProductType>0&&cProductType<PRODUCT_IBOM)
			{
				IProductAcl* pAcl=hashProductSrvs.GetValue(cProductType);
				if(pAcl)
					status=pAcl->RemoveIdentity(enduser.identity.bytes);
			}
			WriteFile(pipelife.PipeHandle(),&status,1,&nBytesWritten,NULL);
		}
	}
}