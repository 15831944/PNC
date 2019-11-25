#include "stdafx.h"
#include "NcPart.h"
#include "ArrayList.h"
#include "DxfFile.h"
#include "NcJg.h"
#include "folder_dialog.h"
#include "direct.h"
#include "SortFunc.h"
#include "LicFuncDef.h"
#include "ParseAdaptNo.h"
#include "XhMath.h"

BOOL CNCPart::m_bDisplayLsOrder=FALSE;
BOOL CNCPart::m_bSortHole=FALSE;
BOOL CNCPart::m_bDeformedProfile=FALSE;
double CNCPart::m_fHoleIncrement = 1.5;
CString CNCPart::m_sExportPartInfoKeyStr;
void GetSysPath(char* startPath);
void MakeDirectory(char *path)
{
	char bak_path[MAX_PATH],drive[MAX_PATH];
	strcpy(bak_path,path);
	char *dir = strtok(bak_path,"/\\");
	if(strlen(dir)==2&&dir[1]==':')
	{
		strcpy(drive,dir);
		strcat(drive,"\\");
		_chdir(drive);
		dir = strtok(NULL,"/\\");
	}
	while(dir)
	{
		_mkdir(dir);
		_chdir(dir);
		dir = strtok(NULL,"/\\");
	}
}
CXhChar500 GetValidWorkDir(const char* work_dir, CPPEModel *pModel = NULL, const char* sub_folder = NULL)
{
	CFileFind fileFind;
	CXhChar500 sWorkDir;
	if(work_dir==NULL||strlen(work_dir)<=0||!fileFind.FindFile(work_dir))
	{	//ָ������·��
		char ss[MAX_PATH];
		GetSysPath(ss);
		CString sFolder(ss);
		if(!InvokeFolderPickerDlg(sFolder))
			return sWorkDir;
		sWorkDir.Copy(sFolder);
	}
	else
		sWorkDir.Copy(work_dir);
	sWorkDir.Append("\\");
	if(pModel&&pModel->m_sTaType.Length()>0)
	{
		sWorkDir.Append(pModel->m_sTaType);
		sWorkDir.Append("\\");
		if(!fileFind.FindFile(sWorkDir))
			MakeDirectory(sWorkDir);
	}
	if (sub_folder != NULL && strlen(sub_folder) > 0)
	{
		sWorkDir.Append(sub_folder);
		sWorkDir.Append("\\");
		if (!fileFind.FindFile(sWorkDir))
			MakeDirectory(sWorkDir);
	}
	return sWorkDir;
}
CXhChar200 GetValidFileName(CPPEModel *pModel,CProcessPart *pPart,const char* sPartNoPrefix=NULL)
{
	CXhChar200 sFileName;
	CXhChar500 sFilePath=pModel->GetPartFilePath(pPart->GetPartNo());
	if(sFilePath.Length()>0)
	{	//����ppi�ļ���ȡ·������ȡ�ļ���
		char drive[8],dir[MAX_PATH],fname[MAX_PATH],ext[10];
		_splitpath(sFilePath,drive,dir,fname,ext);
		sFileName.Copy(fname);
	}
	else
	{	//���ݹ������������ļ���
		CString sSpec=pPart->GetSpec();
		if(sPartNoPrefix)
			sFileName.Printf("%s%s#%s",sPartNoPrefix,(char*)pPart->GetPartNo(),sSpec.Trim());
		else
			sFileName.Printf("%s#%s",(char*)pPart->GetPartNo(),sSpec.Trim());
	}
	return sFileName;
}
static SCOPE_STRU GetPlateVertexScope(CProcessPlate *pPlate,GECS *pCS=NULL)
{
	SCOPE_STRU scope;
	scope.ClearScope();
	f3dPoint axis_len(1,0,0);
	if(pCS)
		axis_len=pCS->axis_x;
	PROFILE_VER* pPreVertex=pPlate->vertex_list.GetTail();
	for(PROFILE_VER *pVertex=pPlate->vertex_list.GetFirst();pVertex;pVertex=pPlate->vertex_list.GetNext())
	{
		f3dPoint pt=pPreVertex->vertex;
		if(pCS)
			coord_trans(pt,*pCS,FALSE);
		scope.VerifyVertex(pt);
		if(pPreVertex->type==2)
		{
			f3dArcLine arcLine;
			arcLine.CreateMethod2(pPreVertex->vertex,pVertex->vertex,pPreVertex->work_norm,pPreVertex->sector_angle);
			f3dPoint startPt=f3dPoint(arcLine.Center())+axis_len*arcLine.Radius();
			f3dPoint endPt=f3dPoint(arcLine.Center())-axis_len*arcLine.Radius();
			if(pCS)
			{
				coord_trans(startPt,*pCS,FALSE);
				coord_trans(endPt,*pCS,FALSE);
			}
			scope.VerifyVertex(startPt);
			scope.VerifyVertex(endPt);
		}
		pPreVertex=pVertex;
	}
	return scope;
}

BOOL GetSysParaFromReg(const char* sEntry,char* sValue)
{
	char sStr[MAX_PATH];
	char sSubKey[MAX_PATH]="Software\\Xerofox\\PPE\\Settings";
	DWORD dwDataType,dwLength=MAX_PATH;
	HKEY hKey;
	if(RegOpenKeyEx(HKEY_CURRENT_USER,sSubKey,0,KEY_READ,&hKey)==ERROR_SUCCESS&&hKey)
	{
		if(RegQueryValueEx(hKey,sEntry,NULL,&dwDataType,(BYTE*)&sStr[0],&dwLength)!= ERROR_SUCCESS)
			return FALSE;
		RegCloseKey(hKey);
	}
	else 
		return FALSE;
	if(sValue && strlen(sStr)>0)
		strcpy(sValue,sStr);
	return TRUE;
}

//ttp��ʽ
//13�ֽ��ļ�����
//1�ֽڱ�ʶΪ��0x80����ʶ�ְ�
//2�ֽڣ�������+��˨��+1��+1Ϊ��ӡ��λ�ã�
//����������ֹ����ܶ�ȡ�����⣬ԭ�����ļ����а���2����ĸ wht 19-04-11
bool CNCPart::CreatePlateTtpFile(CProcessPlate *pPlate,const char* file_path)
{
	if(pPlate==NULL)
		return false;
	CProcessPlate tempPlate;
	pPlate->ClonePart(&tempPlate);
	GECS mcs;
	pPlate->GetMCS(mcs);
	CProcessPlate::TransPlateToMCS(&tempPlate,mcs);
	//
	char drive[8], dir[MAX_PATH], fname[MAX_PATH], ext[10], file_name[MAX_PATH] = {0};
	_splitpath(file_path,drive,dir,fname,ext);
	FILE *fp = fopen(file_path,"wb");	//,ccs=UTF-8
	if(fp==NULL)
		return false;
	//
	CXhChar200 sFileName(fname);
	CXhChar16 sPartLabel = tempPlate.GetPartNo();
	sPartLabel.ToLower();
	sFileName.ToLower();
	sprintf(file_name,"%s.ttp",(char*)sFileName);
	fwrite(file_name,1,13,fp);
	BYTE flag=0x80;	//��ʾ�����ͼ
	fwrite(&flag,1,1,fp);
	short x,y;
	short n = (short)(tempPlate.vertex_list.GetNodeNum() + tempPlate.m_xBoltInfoList.GetNodeNum()) + 1;
	fwrite(&n,2,1,fp);
	fwrite(sPartLabel,1,12,fp);
	for(PROFILE_VER *pVertex=tempPlate.vertex_list.GetFirst();pVertex;pVertex=tempPlate.vertex_list.GetNext())
	{
		f3dPoint pt=pVertex->vertex;
		x = ftoi(pt.x*10);
		y = ftoi(pt.y*10);
		flag = 0x0;	//�߿�������
		fwrite(&x,2,1,fp);
		fwrite(&y,2,1,fp);
		fwrite(&flag,1,1,fp);
	}

	f3dPoint centre;
	for(BOLT_INFO *pHole=tempPlate.m_xBoltInfoList.GetFirst();pHole;pHole=tempPlate.m_xBoltInfoList.GetNext())
	{
		centre.Set(pHole->posX,pHole->posY);
		x = ftoi(centre.x*10);
		y = ftoi(centre.y*10);
		flag = 0x0;
		if(pHole->bolt_d==16)
			flag = 0x01;
		else if(pHole->bolt_d==20)
			flag = 0x02;
		else if(pHole->bolt_d==24)
			flag = 0x03;
		else if(pHole->bolt_d==12)
			flag = 0x04;
		else if(pHole->bolt_d==18||(pHole->bolt_d+pHole->hole_d_increment)==19.5)
			flag = 0x05;
		else if(pHole->bolt_d==22||(pHole->bolt_d+pHole->hole_d_increment)==23.5)
			flag = 0x06;
		fwrite(&x,2,1,fp);
		fwrite(&y,2,1,fp);
		fwrite(&flag,1,1,fp);
	}
	centre.Set(tempPlate.mkpos.x,tempPlate.mkpos.y);
	coord_trans(centre,mcs,FALSE);
	x = ftoi(centre.x*10);
	y = ftoi(centre.y*10);
	flag = 0x07;	//���
	fwrite(&x,2,1,fp);
	fwrite(&y,2,1,fp);
	fwrite(&flag,1,1,fp);
	fclose(fp);
	return true;
}
void CNCPart::CreatePlateTtpFiles(CPPEModel *pModel,CXhPtrSet<CProcessPlate> &plateSet,const char* work_dir)
{
	CXhChar500 sFilePath,sWorkDir=GetValidWorkDir(work_dir,pModel,"TTP");
	if(sWorkDir.Length()<=0)
		return;
	CXhChar200 sFileName;
	for(CProcessPlate *pPlate=plateSet.GetFirst();pPlate;pPlate=plateSet.GetNext())
	{
		sFileName=GetValidFileName(pModel,pPlate);
		if(sFileName.Length()<=0)
			continue;
		sFilePath.Printf("%s\\%s.ttp",(char*)sWorkDir,(char*)sFileName);
		CreatePlateTtpFile(pPlate,sFilePath);
	}
	ShellExecute(NULL,"open",NULL,NULL,sWorkDir,SW_SHOW);
	//WinExec(CXhChar500("explorer.exe %s",(char*)sWorkDir),SW_SHOW);
}
bool CNCPart::CreatePlateWkfFile(CProcessPlate *pPlate, const char* file_path)
{
	if (pPlate == NULL)
		return false;
	CProcessPlate tempPlate;
	pPlate->ClonePart(&tempPlate);
	GECS mcs;
	pPlate->GetMCS(mcs);
	CProcessPlate::TransPlateToMCS(&tempPlate, mcs);
	//
	FILE *fp = fopen(file_path, "wt");
	if (fp == NULL)
		return false;
	//��һ��Ϊ����
	fprintf(fp, "\n");
	//�����������Ϣ
	PROFILE_VER *pPrevVertex = tempPlate.vertex_list.GetTail();
	for (PROFILE_VER *pVertex = tempPlate.vertex_list.GetFirst(); pVertex; pVertex = tempPlate.vertex_list.GetNext())
	{
		double x0 = fabs(pPrevVertex->vertex.x) < EPS ? 0 : pPrevVertex->vertex.x;
		double y0 = fabs(pPrevVertex->vertex.y) < EPS ? 0 : pPrevVertex->vertex.y;
		double x1 = fabs(pVertex->vertex.x) < EPS ? 0 : pVertex->vertex.x;
		double y1 = fabs(pVertex->vertex.y) < EPS ? 0 : pVertex->vertex.y;
		fprintf(fp, "L %.2f %.2f %.2f %.2f\n", x0, y0, x1, y1);
		pPrevVertex = pVertex;
	}
	//�����˨��Ϣ
	for (BOLT_INFO *pHole = tempPlate.m_xBoltInfoList.GetFirst(); pHole; pHole = tempPlate.m_xBoltInfoList.GetNext())
	{
		double x = fabs(pHole->posX) < EPS ? 0 : pHole->posX;
		double y = fabs(pHole->posY) < EPS ? 0 : pHole->posY;
		fprintf(fp, "C %.2f %.2f %.2f\n", x, y, (pHole->bolt_d*0.5));
	}
	//�����ı���Ϣ
	f3dPoint centre(tempPlate.mkpos.x, tempPlate.mkpos.y);
	coord_trans(centre, mcs, FALSE);
	double fTextHeight = 7.5;
	double fPosY = centre.y;
	fprintf(fp, "T %.2f %.2f ����:%s\n", centre.x, fPosY, (char*)tempPlate.GetPartNo());
	fPosY -= fTextHeight;
	fprintf(fp, "T %.2f %.2f ����:%d\n", centre.x, fPosY, tempPlate.m_nDanJiNum);
	fPosY -= fTextHeight;
	fprintf(fp, "T %.2f %.2f ���:%d\n", centre.x, fPosY, (int)tempPlate.m_fThick);
	fPosY -= fTextHeight;
	fprintf(fp, "T %.2f %.2f ����:%s\n", centre.x, fPosY, (char*)model.m_sTaType);
	fclose(fp);
	return true;
}
void CNCPart::CreatePlateWkfFiles(CPPEModel *pModel, CXhPtrSet<CProcessPlate> &plateSet, const char* work_dir)
{
	CXhChar500 sFilePath, sWorkDir = GetValidWorkDir(work_dir, pModel, "WKF");
	if (sWorkDir.Length() <= 0)
		return;
	CXhChar200 sFileName;
	for (CProcessPlate *pPlate = plateSet.GetFirst(); pPlate; pPlate = plateSet.GetNext())
	{
		BOOL bHasArcEdge = FALSE;
		for (PROFILE_VER *pVertex = pPlate->vertex_list.GetFirst(); pVertex; pVertex = pPlate->vertex_list.GetNext())
		{
			if (pVertex->type != 1)
			{
				bHasArcEdge = TRUE;
				break;
			}
		}
		if (bHasArcEdge)
			continue;	//WKF��ʽ�ݲ�֧���������Բ���ߵĸְ� wht 19-06-11
		sFileName = GetValidFileName(pModel, pPlate);
		if (sFileName.Length() <= 0)
			continue;
		sFilePath.Printf("%s\\%s.wkf", (char*)sWorkDir, (char*)sFileName);
		CreatePlateWkfFile(pPlate, sFilePath);
	}
	ShellExecute(NULL, "open", NULL, NULL, sWorkDir, SW_SHOW);
}
static void GetPlateScope(CProcessPlate& tempPlate,SCOPE_STRU& scope)
{
	for(PROFILE_VER* pVertex=tempPlate.vertex_list.GetFirst();pVertex;pVertex=tempPlate.vertex_list.GetNext())
		scope.VerifyVertex(f3dPoint(pVertex->vertex.x,pVertex->vertex.y));
	for(BOLT_INFO *pHole=tempPlate.m_xBoltInfoList.GetFirst();pHole;pHole=tempPlate.m_xBoltInfoList.GetNext())
	{
		double radius=0.5*pHole->bolt_d;
		scope.VerifyVertex(f3dPoint(pHole->posX-radius,pHole->posY-radius));
		scope.VerifyVertex(f3dPoint(pHole->posX+radius,pHole->posY+radius));
	}
}
void CNCPart::DeformedPlateProfile(CProcessPlate *pPlate)
{
	CProcessPlate xUnDeformedPlate;
	pPlate->ClonePart(&xUnDeformedPlate);
	for(BOLT_INFO *pBolt=pPlate->m_xBoltInfoList.GetFirst();pBolt;pBolt=pPlate->m_xBoltInfoList.GetNext())
	{
		f3dPoint ls_pos(pBolt->posX,pBolt->posY,0);
		ls_pos.feature=pBolt->cFaceNo;
		ls_pos=xUnDeformedPlate.GetDeformedVertex(ls_pos);
		pBolt->posX=(float)ls_pos.x;
		pBolt->posY=(float)ls_pos.y;
	}
	for(PROFILE_VER* pVertex=pPlate->vertex_list.GetFirst();pVertex;pVertex=pPlate->vertex_list.GetNext())
		pVertex->vertex=xUnDeformedPlate.GetDeformedVertex(pVertex->vertex);
}
//����ְ��ϵĹ��ߵ�
static void RemoveCollinearPoint(CProcessPlate *pPlate)
{
	ATOM_LIST<PROFILE_VER> xProfileVerArr;
	for(PROFILE_VER *pVertex=pPlate->vertex_list.GetFirst();pVertex;pVertex=pPlate->vertex_list.GetNext())
	{
		PROFILE_VER *pVertexPre=pPlate->vertex_list.GetPrev();
		if(pVertexPre==NULL)
		{
			pVertexPre=pPlate->vertex_list.GetTail();
			pPlate->vertex_list.GetFirst();
		}
		else
			pPlate->vertex_list.GetNext();
		PROFILE_VER *pVertexNext=pPlate->vertex_list.GetNext();
		if(pVertexNext==NULL)
		{
			pVertexNext=pPlate->vertex_list.GetFirst();
			pPlate->vertex_list.GetTail();
		}
		else
			pPlate->vertex_list.GetPrev();
		//
		f3dPoint pre_vec=(pVertex->vertex-pVertexPre->vertex).normalized();
		f3dPoint cur_vec=(pVertex->vertex-pVertexNext->vertex).normalized();
		if(pVertexPre->type==1&&pVertex->type==1&&fabs(pre_vec*cur_vec)>0.9999)
			continue;	//���㹲��
		xProfileVerArr.append(*pVertex);
	}
	//
	pPlate->vertex_list.Empty();
	for(PROFILE_VER *pVertex=xProfileVerArr.GetFirst();pVertex;pVertex=xProfileVerArr.GetNext())
		pPlate->vertex_list.SetValue(pVertex->keyId,*pVertex);
}
bool CNCPart::CreatePlateDxfFile(CProcessPlate *pPlate,const char* file_path,int dxf_mode)
{
	if(pPlate==NULL)
		return false;
	CLogErrorLife logErrLife;
	if(!m_bDeformedProfile && pPlate->m_bIncDeformed)
	{
		logerr.Log("%s�ְ��е��������Ѿ����������δ���",(char*)pPlate->GetPartNo());
		return false;
	}
	GECS mcs;
	pPlate->GetMCS(mcs);
	CProcessPlate tempPlate;
	pPlate->ClonePart(&tempPlate);
	//���о�߹��յĸְ�����ȡ���������
	if(tempPlate.IsRollEdge())
		tempPlate.ProcessRollEdgeVertex();
	//���ɸְ�NC�����迼�ǻ������Σ���Ըְ��еĶ������˨���л������δ���
	if(m_bDeformedProfile && !tempPlate.m_bIncDeformed)
		DeformedPlateProfile(&tempPlate);
	CProcessPlate::TransPlateToMCS(&tempPlate,mcs);
	SCOPE_STRU scope;
	GetPlateScope(tempPlate,scope);
	//
	double fSpecialD=0,fShapeAddDist=0;
	int bNeedSH,iNcMode=0;
	CXhChar100 sValue;
	if(GetSysParaFromReg("LimitSH",sValue))
		fSpecialD=atof(sValue);
	if(GetSysParaFromReg("NeedSH",sValue))
		bNeedSH=atoi(sValue);
	if (dxf_mode == CNCPart::CUT_MODE && GetSysParaFromReg("ShapeAddDist", sValue))
		fShapeAddDist=atof(sValue);
	//�Ƴ����ߵ�֮ǰ��ȡ������λ�� wht 19-09-26
	f3dLine huoquLine[2] = {};
	BYTE bendTypeArr[2] = {0, 0};
	if (dxf_mode == CNCPart::LASER_MODE)
	{
		for (int i = 2; i <= tempPlate.m_cFaceN; i++)
		{
			if (tempPlate.GetBendLineAt(i - 2, &huoquLine[i - 2]) == 0)
			{
				huoquLine[i - 2].startPt.Set();
				huoquLine[i - 2].endPt.Set();
				logerr.Log("��%d������ʼĩ�λ��ƶ������ʧ��", i - 1);
			}
			else
				bendTypeArr[i-2] = tempPlate.GetHuoQuFaceBendType(i);
		}
	}
	ATOM_LIST<PROFILE_VER> xDestList;
	if(tempPlate.m_cFaceN!=3)	//����岻��ִ�й��ߵ�����
		RemoveCollinearPoint(&tempPlate);
	tempPlate.CalEquidistantShape(fShapeAddDist,&xDestList);
	CDxfFile file;
	file.extmin.Set(scope.fMinX,scope.fMaxY);
	file.extmax.Set(scope.fMaxX,scope.fMinY);
	if(file.OpenFile(file_path))
	{
		//����������
		PROFILE_VER* pPrevVertex=xDestList.GetTail();
		for(PROFILE_VER* pVertex=xDestList.GetFirst();pVertex;pVertex=xDestList.GetNext())
		{
			if (pPlate->m_cFaceN < 3 || pPrevVertex->vertex.feature != 3 || pVertex->vertex.feature != 2)
			{
				if (pPrevVertex->type == 1)
					file.NewLine(pPrevVertex->vertex, pVertex->vertex);
				else
				{
					f3dArcLine arcLine;
					if (pPrevVertex->type == 2)
						arcLine.CreateMethod2(pPrevVertex->vertex, pVertex->vertex, pPrevVertex->work_norm, pPrevVertex->sector_angle);
					else
						arcLine.CreateEllipse(pPrevVertex->center, pPrevVertex->vertex, pVertex->vertex, pPrevVertex->column_norm, pPrevVertex->work_norm, pPrevVertex->radius);
					f3dPoint startPt = arcLine.Start();
					f3dPoint endPt = arcLine.End();
					if (arcLine.WorkNorm()*mcs.axis_z < 0)
					{	//���߷���ͬ������Բ����ʼ�ն�
						startPt = arcLine.End();
						endPt = arcLine.Start();
					}
					if (tempPlate.mcsFlg.ciOverturn == 1)
					{	//��ת�����Բ����ʼ�ն�
						f3dPoint tmePt = startPt;
						startPt = endPt;
						endPt = tmePt;
					}
					f3dPoint centre = arcLine.Center();
					double fAngleS = Cal2dLineAng(centre.x, centre.y, startPt.x, startPt.y);
					double fAngleE = Cal2dLineAng(centre.x, centre.y, endPt.x, endPt.y);
					if (pPrevVertex->type == 2)
						file.NewArc(centre.x, centre.y, arcLine.Radius(), fAngleS*DEGTORAD_COEF, fAngleE*DEGTORAD_COEF);
					else
					{	//��Բ�������ֱ�߶�ģ����ʵ,NewEllipse��Ҫ�������� wht 18-12-01
						/*f3dPoint long_axis_pt=arcLine.PositionInAngle(0);
						f3dPoint short_axis_pt=arcLine.PositionInAngle(Pi);
						double long_axis_len=DISTANCE(long_axis_pt,centre);
						double short_axis_len=DISTANCE(short_axis_pt,centre);
						long_axis_pt-=centre;
						double scale=short_axis_len/long_axis_len;
						file.NewEllipse(centre,long_axis_pt,scale,fAngleS*DEGTORAD_COEF,fAngleE*DEGTORAD_COEF);*/
						//
						int nSlices = CalArcResolution(arcLine.Radius(), arcLine.SectorAngle(), 1.0, 3.0, 10);
						double slice_angle = arcLine.SectorAngle() / nSlices;
						f3dPoint pre_pt = pPrevVertex->vertex;
						for (int i = 1; i <= nSlices; i++)
						{
							f3dPoint pt = arcLine.PositionInAngle(i*slice_angle);
							file.NewLine(pre_pt, pt);
							pre_pt = pt;
						}
					}
				}
			}
			else
			{
				file.NewLine(pPrevVertex->vertex, tempPlate.top_point);
				file.NewLine(tempPlate.top_point, pVertex->vertex);
			}
			pPrevVertex=pVertex;
		}
		//������˨��
		f3dPoint centre,startPt;
		for(BOLT_INFO *pHole=tempPlate.m_xBoltInfoList.GetFirst();pHole;pHole=tempPlate.m_xBoltInfoList.GetNext())
		{
			if(startPt.IsZero())
				startPt.Set(pHole->posX,pHole->posY);
			centre.Set(pHole->posX,pHole->posY);
			if (dxf_mode == CNCPart::CUT_MODE)
			{	//�и�����ģʽ�£���������׽��мӹ�
				if (pHole->bolt_d >= fSpecialD)
					file.NewCircle(centre, (pHole->bolt_d + pHole->hole_d_increment) / 2.0);
			}
			else if (dxf_mode == CNCPart::PROCESS_MODE)
			{	//�崲���ģʽ��
				if (pHole->bolt_d <= 24 || pHole->bolt_d < fSpecialD)	//����ͨ������˨�׽��мӹ�
					file.NewCircle(centre, (pHole->bolt_d + pHole->hole_d_increment) / 2.0);
				if (bNeedSH&&pHole->bolt_d >= fSpecialD)		//�����豣�������Ҫ��ģ�������׽��мӹ�
					file.NewCircle(centre, (pHole->bolt_d + pHole->hole_d_increment) / 2.0);
			}
			else if (dxf_mode == CNCPart::LASER_MODE)
			{	//����ӹ�ģʽ�£��������п�
				file.NewCircle(centre, (pHole->bolt_d + pHole->hole_d_increment) / 2.0);
			}
			startPt=centre;
		}
		if ((tempPlate.IsDisplayMK() && dxf_mode == CNCPart::PROCESS_MODE))
		{	//���ƺ��Ͽ�(�崲���ʱ��Ҫ���Ƹ�ӡ��)
			double fMkHoldD=5,fMkRectL=60,fMkRectW=30;
			//���Ͽ�
			if(GetSysParaFromReg("MKHoleD",sValue))
				fMkHoldD=atof(sValue);
			if (fMkHoldD > 0)
			{
				f3dPoint centre(tempPlate.mkpos.x,tempPlate.mkpos.y);
				coord_trans(centre,mcs,FALSE);
				file.NewCircle(f3dPoint(centre.x,centre.y),fMkHoldD/2.0);
			}
			//�ֺ�
			BOOL bDrawRect=0;
			if(GetSysParaFromReg("NeedMKRect",sValue))
				bDrawRect=atoi(sValue);
			if(bDrawRect)
			{
				if(GetSysParaFromReg("MKRectL",sValue))
					fMkRectL=atof(sValue);
				if(GetSysParaFromReg("MKRectW",sValue))
					fMkRectW=atof(sValue);
				if(tempPlate.mkVec.IsZero())
				{
					tempPlate.mkVec.Set(1,0,0);
					vector_trans(tempPlate.mkVec,mcs,TRUE);
				}
				ATOM_LIST<f3dPoint> ptArr;
				tempPlate.GetMkRect(fMkRectL,fMkRectW,ptArr);
				for(int i=0;i<4;i++)
				{
					f3dPoint startPt(ptArr[i]);
					f3dPoint endPt(ptArr[(i+1)%4]);
					coord_trans(startPt,mcs,FALSE);
					coord_trans(endPt,mcs,FALSE);
					file.NewLine(startPt,endPt);
				}
			}	
		}
		if (dxf_mode == CNCPart::LASER_MODE)
		{	//��������߲���+��-��ʶ������ wht 19-09-26
			BOOL bOutputBendLine = FALSE, bOutputBendType = FALSE;
			if (GetSysParaFromReg("nc.LaserPara.m_bOutputBendLine", sValue))
				bOutputBendLine = atoi(sValue);
			if (GetSysParaFromReg("nc.LaserPara.m_bOutputBendType", sValue))
				bOutputBendType = atoi(sValue);
			for (int i = 0; i < tempPlate.m_cFaceN-1; i++)
			{
				if(huoquLine[i].startPt!=huoquLine[i].endPt)
				{
					if(bOutputBendLine)
						file.NewLine(huoquLine[i].startPt, huoquLine[i].endPt, 1);	//���û�����Ϊ��ɫ
					if (bOutputBendType)
					{
						//�ж������������ڻ����߸�����ע + -��
						BYTE cBendType = bendTypeArr[i];
						//��δ��ȡ�����Ƕȣ��޷�ʶ��������� wht 19-10-04
						/*if (cBendType == CProcessPlate::BEND_IN)
							file.NewText("-",);
						else if (cBendType == CProcessPlate::BEND_OUT)
							file.NewText("+");*/
					}
				}
			}
			//����ӹ�ģʽ�£���ʾ�û�ָ������Ϣ
			CStringArray str_arr, sNoteArr;
			CXhChar200 key_str;
			key_str.Copy(m_sExportPartInfoKeyStr);
			for (char *token = strtok(key_str, "\n"); token; token = strtok(NULL, "\n"))
				str_arr.Add(token);
			for (int i = 0; i < str_arr.GetSize(); i++)
			{
				CXhChar100 sNotes, sTemp;
				key_str.Copy(str_arr[i]);
				CXhChar100 sPrevProp;
				for (char *token = strtok(key_str, "&"); token; token = strtok(NULL, "&"))
				{
					if (stricmp(token, "��Ƶ�λ") == 0)
					{
						if (sNotes.GetLength() > 0)
							sNotes.Append(' ');
						sNotes.Append(model.m_sCompanyName);
					}
					else if (stricmp(token, "���̱��") == 0)
					{
						if (sNotes.GetLength() > 0)
							sNotes.Append(' ');
						sNotes.Append(model.m_sPrjCode);
					}
					else if (stricmp(token, "��������") == 0)
					{
						if (sNotes.GetLength() > 0)
							sNotes.Append(' ');
						sNotes.Append(model.m_sPrjName);
					}
					else if (stricmp(token, "����") == 0)
					{
						if (sNotes.GetLength() > 0)
							sNotes.Append(' ');
						sNotes.Append(model.m_sTaType);
					}
					else if (stricmp(token, "����") == 0)
					{
						if (sNotes.GetLength() > 0)
							sNotes.Append(' ');
						sNotes.Append(model.m_sTaAlias);
					}
					else if (stricmp(token, "��ӡ��") == 0)
					{
						if (sNotes.GetLength() > 0)
							sNotes.Append(' ');
						sNotes.Append(model.m_sTaStampNo);
					}
					else if (stricmp(token, "����Ա") == 0)
					{
						if (sNotes.GetLength() > 0)
							sNotes.Append(' ');
						sNotes.Append(model.m_sOperator);
					}
					else if (stricmp(token, "�����") == 0)
					{
						if (sNotes.GetLength() > 0)
							sNotes.Append(' ');
						sNotes.Append(model.m_sAuditor);
					}
					else if (stricmp(token, "������") == 0)
					{
						if (sNotes.GetLength() > 0)
							sNotes.Append(' ');
						sNotes.Append(model.m_sCritic);
					}
					else if (stricmp(token, "����") == 0)
					{	//�򻯲����ַ��ڼ���֮ǰ����Ҫ�ڼ���ǰ�ӿո� wht 19-11-05
						if (!sPrevProp.EqualNoCase("�򻯲����ַ�")&&sNotes.GetLength() > 0)
							sNotes.Append(' ');
						sNotes.Append(tempPlate.GetPartNo());
					}
					else if (stricmp(token, "����") == 0)
					{
						char steelmark[20] = "";
						CProcessPart::QuerySteelMatMark(tempPlate.cMaterial, steelmark);
						if (sNotes.GetLength() > 0)
							sNotes.Append(' ');
						sNotes.Append(steelmark);
					}
					else if (stricmp(token, "�򻯲����ַ�") == 0)
					{	//�򻯲����ַ��ڼ���֮����Ҫ�ڼ��ַ�ǰ�ӿո� wht 19-11-05
						char steelmark[20] = "";
						CProcessPart::QuerySteelMatMark(tempPlate.cMaterial, steelmark);
						if (stricmp(steelmark, "Q235") != 0)
						{
							if (!sPrevProp.EqualNoCase("����") && sNotes.GetLength() > 0)
								sNotes.Append(' ');
							sNotes.Append(toupper(tempPlate.cMaterial));
						}
					}
					else if (stricmp(token, "���") == 0)
					{
						if (sNotes.GetLength() > 0)
							sNotes.Append(' ');
						sTemp.Printf("%.0f", tempPlate.GetThick());
						sNotes.Append(sTemp);
					}
					sPrevProp.Copy(token);
				}
				sNoteArr.Add(sNotes);
			}
			if (sNoteArr.GetSize() > 0)
			{
				double fTextW = 0, fTextH = 0, fFontH = 10;
				if (GetSysParaFromReg("TextHeight", sValue))
					fFontH = atof(sValue);
				for (int i = 0; i < sNoteArr.GetSize(); i++)
				{
					double len = strlen(sNoteArr[i])*fFontH*0.7;
					fTextW = max(fTextW, len);
					fTextH += fFontH * 1.4;
				}
				double fLineH = fTextH / sNoteArr.GetSize();
				//
				f3dPoint dimVec(1, 0, 0), offsetVec, dimPt;
				dimPt.Set(tempPlate.mkpos.x, tempPlate.mkpos.y);
				coord_trans(dimPt, mcs, FALSE);
				offsetVec.Set(-dimVec.y, dimVec.x, 0);
				for (int i = 0; i < sNoteArr.GetSize(); i++)
				{
					f3dPoint dimPos = dimPt + offsetVec * fLineH*i - dimVec * fTextW*0.5;
					file.NewText(sNoteArr[i], dimPos, fFontH);
				}
			}
		}
		file.CloseFile();
		return true;
	}
	else 
		return false;
}
#ifdef __PNC_
void CNCPart::CreatePlatePncDxfFiles(CPPEModel *pModel, CXhPtrSet<CProcessPlate> &plateSet, const char* work_dir)
{
	//PNC�û�֧�ֶ���ģʽ�µ�DXF�ļ�
	CHashList<PLATE_GROUP> hashPlateByThick;
	for (CProcessPart *pPart = plateSet.GetFirst(); pPart; pPart = plateSet.GetNext())
	{
		int thick = (int)(pPart->GetThick());
		PLATE_GROUP* pPlateGroup = hashPlateByThick.GetValue(thick);
		if (pPlateGroup == NULL)
		{
			pPlateGroup = hashPlateByThick.Add(thick);
			pPlateGroup->thick = thick;
		}
		pPlateGroup->plateSet.append((CProcessPlate*)pPart);
	}
	CXhChar500 sFilePath, sWorkDir = GetValidWorkDir(work_dir, pModel);
	if (sWorkDir.Length() <= 0)
		return;
	CFileFind fileFind;
	int iNcMode = 1, iDxfMode = 0;
	CXhChar100 sValue, sFileName;
	if (GetSysParaFromReg("NCMode", sValue))
		iNcMode = atoi(sValue);
	if (GetSysParaFromReg("DxfMode", sValue))
		iDxfMode = atoi(sValue);
	CXhChar500 sDxfWorkDir;
	int index = 0, num = pModel->PartCount();
	if ((iNcMode & CNCPart::CUT_MODE) > 0)
	{	//�и�����DXF
		index = 0;
		sDxfWorkDir.Printf("%s\\�и�����", (char*)sWorkDir);
		pModel->DisplayProcess(0, "���������и����ϵ�DXF�ļ�����");
		for (PLATE_GROUP *pPlateGroup = hashPlateByThick.GetFirst(); pPlateGroup; pPlateGroup = hashPlateByThick.GetNext())
		{
			for (CProcessPlate *pPlate = pPlateGroup->plateSet.GetFirst(); pPlate; pPlate = pPlateGroup->plateSet.GetNext())
			{
				index++;
				pModel->DisplayProcess(ftoi(100 * index / num), "���������и����ϵ�DXF�ļ�����");
				sFileName = GetValidFileName(pModel, pPlate);
				if (sFileName.Length() <= 0)
					continue;
				if (iDxfMode == 1)
				{	//����ȴ����ļ�Ŀ¼
					sFilePath.Printf("%s\\���-%d-", (char*)sDxfWorkDir, pPlateGroup->thick);
					if (pPlate->cMaterial == 'S')
						sFilePath.Append("Q235");
					else if (pPlate->cMaterial == 'H')
						sFilePath.Append("Q345");
					else if (pPlate->cMaterial == 'h')
						sFilePath.Append("Q355");
					else if (pPlate->cMaterial == 'P')
						sFilePath.Append("Q420");
				}
				else
					sFilePath.Printf("%s", (char*)sDxfWorkDir);
				if (!fileFind.FindFile(sFilePath))
					MakeDirectory(sFilePath);
				sFilePath.Printf("%s\\%s.dxf", (char*)sFilePath, (char*)sFileName);
				CreatePlateDxfFile(pPlate, sFilePath, CNCPart::CUT_MODE);
			}
		}
		pModel->DisplayProcess(100, "���������и����ϵ�DXF�ļ����");
	}
	if ((iNcMode & CNCPart::PROCESS_MODE) > 0)
	{	//�崲�ӹ�DXF
		index = 0;
		sDxfWorkDir.Printf("%s\\�崲�ӹ�", (char*)sWorkDir);
		pModel->DisplayProcess(0, "�������ڰ崲�ӹ���DXF�ļ�����");
		int index = 0, num = pModel->PartCount();
		for (PLATE_GROUP *pPlateGroup = hashPlateByThick.GetFirst(); pPlateGroup; pPlateGroup = hashPlateByThick.GetNext())
		{
			for (CProcessPlate *pPlate = pPlateGroup->plateSet.GetFirst(); pPlate; pPlate = pPlateGroup->plateSet.GetNext())
			{
				index++;
				pModel->DisplayProcess(ftoi(100 * index / num), "�������ڰ崲�ӹ���DXF�ļ�����");
				sFileName = GetValidFileName(pModel, pPlate);
				if (sFileName.Length() <= 0)
					continue;
				if (iDxfMode == 1)
				{	//����ȴ����ļ�Ŀ¼
					sFilePath.Printf("%s\\���-%d-", (char*)sDxfWorkDir, pPlateGroup->thick);
					if (pPlate->cMaterial == 'S')
						sFilePath.Append("Q235");
					else if (pPlate->cMaterial == 'H')
						sFilePath.Append("Q345");
					else if (pPlate->cMaterial == 'h')
						sFilePath.Append("Q355");
					else if (pPlate->cMaterial == 'P')
						sFilePath.Append("Q420");
				}
				else
					sFilePath.Printf("%s", (char*)sDxfWorkDir);
				if (!fileFind.FindFile(sFilePath))
					MakeDirectory(sFilePath);
				sFilePath.Printf("%s\\%s.dxf", (char*)sFilePath, (char*)sFileName);
				CreatePlateDxfFile(pPlate, sFilePath, CNCPart::PROCESS_MODE);
			}
		}
		pModel->DisplayProcess(100, "�������ڰ崲�ӹ���DXF�ļ����");
	}
	if ((iNcMode & CNCPart::LASER_MODE) > 0)
	{
		index = 0;
		sDxfWorkDir.Printf("%s\\���⸴�ϻ�", (char*)sWorkDir);
		pModel->DisplayProcess(0, "�������ڼ��⸴�ϻ���DXF�ļ�����");
		for (PLATE_GROUP *pPlateGroup = hashPlateByThick.GetFirst(); pPlateGroup; pPlateGroup = hashPlateByThick.GetNext())
		{
			for (CProcessPlate *pPlate = pPlateGroup->plateSet.GetFirst(); pPlate; pPlate = pPlateGroup->plateSet.GetNext())
			{
				index++;
				pModel->DisplayProcess(ftoi(100 * index / num), "�������ڼ��⸴�ϻ���DXF�ļ�����");
				sFileName = GetValidFileName(pModel, pPlate);
				if (sFileName.Length() <= 0)
					continue;
				if (iDxfMode == 1)
				{	//����ȴ����ļ�Ŀ¼
					sFilePath.Printf("%s\\���-%d-", (char*)sDxfWorkDir, pPlateGroup->thick);
					if (pPlate->cMaterial == 'S')
						sFilePath.Append("Q235");
					else if (pPlate->cMaterial == 'H')
						sFilePath.Append("Q345");
					else if (pPlate->cMaterial == 'h')
						sFilePath.Append("Q355");
					else if (pPlate->cMaterial == 'P')
						sFilePath.Append("Q420");
				}
				else
					sFilePath.Printf("%s", (char*)sDxfWorkDir);
				if (!fileFind.FindFile(sFilePath))
					MakeDirectory(sFilePath);
				sFilePath.Printf("%s\\%s.dxf", (char*)sFilePath, (char*)sFileName);
				CreatePlateDxfFile(pPlate, sFilePath, CNCPart::LASER_MODE);
			}
		}
		pModel->DisplayProcess(100, "�������ڼ��⸴�ϻ���DXF�ļ�����");
	}
	ShellExecute(NULL, "open", NULL, NULL, sWorkDir, SW_SHOW);
	//WinExec(CXhChar500("explorer.exe %s",(char*)sWorkDir),SW_SHOW);
}
#endif
void CNCPart::CreatePlateDxfFiles(CPPEModel *pModel,CXhPtrSet<CProcessPlate> &plateSet,const char* work_dir)
{
	//��ͨ�û�ֻ֧�ְ崲�ӹ�ģʽ�µ�DXF�ļ�
	CXhChar500 sFilePath,sWorkDir=GetValidWorkDir(work_dir,pModel);
	if(sWorkDir.Length()<=0)
		return;
	CXhChar200 sFileName;
	pModel->DisplayProcess(0,"����DXF�ļ�����");
	int i=0,num=plateSet.GetNodeNum();
	for(CProcessPlate *pPlate=plateSet.GetFirst();pPlate;pPlate=plateSet.GetNext())
	{
		sFileName=GetValidFileName(pModel,pPlate);
		if(sFileName.Length()<=0)
			continue;
		pModel->DisplayProcess(ftoi(100*i/num),"����DXF�ļ�����");
		sFilePath.Printf("%s\\%s.dxf",(char*)sWorkDir,(char*)sFileName);
		CreatePlateDxfFile(pPlate,sFilePath,CNCPart::PROCESS_MODE);
	}
	pModel->DisplayProcess(100,"����DXF�ļ����");
	ShellExecute(NULL,"open",NULL,NULL,sWorkDir,SW_SHOW);
	//WinExec(CXhChar500("explorer.exe %s",(char*)sWorkDir),SW_SHOW);
}
#ifdef __PNC_
//����ͬ����˨�������׾���С���бȽ�
int compare_boltInfo(const DRILL_BOLT_INFO& pBoltInfo1,const DRILL_BOLT_INFO& pBoltInfo2)
{
	return compare_double(pBoltInfo1.fHoleD, pBoltInfo2.fHoleD);
}
//����ͬ�׾�����˨�䵽ԭ�����̾�����бȽ�
int compare_boltInfo2(const DRILL_BOLT_INFO& pBoltInfo1,const DRILL_BOLT_INFO& pBoltInfo2)
{
	return compare_double(pBoltInfo1.fMinDist, pBoltInfo2.fMinDist);
}

void CNCPart::InitDrillBoltHashTbl(CProcessPlate *pPlate, CHashList<CDrillBolt>& hashDrillBoltByD,
								   BOOL bMergeHole /*= FALSE*/, BOOL bIncSpecialHole /*= TRUE*/, BOOL bDrillGroupSort/*=FALSE*/)
{	
	f3dPoint startPt(0, 0, 0), ls_pos;
	//1��������˨�Ŀ׾�����˨���з���,������
	double fSpecialD = 0;
	CXhChar100 sValue;
	if (GetSysParaFromReg("LimitSH", sValue))
		fSpecialD = atof(sValue);
	//���ֱ�׼�׾�
	const double D16_HOLE = 16 + CNCPart::m_fHoleIncrement;
	const double D20_HOLE = 20 + CNCPart::m_fHoleIncrement;
	const double D24_HOLE = 24 + CNCPart::m_fHoleIncrement;
	ARRAY_LIST<DRILL_BOLT_INFO> drillInfoArr;
	for (BOLT_INFO* pBolt = pPlate->m_xBoltInfoList.GetFirst(); pBolt; pBolt = pPlate->m_xBoltInfoList.GetNext())
	{
		if (pBolt->bolt_d <= 0)
			continue;
		double fHoleD = pBolt->bolt_d + pBolt->hole_d_increment;
		if (bMergeHole)
		{	//��Ҫ�ϲ���ʱ�ڴ˴���������� wht 19-07-25
			BOOL bSpecialHole = (fSpecialD > 0 && fHoleD >= fSpecialD);
			if (!bIncSpecialHole&&bSpecialHole)
				continue;	//���˵������ wht 19-07-25
		}
		DRILL_BOLT_INFO* pInfo = NULL;
		ls_pos.Set(pBolt->posX, pBolt->posY, 0);
		double fDist = DISTANCE(startPt, ls_pos);
		for (pInfo = drillInfoArr.GetFirst(); pInfo; pInfo = drillInfoArr.GetNext())
		{
			if (pInfo->fHoleD == fHoleD)
			{
				pInfo->nBoltNum++;
				if (pInfo->fMinDist > fDist)
					pInfo->fMinDist = fDist;
				break;
			}
		}
		if (pInfo == NULL)
		{
			pInfo = drillInfoArr.append();
			pInfo->fHoleD = fHoleD;
			pInfo->nBoltNum++;
			pInfo->fMinDist = fDist;
		}
	}
	CHashList<double> hashDrillDByHoleD;	//��¼����ӳ���ϵ
	if (bMergeHole)
	{	//��С������׾��ķǱ�ף�ʹ�ñ�׼��\��ͷ�ӹ� wht 19-07-25

		for (int i = 0; i < drillInfoArr.GetSize(); i++)
		{
			if (drillInfoArr[i].fHoleD == D16_HOLE ||
				drillInfoArr[i].fHoleD == D20_HOLE ||
				drillInfoArr[i].fHoleD == D24_HOLE)
				continue;	//��׼��
			double fDestHoleD = 0;
			if (drillInfoArr[i].fHoleD > D24_HOLE)
				fDestHoleD = D24_HOLE;
			else if (drillInfoArr[i].fHoleD > D20_HOLE)
				fDestHoleD = D20_HOLE;
			else if (drillInfoArr[i].fHoleD > D16_HOLE)
				fDestHoleD = D16_HOLE;
			else
				continue;
			for (int j = 0; j < drillInfoArr.GetSize(); j++)
			{
				if (j == i)
					continue;
				if (drillInfoArr[j].fHoleD == fDestHoleD)
				{	//���Ǳ�׺ϲ�����ǰ�飬����¼�׶�Ӧ��ϵ
					drillInfoArr[j].nBoltNum += drillInfoArr[i].nBoltNum;
					if (drillInfoArr[i].fMinDist < drillInfoArr[j].fMinDist)
						drillInfoArr[j].fMinDist = drillInfoArr[i].fMinDist;
					int key = ftoi(drillInfoArr[i].fHoleD * 10);
					hashDrillDByHoleD.SetValue(key, drillInfoArr[j].fHoleD);
					drillInfoArr.RemoveAt(i);
					break;
				}
			}
		}
		DRILL_BOLT_INFO *pDrillInfo175 = NULL, *pDrillInfo215 = NULL, *pDrillInfo255 = NULL;
		for (int i = 0; i < drillInfoArr.GetSize(); i++)
		{
			if (drillInfoArr[i].fHoleD == D16_HOLE)
				pDrillInfo175 = &drillInfoArr[i];
			if (drillInfoArr[i].fHoleD == D20_HOLE)
				pDrillInfo215 = &drillInfoArr[i];
			if (drillInfoArr[i].fHoleD == D24_HOLE)
				pDrillInfo255 = &drillInfoArr[i];
		}
		//���ݹ���ϲ���ͷ���ݣ�����緶���ƣ� wht 19-07-25
		if (pDrillInfo175&&pDrillInfo215&&pDrillInfo255&&drillInfoArr.GetSize() == 3)
		{
			BOOL bDeleteDrillInfo215 = FALSE;
			if (pDrillInfo255->nBoltNum >= pDrillInfo215->nBoltNum)
			{	//25.5���� > 21.5������21.5�׾�����17.5��Ȼ���˹����ף�T1Ĭ��Ϊ��ӡλ�ã�T2Ϊ25.5��ͷ��T3Ϊ17.5��ͷ��
				//pDrillInfo215==>pDrillInfo175 21.5�ϲ���17.5
				pDrillInfo175->nBoltNum += pDrillInfo215->nBoltNum;
				if (pDrillInfo215->fMinDist < pDrillInfo175->fMinDist)
					pDrillInfo175->fMinDist = pDrillInfo215->fMinDist;
				int key = ftoi(pDrillInfo215->fHoleD * 10);
				hashDrillDByHoleD.SetValue(key, pDrillInfo175->fHoleD);
				bDeleteDrillInfo215 = TRUE;
			}
			else //if(pDrillInfo3->nBoltNum<pDrillInfo2->nBoltNum)
			{	//25.5���� < 21.5������25.5�׾�����21.5��Ȼ���˹����ף�T1Ĭ��Ϊ��ӡλ�ã�T2Ϊ21.5��ͷ��T3Ϊ17.5��ͷ��
				//pDrillInfo255==>pDrillInfo215 25.5�ϲ���21.5
				pDrillInfo215->nBoltNum += pDrillInfo255->nBoltNum;
				if (pDrillInfo255->fMinDist < pDrillInfo215->fMinDist)
					pDrillInfo215->fMinDist = pDrillInfo255->fMinDist;
				int key = ftoi(pDrillInfo255->fHoleD * 10);
				hashDrillDByHoleD.SetValue(key, pDrillInfo215->fHoleD);
				bDeleteDrillInfo215 = FALSE;
			}
			for (int i = 0; i < drillInfoArr.GetSize(); i++)
			{
				if ((bDeleteDrillInfo215&&pDrillInfo215 == &drillInfoArr[i]) ||
					(!bDeleteDrillInfo215&&pDrillInfo255 == &drillInfoArr[i]))
					drillInfoArr.RemoveAt(i);
			}
		}
	}
	int	iSortType = (GetSysParaFromReg("GroupSortType", sValue)) ? atoi(sValue) : 0;
	if (iSortType == 1)
		CQuickSort<DRILL_BOLT_INFO>::QuickSort(drillInfoArr.m_pData, drillInfoArr.GetSize(), compare_boltInfo);
	else
		CQuickSort<DRILL_BOLT_INFO>::QuickSort(drillInfoArr.m_pData, drillInfoArr.GetSize(), compare_boltInfo2);
	//2�����hashDrillBoltByD
	for (int i = 0; i < drillInfoArr.GetSize(); i++)
	{
		CDrillBolt* pDrillBolt = hashDrillBoltByD.Add(ftoi(drillInfoArr[i].fHoleD * 10));
		pDrillBolt->fHoleD = drillInfoArr[i].fHoleD;
		for (BOLT_INFO* pBolt = pPlate->m_xBoltInfoList.GetFirst(); pBolt; pBolt = pPlate->m_xBoltInfoList.GetNext())
		{
			double fCurHoleD = pBolt->bolt_d + pBolt->hole_d_increment;
			for (int j = 0; j < 2; j++)
			{	//������˨�׾���ѯ��Ӧ�ļӹ��׾�,�����Ҫ��ѯ���Σ�һ��Ϊ�Ǳ��תΪ��׼�ף�һ��Ϊ��׼�׺ϲ��� wht 19-07-25
				int key = ftoi(fCurHoleD * 10);
				double *pfValue = hashDrillDByHoleD.GetValue(key);
				if (pfValue)
					fCurHoleD = *pfValue;
			}
			if (fCurHoleD != pDrillBolt->fHoleD)
				continue;
			BOLT_INFO newBolt;
			newBolt.CloneBolt(pBolt);
			pDrillBolt->boltList.append(newBolt);
		}
	}
}

void CNCPart::OptimizeBolt( CProcessPlate *pPlate,CHashList<CDrillBolt>& hashDrillBoltByD,
							BOOL bSortByHoleD/*=TRUE*/, BYTE ciAlgType /*= 0*/, 
							BOOL bMergeHole /*= FALSE*/, BOOL bIncSpecialHole /*= TRUE*/)
{
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_HOLE_ROUTER))
		return;
	if (bSortByHoleD)
		InitDrillBoltHashTbl(pPlate, hashDrillBoltByD, bMergeHole, bIncSpecialHole, TRUE);
	else
	{	//�����ݿ׾����з���
		CDrillBolt* pDrillBolt=hashDrillBoltByD.Add(0);
		for(BOLT_INFO* pBolt=pPlate->m_xBoltInfoList.GetFirst();pBolt;pBolt=pPlate->m_xBoltInfoList.GetNext())
		{
			BOLT_INFO newBolt;
			newBolt.CloneBolt(pBolt);
			pDrillBolt->fHoleD=pBolt->bolt_d+pBolt->hole_d_increment;
			pDrillBolt->boltList.append(newBolt);
		}
	}
	//3���Բ�ͬ�׾���˨����·���Ż�
	f3dPoint startPos;
	for(CDrillBolt* pDrillBolt=hashDrillBoltByD.GetFirst();pDrillBolt;pDrillBolt=hashDrillBoltByD.GetNext())
		pDrillBolt->OptimizeBoltOrder(startPos, ciAlgType);
}
#endif
void CNCPart::InitStoreMode(CHashList<CDrillBolt>& hashDrillBoltByD, ARRAY_LIST<double> &holeDList, BOOL bIncSH /*= TRUE*/)
{
	if (hashDrillBoltByD.GetNodeNum() <= 0)
		return;
	CXhChar100 sValue;
	double fSpecialD = 0;
	if (GetSysParaFromReg("LimitSH", sValue))
		fSpecialD = atof(sValue);
	//1.ͳ����˨ֱ����������С��������
	holeDList.Empty();
	CDrillBolt* pDrillBolt = NULL;
	for (pDrillBolt = hashDrillBoltByD.GetFirst(); pDrillBolt; pDrillBolt = hashDrillBoltByD.GetNext())
	{
		BOOL bSpecialBolt = TRUE;
		if (fSpecialD > 0 && pDrillBolt->fHoleD < fSpecialD)
			bSpecialBolt = FALSE;
		if (!bIncSH && bSpecialBolt)
			continue;	//�û��趨�����������
		holeDList.append(pDrillBolt->fHoleD);
	}
	CHeapSort<double>::HeapSort(holeDList.m_pData, holeDList.GetSize(), compare_double);
	//2.������˨������
	if (holeDList.GetSize() == 1 && holeDList[0] >= 17.5 && holeDList[0] < 20)
	{	//ֻ��M16����˨(�׾�����ֵ��Χ1.5-4)ʱ������ʹ��T3��ͷ
		pDrillBolt = hashDrillBoltByD.GetValue(ftoi(holeDList[0] * 10));
		pDrillBolt->biMode = CDrillBolt::DRILL_T3;
	}
	else
	{	//�ж��п׾�ʱ��Ĭ��T2Ϊ��ף�T3ΪС��
		for (int i = 0; i < holeDList.GetSize(); i += 2)
		{
			if (i + 1 < holeDList.GetSize())
			{
				pDrillBolt = hashDrillBoltByD.GetValue(ftoi(holeDList[i] * 10));
				pDrillBolt->biMode = CDrillBolt::DRILL_T3;
				pDrillBolt = hashDrillBoltByD.GetValue(ftoi(holeDList[i + 1] * 10));
				pDrillBolt->biMode = CDrillBolt::DRILL_T2;
			}
			else
			{
				pDrillBolt = hashDrillBoltByD.GetValue(ftoi(holeDList[i] * 10));
				pDrillBolt->biMode = CDrillBolt::DRILL_T2;
			}
		}
	}
}
//���¸��¸ְ����˨����Ϣ
void CNCPart::RefreshPlateHoles(CProcessPlate *pPlate,BOOL bSortByHoleD/*=TRUE*/, BYTE ciAlgType/*=0*/)
{
#ifdef __PNC_
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_HOLE_ROUTER))
		return;
	if(pPlate==NULL)
		return;
	GECS mcs;
	CHashList<CDrillBolt> hashDrillBoltByD;
	CProcessPlate tempPlate;
	pPlate->ClonePart(&tempPlate);
	pPlate->GetMCS(mcs);
	CProcessPlate::TransPlateToMCS(&tempPlate,mcs);
	CXhChar100 sValue;
	BOOL bIncSH = FALSE, bMergeHole = FALSE;
	if (GetSysParaFromReg("PbjIncSH", sValue))
		bIncSH = atoi(sValue);
	if (GetSysParaFromReg("PbjMergeHole", sValue))
		bMergeHole = atoi(sValue);
	OptimizeBolt(&tempPlate,hashDrillBoltByD,bSortByHoleD,ciAlgType,bMergeHole,bIncSH);
	//����д����˨
	pPlate->m_xBoltInfoList.Empty();
	for(CDrillBolt* pDrillBolt=hashDrillBoltByD.GetFirst();pDrillBolt;pDrillBolt=hashDrillBoltByD.GetNext())
	{
		for(BOLT_INFO* pBolt=pDrillBolt->boltList.GetFirst();pBolt;pBolt=pDrillBolt->boltList.GetNext())
		{
			BOLT_INFO* pNewBolt=pPlate->m_xBoltInfoList.Add(0);
			f3dPoint ls_pos(pBolt->posX,pBolt->posY,0);
			coord_trans(ls_pos,mcs,TRUE);
			pNewBolt->CloneBolt(pBolt);
			pNewBolt->posX=(float)ls_pos.x;
			pNewBolt->posY=(float)ls_pos.y;
		}
	}
#endif
}
BOOL CNCPart::IsNeedCreateHoleFile(CProcessPlate *pPlate,BYTE ciHoleProcessType)
{
	if(pPlate==NULL)
		return FALSE;
	if(pPlate->m_xBoltInfoList.GetNodeNum()<=0)
		return FALSE;
	double fSpecialD=0;
	int bNeedSH=0,nThick=0;
	CXhChar100 sValue,sThick;
	if(GetSysParaFromReg("LimitSH",sValue))
		fSpecialD=atof(sValue);
	if(GetSysParaFromReg("NeedSH",sValue))
		bNeedSH=atoi(sValue);
	CHashList<SEGI> hashThick;
	if(ciHoleProcessType==0&&GetSysParaFromReg("ThickToPBJ",sThick))
		GetSegNoHashTblBySegStr(sThick,hashThick);
	if(ciHoleProcessType==1&&GetSysParaFromReg("ThickToPMZ",sThick))
		GetSegNoHashTblBySegStr(sThick,hashThick);
	//���ݰ�����ѡ��׼ӹ���ʽ
	BOOL bValid=TRUE;
	if(hashThick.GetNodeNum()>0&&hashThick.GetValue(ftoi(pPlate->m_fThick))==NULL)
		bValid=FALSE;
	if(bValid)
	{	//���ݿ׾��ж��Ƿ���Ҫ���мӹ���
		BOLT_INFO* pBolt=NULL;
		for(pBolt=pPlate->m_xBoltInfoList.GetFirst();pBolt;pBolt=pPlate->m_xBoltInfoList.GetNext())
		{
			if((fSpecialD>0 && pBolt->bolt_d<fSpecialD)||(bNeedSH && pBolt->bolt_d>=fSpecialD))
				return TRUE;
		}
	}
	return FALSE;
}
#ifdef __PNC_
bool CNCPart::CreatePlatePbjFile(CProcessPlate *pPlate, const char* file_path)
{
	if (!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_PBJ_FILE))
		return false;
	if (pPlate == NULL)
		return false;
	CXhChar16 sValue;
	BOOL bIncVertex = FALSE, bAutoSplitFile = FALSE, bIncSH = FALSE, bMergeHole = FALSE;
	if (GetSysParaFromReg("PbjIncVertex", sValue))
		bIncVertex = atoi(sValue);
	if (GetSysParaFromReg("PbjAutoSplitFile", sValue))
		bAutoSplitFile = atoi(sValue);
	if (GetSysParaFromReg("PbjIncSH", sValue))
		bIncSH = atoi(sValue);
	if (GetSysParaFromReg("PbjMergeHole", sValue))
		bMergeHole = atoi(sValue);
	CLogErrorLife logErrLife;
	CProcessPlate tempPlate;
	pPlate->ClonePart(&tempPlate);
	GECS mcs;
	pPlate->GetMCS(mcs);
	CProcessPlate::TransPlateToMCS(&tempPlate, mcs);
	CHashList<CDrillBolt> hashDrillBoltByD;
	if (m_bSortHole)
		OptimizeBolt(&tempPlate, hashDrillBoltByD,TRUE,0,bMergeHole,bIncSH);	//����˨����˳���Ż�
	else
	{	//���ݿ׾���ʼ����˨��Ϣ
		InitDrillBoltHashTbl(&tempPlate, hashDrillBoltByD, bMergeHole, bIncSH, FALSE);
	}
	ARRAY_LIST<double> holeDList;
	InitStoreMode(hashDrillBoltByD, holeDList, bIncSH);
	if (!bAutoSplitFile&&holeDList.GetSize() > 2)
	{	//���������˨ֱ���������2ʱ�������� wht 19-07-20
		if (holeDList.GetSize() > 2)
			logerr.Log("%s�ְ��ϲ�ͬ�׾�����2�֣�", (char*)tempPlate.GetPartNo());
		//д���ļ�
		FILE* fp = fopen(file_path, "wt");
		if (fp)
		{
			fprintf(fp, "P:%g\n", tempPlate.GetThick());      //д��P:���
			fprintf(fp, "T1:MK\n");                          //д��T1:MK
			//д����������ֱ��
			int iStart = 2; //��T2��ʼ����
			int n = holeDList.GetSize();
			for (int i = 0; i < n; i++)
			{
				int nKey0 = ftoi(holeDList[i] * 10);
				int nKey1 = ((i + 1) < n) ? ftoi(holeDList[i + 1] * 10) : 0;
				CDrillBolt *pDrillBolt0 = hashDrillBoltByD.GetValue(nKey0);
				CDrillBolt *pDrillBolt1 = nKey1 > 0 ? hashDrillBoltByD.GetValue(nKey1) : NULL;
				if (pDrillBolt0&&pDrillBolt1)
				{
					fprintf(fp, "T%d:%.1f\n", iStart + i, holeDList[i + 1]);
					fprintf(fp, "T%d:%.1f\n", iStart + i + 1, holeDList[i]);
					pDrillBolt1->biMode = i + 1;
					pDrillBolt0->biMode = i;
				}
				else if (pDrillBolt0)
				{
					fprintf(fp, "T%d:%.1f\n", iStart + i, holeDList[i]);
					pDrillBolt0->biMode = i;
				}
				i++;
			}
			for (int i = 0; i < holeDList.GetSize(); i++)
			{	//д�벻ֱͬ������˨����
				double fHoleD = holeDList[i];
				int nKey = ftoi(fHoleD * 10);
				CXhChar50 sValueX, sValueY;
				CDrillBolt* pDrillBolt = hashDrillBoltByD.GetValue(nKey);
				if (pDrillBolt)
				{
					fprintf(fp, "T%d\n", iStart + pDrillBolt->biMode);
					for (BOLT_INFO* pBolt = pDrillBolt->boltList.GetFirst(); pBolt; pBolt = pDrillBolt->boltList.GetNext())
						fprintf(fp, "X%.1f Y%.1f\n", pBolt->posX, pBolt->posY);
					//fprintf(fp, "X%g Y%g\n", pBolt->posX, pBolt->posY);
				}
				else
				{
					logerr.Log("%s�ְ�PBJ�ļ�����ʧ�ܣ�", (char*)tempPlate.GetPartNo());
					return false;
				}
			}
			//
			if (tempPlate.IsDisplayMK())
			{
				fprintf(fp, "MK\n");		//д��MK
				coord_trans(tempPlate.mkpos, mcs, FALSE);
				fprintf(fp, "X%.1f Y%.1f\n", tempPlate.mkpos.x, tempPlate.mkpos.y);//д��MK����
			}
			fprintf(fp, "M30\n");
			if (bIncVertex)
			{
				for (PROFILE_VER* pVer = tempPlate.vertex_list.GetFirst(); pVer; pVer = tempPlate.vertex_list.GetNext())  //дplat����������
				{
					double x = pVer->vertex.x < EPS ? 0 : pVer->vertex.x;
					double y = pVer->vertex.y < EPS ? 0 : pVer->vertex.y;
					fprintf(fp, "X%.1f Y%.1f\n", x, y);
				}
			}
			fprintf(fp, "END");//д�������ʾ
			fclose(fp);
			return true;
		}
		else
			return false;
	}
	else
	{
		int hits = 0;
		for (int i = 0; i < holeDList.GetSize(); i += 2)
		{
			if (i > 0 && holeDList.GetSize() > 2)
			{
				if (!bAutoSplitFile)
				{
					logerr.Log("%s�ְ��ϲ�ͬ�׾�����2�֣�", (char*)tempPlate.GetPartNo());
					break;
				}
			}
			int iStart = i;
			CString sPbjFilePath = file_path;
			if (holeDList.GetSize() > 2)
			{	//��Ҫ��ֵ�PBJ�ļ�����������ӿ׾� wht 19-06-18
				CXhChar50 sHoleStr;
				if (i + 1 < holeDList.GetSize())
					sHoleStr.Printf("%.1f,%.1f", holeDList[i + 1], holeDList[i]);
				else
					sHoleStr.Printf("%.1f", holeDList[i]);
				sPbjFilePath.Replace(".pbj", CXhChar16("-%s.pbj", (char*)sHoleStr));
			}
			//д���ļ�
			CDrillBolt* pDrillBolt = NULL;
			FILE* fp = fopen(sPbjFilePath, "wt");
			if (fp)
			{
				fprintf(fp, "P:%g\n", tempPlate.GetThick());      //д��P:���
				fprintf(fp, "T1:MK\n");                          //д��T1:MK
				//д����������ֱ��
				if (i + 1 < holeDList.GetSize())
				{
					fprintf(fp, "T2:%.1f\n", holeDList[i + 1]);
					fprintf(fp, "T3:%.1f\n", holeDList[i]);
					//T2��˨��
					pDrillBolt = hashDrillBoltByD.GetValue(ftoi(holeDList[i + 1] * 10));
					if (pDrillBolt)
					{
						fprintf(fp, "T2\n");
						for (BOLT_INFO* pBolt = pDrillBolt->boltList.GetFirst(); pBolt; pBolt = pDrillBolt->boltList.GetNext())
							fprintf(fp, "X%.1f Y%.1f\n", pBolt->posX, pBolt->posY);
					}
					//T3��˨��
					pDrillBolt = hashDrillBoltByD.GetValue(ftoi(holeDList[i] * 10));
					if (pDrillBolt)
					{
						fprintf(fp, "T3\n");
						for (BOLT_INFO* pBolt = pDrillBolt->boltList.GetFirst(); pBolt; pBolt = pDrillBolt->boltList.GetNext())
							fprintf(fp, "X%.1f Y%.1f\n", pBolt->posX, pBolt->posY);
					}
				}
				else
				{	//ֻ��һ�ֿ׾�ʱ
					pDrillBolt = hashDrillBoltByD.GetValue(ftoi(holeDList[i] * 10));
					if (pDrillBolt->biMode == CDrillBolt::DRILL_T2)
					{
						fprintf(fp, "T2:%.1f\n", holeDList[i]);
						fprintf(fp, "T3:\n");
						fprintf(fp, "T2\n");
					}
					else if (pDrillBolt->biMode == CDrillBolt::DRILL_T3)
					{
						fprintf(fp, "T2:\n");
						fprintf(fp, "T3:%.1f\n", holeDList[i]);
						fprintf(fp, "T3\n");
					}
					for (BOLT_INFO* pBolt = pDrillBolt->boltList.GetFirst(); pBolt; pBolt = pDrillBolt->boltList.GetNext())
						fprintf(fp, "X%.1f Y%.1f\n", pBolt->posX, pBolt->posY);
				}
				//
				if (tempPlate.IsDisplayMK())
				{
					fprintf(fp, "MK\n");		//д��MK
					GEPOINT mk_pt = tempPlate.mkpos;
					coord_trans(mk_pt, mcs, FALSE);
					fprintf(fp, "X%.1f Y%.1f\n", mk_pt.x, mk_pt.y);//д��MK����
				}
				fprintf(fp, "M30\n");
				if (bIncVertex)
				{
					for (PROFILE_VER* pVer = tempPlate.vertex_list.GetFirst(); pVer; pVer = tempPlate.vertex_list.GetNext())  //дplat����������
					{
						double x = pVer->vertex.x < EPS ? 0 : pVer->vertex.x;
						double y = pVer->vertex.y < EPS ? 0 : pVer->vertex.y;
						fprintf(fp, "X%.1f Y%.1f\n", x, y);
					}
				}
				fprintf(fp, "END");//д�������ʾ
				fclose(fp);
				hits++;
			}
			else
				continue;
		}
		return (hits > 0);
	}
}
void CNCPart::CreatePlatePbjFiles(CPPEModel *pModel,CXhPtrSet<CProcessPlate> &plateSet,const char* work_dir)
{
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_PBJ_FILE))
		return;
	CreatePlateFiles(pModel,plateSet,work_dir,PLATE_PBJ_FILE);
	/*CXhChar500 sFilePath,sWorkDir=GetValidWorkDir(work_dir,pModel);
	if(sWorkDir.Length()<=0)
		return;
	CFileFind fileFind;
	pModel->DisplayProcess(0,"����PBJ�ļ�����");
	int i=0,num=plateSet.GetNodeNum();
	for(CProcessPlate *pPlate=plateSet.GetFirst();pPlate;pPlate=plateSet.GetNext(),i++)
	{
		pModel->DisplayProcess(ftoi(100*i/num),"����PBJ�ļ�����");
		if(!IsNeedCreateHoleFile(pPlate,0))
			continue;
		CXhChar200 sFileName=GetValidFileName(pModel,pPlate);
		if(sFileName.Length()<=0)
			continue;
		CXhChar500 sPbjWorkDir("%s\\�崲�ӹ�-���",(char*)sWorkDir);
		if(!fileFind.FindFile(sPbjWorkDir))
			MakeDirectory(sPbjWorkDir);
		sFilePath.Printf("%s\\%s.pbj",(char*)sPbjWorkDir,(char*)sFileName);
		CreatePlatePbjFile(pPlate,sFilePath);
	}
	pModel->DisplayProcess(100,"����PBJ�ļ����");
	ShellExecute(NULL,"open",NULL,NULL,sWorkDir,SW_SHOW);*/
	//WinExec(CXhChar500("explorer.exe %s",(char*)sWorkDir),SW_SHOW);
}
extern char* SearchChar(char* srcStr,char ch,bool reverseOrder/*=false*/);
bool CNCPart::CreatePlatePmzFile(CProcessPlate *pPlate,const char* file_path)
{
	if(pPlate==NULL)
		return false;
	CXhChar100 sValue;
	int iPmzMode = 0;
	BOOL bIncVertex = FALSE;
	if (GetSysParaFromReg("PmzIncVertex", sValue))
		bIncVertex = atoi(sValue);
	if (GetSysParaFromReg("PmzMode", sValue))
		iPmzMode = atoi(sValue);
	CLogErrorLife logErrLife;
	CProcessPlate tempPlate;
	pPlate->ClonePart(&tempPlate);
	GECS mcs;
	pPlate->GetMCS(mcs);
	CProcessPlate::TransPlateToMCS(&tempPlate,mcs);
	CHashList<CDrillBolt> hashDrillBoltByD;
	if(m_bSortHole)
		OptimizeBolt(&tempPlate,hashDrillBoltByD);	//����˨����˳���Ż�
	else
	{	//���ݿ׾���ʼ����˨��Ϣ
		CDrillBolt* pDrillBolt=NULL;
		for(BOLT_INFO* pBolt=tempPlate.m_xBoltInfoList.GetFirst();pBolt;pBolt=tempPlate.m_xBoltInfoList.GetNext())
		{
			double fHoleD=pBolt->bolt_d+pBolt->hole_d_increment;
			int nKeyD=ftoi(fHoleD*10);
			pDrillBolt=hashDrillBoltByD.GetValue(nKeyD);
			if(pDrillBolt==NULL)
				pDrillBolt=hashDrillBoltByD.Add(nKeyD);
			BOLT_INFO newBolt;
			newBolt.CloneBolt(pBolt);
			pDrillBolt->fHoleD=fHoleD;
			pDrillBolt->boltList.append(newBolt);
		}
	}
	CXhChar500 sFile(file_path);
	double fSpecialD=(GetSysParaFromReg("LimitSH",sValue))?atof(sValue):0;
	int bNeedSH=(GetSysParaFromReg("NeedSH",sValue))?atoi(sValue):0;
	char* szExt=SearchChar(sFile,'.',true);
	if(szExt)	//�����к��о��
		*szExt=0;
	if (iPmzMode == 1)
	{	//һ�ֿ׾���Ӧһ��pmz�ļ�
		for (CDrillBolt* pDrillBolt = hashDrillBoltByD.GetFirst(); pDrillBolt; pDrillBolt = hashDrillBoltByD.GetNext())
		{
			if (!bNeedSH&&fSpecialD > 0 && pDrillBolt->fHoleD >= fSpecialD)
				continue;
			CXhChar100 sFilePath("%s-D%s.pmz", (char*)sFile, (char*)CXhChar16(pDrillBolt->fHoleD));
			FILE* fp = fopen(sFilePath, "wt");
			if (fp == NULL)
				continue;
			fprintf(fp, "P: %g\n", tempPlate.GetThick());      //д��P:���
			fprintf(fp, "L: %d\n", tempPlate.GetLength());     //д��L:����
			fprintf(fp, "W: %g\n", tempPlate.GetWidth());      //д��W:���
			fprintf(fp, "D%g\n", pDrillBolt->fHoleD);
			for (BOLT_INFO* pBolt = pDrillBolt->boltList.GetFirst(); pBolt; pBolt = pDrillBolt->boltList.GetNext())
				fprintf(fp, "X%.1f Y%.1f\n", pBolt->posX, pBolt->posY);
				//fprintf(fp, "X%g Y%g\n", pBolt->posX, pBolt->posY);
			fprintf(fp, "M30\n");
			//��ʱ����Ҫ��ʾ����������
			if (bIncVertex)
			{
				for (PROFILE_VER* pVer = tempPlate.vertex_list.GetFirst(); pVer; pVer = tempPlate.vertex_list.GetNext())  //дplat����������
					fprintf(fp,"X%.1f Y%.1f\n",pVer->vertex.x,pVer->vertex.y);
			}
			fprintf(fp, "END");//д�������ʾ
			fclose(fp);
		}
	}
	else
	{
		CXhChar100 sFilePath("%s.pmz", (char*)sFile);
		FILE* fp = fopen(sFilePath, "wt");
		if (fp == NULL)
			return false;
		fprintf(fp, "P: %g\n", tempPlate.GetThick());      //д��P:���
		fprintf(fp, "L: %d\n", tempPlate.GetLength());     //д��L:����
		fprintf(fp, "W: %g\n", tempPlate.GetWidth());      //д��W:���
		for (CDrillBolt* pDrillBolt = hashDrillBoltByD.GetFirst(); pDrillBolt; pDrillBolt = hashDrillBoltByD.GetNext())
		{
			if (!bNeedSH&&fSpecialD > 0 && pDrillBolt->fHoleD >= fSpecialD)
				continue;
			fprintf(fp, "D%.1f\n", pDrillBolt->fHoleD);
			for (BOLT_INFO* pBolt = pDrillBolt->boltList.GetFirst(); pBolt; pBolt = pDrillBolt->boltList.GetNext())
				fprintf(fp, "  X%.1f     Y%.1f\n", pBolt->posX, pBolt->posY);
				//fprintf(fp, "  X%g     Y%g\n", pBolt->posX, pBolt->posY);
		}
		fprintf(fp, "M30\n");
		//����������
		if (bIncVertex)
		{
			for (PROFILE_VER* pVer = tempPlate.vertex_list.GetFirst(); pVer; pVer = tempPlate.vertex_list.GetNext())  //дplat����������
				fprintf(fp, "  X%.1f     Y%.1f\n", pVer->vertex.x, pVer->vertex.y);
		}
		fprintf(fp, "END");//д�������ʾ
		fclose(fp);
	}
	return true;
}
bool CNCPart::CreatePlatePmzCheckFile(CProcessPlate *pPlate, const char* file_path)
{
	if (pPlate == NULL)
		return false;
	CXhChar100 sValue;
	BOOL bIncVertex = FALSE;
	if (GetSysParaFromReg("PmzIncVertex", sValue))
		bIncVertex = atoi(sValue);
	CLogErrorLife logErrLife;
	CProcessPlate tempPlate;
	pPlate->ClonePart(&tempPlate);
	GECS mcs;
	pPlate->GetMCS(mcs);
	CProcessPlate::TransPlateToMCS(&tempPlate, mcs);
	CHashList<CDrillBolt> hashDrillBoltByD;
	CXhChar500 sFile(file_path);
	double fSpecialD = (GetSysParaFromReg("LimitSH", sValue)) ? atof(sValue) : 0;
	int bNeedSH = (GetSysParaFromReg("NeedSH", sValue)) ? atoi(sValue) : 0;
	char* szExt = SearchChar(sFile, '.', true);
	if (szExt)	//�����к��о��
		*szExt = 0;
	//���Ԥ��pmz�ļ�
	CXhChar500 sFilePath("%s.pmz", (char*)sFile);
	FILE* fp = fopen(sFilePath, "wt");
	if (fp == NULL)
		return false;
	fprintf(fp, "P: %g\n", tempPlate.GetThick());      //д��P:���
	fprintf(fp, "L: %d\n", tempPlate.GetLength());     //д��L:����
	fprintf(fp, "W: %g\n", tempPlate.GetWidth());      //д��W:���
	if (tempPlate.m_xBoltInfoList.Count < 4)
	{	//��˨����С��4ʱ��ֱ��������п�
		for (CDrillBolt* pDrillBolt = hashDrillBoltByD.GetFirst(); pDrillBolt; pDrillBolt = hashDrillBoltByD.GetNext())
		{
			//if (!bNeedSH&&fSpecialD > 0 && pDrillBolt->fHoleD >= fSpecialD)
			//	continue;
			fprintf(fp, "D%.1f\n", pDrillBolt->fHoleD);
			for (BOLT_INFO* pBolt = pDrillBolt->boltList.GetFirst(); pBolt; pBolt = pDrillBolt->boltList.GetNext())
				fprintf(fp, "  X%.1f     Y%.1f\n", pBolt->posX, pBolt->posY);
		}
	}
	else
	{	//��ʼ��Ԥ�����ף�������˨���������Σ������ĸ����������̵��Ŀ���˨�����ܴ����ص������
		hashDrillBoltByD.Empty();
		CDrillBolt *pDrillBolt = hashDrillBoltByD.Add(0);
		SCOPE_STRU scope;
		//1.������˨����������
		CMaxDouble maxX,maxY;
		CMaxDouble minX,minY;
		for (BOLT_INFO* pBolt = tempPlate.m_xBoltInfoList.GetFirst(); pBolt; pBolt = tempPlate.m_xBoltInfoList.GetNext())
		{
			scope.VerifyVertex(GEPOINT(pBolt->posX, pBolt->posY));
			minX.Update(pBolt->posX, pBolt);
			minY.Update(pBolt->posY, pBolt);
			maxX.Update(pBolt->posX, pBolt);
			maxY.Update(pBolt->posY, pBolt);
		}
		BOOL bRetCode = FALSE;
		if (scope.wide() > 0 && scope.high() > 0)
		{
			CMinDouble minDistArr[4];
			GEPOINT cornerPtArr[4] = { GEPOINT(scope.fMinX, scope.fMaxY), GEPOINT(scope.fMinX, scope.fMinY),
									   GEPOINT(scope.fMaxX, scope.fMaxY), GEPOINT(scope.fMaxX, scope.fMinY) };
			for (BOLT_INFO* pBolt = tempPlate.m_xBoltInfoList.GetFirst(); pBolt; pBolt = tempPlate.m_xBoltInfoList.GetNext())
			{
				GEPOINT boltPos(pBolt->posX, pBolt->posY);
				for(int i=0;i<4;i++)
					minDistArr[i].Update(DISTANCE(boltPos,cornerPtArr[i]), pBolt);
			}
			CMinDouble minD;
			CHashSet<BOLT_INFO*> hashBoltSet;
			for (int i = 0; i < 4; i++)
			{
				BOLT_INFO *pTempBolt = minDistArr[i].IsInited() ? (BOLT_INFO*)minDistArr[i].m_pRelaObj : NULL;
				if (pTempBolt == NULL || hashBoltSet.GetValue((DWORD)pTempBolt) != NULL)
					continue;
				hashBoltSet.SetValue((DWORD)pTempBolt, pTempBolt);
				minD.Update(pTempBolt->bolt_d + pTempBolt->hole_d_increment, pTempBolt);
				BOLT_INFO newBolt;
				newBolt.CloneBolt(pTempBolt);
				pDrillBolt->boltList.append(newBolt);
			}
			if (hashBoltSet.GetNodeNum() >= 2&&minD.IsInited())
			{
				pDrillBolt->fHoleD = minD.number;
				bRetCode = TRUE;
			}
			else
			{
				pDrillBolt->boltList.Empty();
				bRetCode = FALSE;
			}
		}
		else if (scope.wide() > 0)
		{	//
			BOLT_INFO *pMinXBolt = minX.IsInited() ? (BOLT_INFO*)minX.m_pRelaObj : NULL;
			BOLT_INFO *pMaxXBolt = maxX.IsInited() ? (BOLT_INFO*)maxX.m_pRelaObj : NULL;
			if (pMinXBolt&&pMaxXBolt&&pMinXBolt != pMaxXBolt)
			{
				BOLT_INFO newBolt;
				newBolt.CloneBolt(pMinXBolt);
				pDrillBolt->boltList.append(newBolt);
				newBolt.CloneBolt(pMaxXBolt);
				pDrillBolt->boltList.append(newBolt);
				pDrillBolt->fHoleD = min(pMinXBolt->hole_d_increment + pMinXBolt->bolt_d, pMaxXBolt->hole_d_increment + pMaxXBolt->bolt_d);
				bRetCode = TRUE;
			}
		}
		else if (scope.high() > 0)
		{
			BOLT_INFO *pMinYBolt = minY.IsInited() ? (BOLT_INFO*)minY.m_pRelaObj : NULL;
			BOLT_INFO *pMaxYBolt = maxY.IsInited() ? (BOLT_INFO*)maxY.m_pRelaObj : NULL;
			if (pMinYBolt&&pMaxYBolt&&pMinYBolt != pMaxYBolt)
			{
				BOLT_INFO newBolt;
				newBolt.CloneBolt(pMinYBolt);
				pDrillBolt->boltList.append(newBolt);
				newBolt.CloneBolt(pMaxYBolt);
				pDrillBolt->boltList.append(newBolt);
				pDrillBolt->fHoleD = min(pMinYBolt->hole_d_increment + pMinYBolt->bolt_d, pMaxYBolt->hole_d_increment + pMaxYBolt->bolt_d);
				bRetCode = TRUE;
			}
		}
		if(!bRetCode) //�쳣��������������˨
		{
			pDrillBolt->boltList.Empty();
			CMinDouble minD;
			for (BOLT_INFO* pBolt = tempPlate.m_xBoltInfoList.GetFirst(); pBolt; pBolt = tempPlate.m_xBoltInfoList.GetNext())
			{
				BOLT_INFO newBolt;
				newBolt.CloneBolt(pBolt);
				pDrillBolt->boltList.append(newBolt);
				double fHoleD = pBolt->bolt_d + pBolt->hole_d_increment;
				minD.Update(fHoleD);
			}
			pDrillBolt->fHoleD = minD.number;
		}
		//2.��˨����
		//OptimizeBolt(&tempPlate, hashDrillBoltByD, FALSE);	//����˨����˳���Ż�
		f3dPoint startPos;
		pDrillBolt->OptimizeBoltOrder(startPos);
		//3.�����˨����
		for (CDrillBolt* pDrillBolt = hashDrillBoltByD.GetFirst(); pDrillBolt; pDrillBolt = hashDrillBoltByD.GetNext())
		{
			//if (!bNeedSH&&fSpecialD > 0 && pDrillBolt->fHoleD >= fSpecialD)
			//	continue;
			fprintf(fp, "D%.1f\n", pDrillBolt->fHoleD);
			for (BOLT_INFO* pBolt = pDrillBolt->boltList.GetFirst(); pBolt; pBolt = pDrillBolt->boltList.GetNext())
				fprintf(fp, "  X%.1f     Y%.1f\n", pBolt->posX, pBolt->posY);
		}
	}
	fprintf(fp, "M30\n");
	//����������
	if (bIncVertex)
	{
		for (PROFILE_VER* pVer = tempPlate.vertex_list.GetFirst(); pVer; pVer = tempPlate.vertex_list.GetNext())  //дplat����������
			fprintf(fp, "  X%.1f     Y%.1f\n", pVer->vertex.x, pVer->vertex.y);
	}
	fprintf(fp, "END");//д�������ʾ
	fclose(fp);
	return true;
}
void CNCPart::CreatePlatePmzFiles(CPPEModel *pModel,CXhPtrSet<CProcessPlate> &plateSet,const char* work_dir)
{
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_PBJ_FILE))
		return;
	CXhChar500 sFilePath,sWorkDir=GetValidWorkDir(work_dir,pModel);
	if(sWorkDir.Length()<=0)
		return;
	CXhChar100 sValue;
	BOOL bPmzCheck = FALSE;
	if (GetSysParaFromReg("PmzCheck", sValue))
		bPmzCheck = atoi(sValue);
	CFileFind fileFind;
	pModel->DisplayProcess(0,"����PMZ�ļ�����");
	int i=0,num=plateSet.GetNodeNum();
	for(CProcessPlate *pPlate=plateSet.GetFirst();pPlate;pPlate=plateSet.GetNext(),i++)
	{
		pModel->DisplayProcess(ftoi(100*i/num),"����PMZ�ļ�����");
		if(!IsNeedCreateHoleFile(pPlate,1))
			continue;
		CXhChar200 sFileName=GetValidFileName(pModel,pPlate);
		if(sFileName.Length()<=0)
			continue;
		CXhChar500 sPmzWorkDir("%s\\�崲�ӹ�-PMZ",(char*)sWorkDir);
		if(!fileFind.FindFile(sPmzWorkDir))
			MakeDirectory(sPmzWorkDir);
		sFilePath.Printf("%s\\%s.pmz",(char*)sPmzWorkDir,(char*)sFileName);
		CreatePlatePmzFile(pPlate,sFilePath);
		if (bPmzCheck)
		{	//����PMZԤ���ļ� wht 19-07-02
			sPmzWorkDir.Printf("%s\\�崲�ӹ�-PMZԤ��", (char*)sWorkDir);
			if (!fileFind.FindFile(sPmzWorkDir))
				MakeDirectory(sPmzWorkDir);
			sFilePath.Printf("%s\\%s.pmz", (char*)sPmzWorkDir, (char*)sFileName);
			CreatePlatePmzCheckFile(pPlate, sFilePath);
		}
	}
	pModel->DisplayProcess(100,"����PMZ�ļ����");
	ShellExecute(NULL,"open",NULL,NULL,sWorkDir,SW_SHOW);
	//WinExec(CXhChar500("explorer.exe %s",(char*)sWorkDir),SW_SHOW);
}
bool CNCPart::CreatePlateTxtFile(CProcessPlate *pPlate,const char* file_path)
{
	BOOL bCutSpecialHole = FALSE;
	int nInLineLen=-1,nOutLineLen=-1;
	if(CPPEModel::sysPara!=NULL)
	{
		nInLineLen=(int)CPPEModel::sysPara->GetCutInLineLen(pPlate->m_fThick,ISysPara::TYPE_PLASMA_CUT);
		nOutLineLen=(int)CPPEModel::sysPara->GetCutOutLineLen(pPlate->m_fThick,ISysPara::TYPE_PLASMA_CUT);
		bCutSpecialHole = CPPEModel::sysPara->IsCutSpecialHole(ISysPara::TYPE_PLASMA_CUT);
	}
	CNCPlate ncPlate(pPlate,GEPOINT(0,0,0),0,0,false,nInLineLen,nOutLineLen,0,0,0,bCutSpecialHole);
	return ncPlate.CreatePlateTxtFile(file_path);
}

void CNCPart::CreatePlateTxtFiles(CPPEModel *pModel,CXhPtrSet<CProcessPlate> &plateSet,const char* work_dir)
{
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
		return;
	CreatePlateFiles(pModel,plateSet,work_dir,PLATE_TXT_FILE);
}

bool CNCPart::CreatePlateNcFile(CProcessPlate *pPlate,const char* file_path)
{	
	int nInLineLen=-1,nOutLineLen=-1;
	if(CPPEModel::sysPara!=NULL)
	{
		nInLineLen=(int)CPPEModel::sysPara->GetCutInLineLen(pPlate->m_fThick,ISysPara::TYPE_FLAME_CUT);
		nOutLineLen=(int)CPPEModel::sysPara->GetCutOutLineLen(pPlate->m_fThick,ISysPara::TYPE_FLAME_CUT);
	}
	CNCPlate ncPlate(pPlate,GEPOINT(),0,1,true,nInLineLen,nOutLineLen,5,2);
	return ncPlate.CreatePlateNcFile(file_path);
}

void CNCPart::CreatePlateNcFiles(CPPEModel *pModel,CXhPtrSet<CProcessPlate> &plateSet,const char* work_dir)
{
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
		return;
	CreatePlateFiles(pModel,plateSet,work_dir,PLATE_NC_FILE);
}

bool CNCPart::CreatePlateCncFile(CProcessPlate *pPlate,const char* file_path)
{	
	int nInLineLen=-1,nOutLineLen=-1,nEnlargedSpace=0;
	if(CPPEModel::sysPara!=NULL)
	{
		nInLineLen=(int)CPPEModel::sysPara->GetCutInLineLen(pPlate->m_fThick,ISysPara::TYPE_FLAME_CUT);
		nOutLineLen=(int)CPPEModel::sysPara->GetCutOutLineLen(pPlate->m_fThick,ISysPara::TYPE_FLAME_CUT);
		nEnlargedSpace=CPPEModel::sysPara->GetCutEnlargedSpaceLen(ISysPara::TYPE_FLAME_CUT);
	}
	CNCPlate ncPlate(pPlate,GEPOINT(0,0,0),0,0,false,nInLineLen,nOutLineLen,0,0,nEnlargedSpace);
	return ncPlate.CreatePlateTxtFile(file_path);
}

void CNCPart::CreatePlateCncFiles(CPPEModel *pModel,CXhPtrSet<CProcessPlate> &plateSet,const char* work_dir)
{
	if(!VerifyValidFunction(PNC_LICFUNC::FUNC_IDENTITY_CUT_FILE))
		return;
	CreatePlateFiles(pModel,plateSet,work_dir,PLATE_CNC_FILE);
}
void CNCPart::CreatePlateFiles(CPPEModel *pModel,CXhPtrSet<CProcessPlate> &plateSet,const char* work_dir,int nFileType)
{	//PNC�û�֧�ֶ���ģʽ�µ�DXF�ļ�
	int iDxfMode=0;
	CXhChar16 sExtName,sFolderName;
	CXhChar100 sTitle("����"),sValue;
	if(GetSysParaFromReg("DxfMode",sValue))
		iDxfMode=atoi(sValue);
	if(nFileType==PLATE_NC_FILE)
	{
		sTitle.Copy("�����ְ�NC�ļ�����");	
		sExtName.Copy("nc");
		sFolderName.Copy("�������и�");
	}
	else if(nFileType==PLATE_CNC_FILE)
	{
		sTitle.Copy("�����ְ�CNC�ļ�����");	
		sExtName.Copy("cnc");
		sFolderName.Copy("�����и�CNC");
	}
	else if(nFileType==PLATE_TXT_FILE)
	{
		sTitle.Copy("�����ְ�TXT�ļ�����");	
		sExtName.Copy("txt");
		sFolderName.Copy("�����и�TXT");
	}
	else if(nFileType==PLATE_PBJ_FILE)
	{
		sTitle.Copy("�����ְ�PBJ�ļ�����");	
		sExtName.Copy("pbj");
		sFolderName.Copy("�崲�ӹ�-���");
		if(GetSysParaFromReg("PbjMode",sValue))
			iDxfMode=atoi(sValue);
		else
			iDxfMode=0;
	}
	else
		return;
	CXhChar500 sFilePath,sWorkDir=GetValidWorkDir(work_dir,pModel);
	if(sWorkDir.Length()<=0)
		return;
	CLogErrorLife logErrLife;
	CHashList<PLATE_GROUP> hashPlateByThick;
	for(CProcessPart *pPart=plateSet.GetFirst();pPart;pPart=plateSet.GetNext())
	{
		int thick=(int)(pPart->GetThick());
		PLATE_GROUP* pPlateGroup=hashPlateByThick.GetValue(thick);
		if(pPlateGroup==NULL)
		{
			pPlateGroup=hashPlateByThick.Add(thick);
			pPlateGroup->thick=thick;
		}
		pPlateGroup->plateSet.append((CProcessPlate*)pPart);
	}
	CFileFind fileFind;
	CXhChar100 sFileName;
	pModel->DisplayProcess(0,sTitle);
	CXhChar500 sFileWorkDir("%s\\%s",(char*)sWorkDir,(char*)sFolderName);
	int index=0,num=pModel->PartCount();
	for(PLATE_GROUP *pPlateGroup=hashPlateByThick.GetFirst();pPlateGroup;pPlateGroup=hashPlateByThick.GetNext())
	{
		int thick=pPlateGroup->thick;
		for(CProcessPlate *pPlate=pPlateGroup->plateSet.GetFirst();pPlate;pPlate=pPlateGroup->plateSet.GetNext())
		{
			index++;
			sFileName=GetValidFileName(pModel,pPlate);
			if(sFileName.Length()<=0)
				continue;
			if(iDxfMode==1)
			{	//����ȴ����ļ�Ŀ¼
				sFilePath.Printf("%s\\���-%d-",(char*)sFileWorkDir,thick);
				//CXhChar16 sMat;
				//QuerySteelMatMark(pPlate->cMaterial, sMat);
				//sFilePath.Append(sMat);
				if (pPlate->cMaterial == 'S')
					sFilePath.Append("Q235");
				else if (pPlate->cMaterial == 'H')
					sFilePath.Append("Q345");
				else if (pPlate->cMaterial == 'h')
					sFilePath.Append("Q355");
				else if (pPlate->cMaterial == 'P')
					sFilePath.Append("Q420");
				else if (pPlate->cMaterial == 'T')
					sFilePath.Append("Q460");
			}
			else
				sFilePath.Printf("%s",(char*)sFileWorkDir);
			if(!fileFind.FindFile(sFilePath))
				MakeDirectory(sFilePath);
			sFilePath.Printf("%s\\%s.%s",(char*)sFilePath,(char*)sFileName,(char*)sExtName);
			if(nFileType==PLATE_NC_FILE)
				CreatePlateNcFile(pPlate,sFilePath);
			else if(nFileType==PLATE_CNC_FILE)
				CreatePlateCncFile(pPlate,sFilePath);
			else if(nFileType==PLATE_TXT_FILE)
				CreatePlateTxtFile(pPlate,sFilePath);
			else if(nFileType==PLATE_PBJ_FILE)
				CreatePlatePbjFile(pPlate,sFilePath);
		}
		pModel->DisplayProcess(ftoi(100*index/num),sTitle);
	}
	pModel->DisplayProcess(100,sTitle);
	ShellExecute(NULL,"open",NULL,NULL,sFileWorkDir,SW_SHOW);
	//WinExec(CXhChar500("explorer.exe %s",(char*)sWorkDir),SW_SHOW);
}
void CNCPart::ImprotPlateCncOrTextFile(CProcessPlate *pPlate,const char* file_path)
{
	char drive[4];
	char dir[MAX_PATH];
	CXhChar50 file_name;
	CXhChar16 extension;
	_splitpath(file_path,drive,dir,file_name,extension);
	pPlate->SetPartNo(CXhChar100("%s%s",(char*)file_name,(char*)extension));
	CNCPlate::InitVertextListByNcFile(pPlate,file_path);
}
#endif

void CNCPart::CreateAllPlateFiles(int nFileType)
{
	CLogErrorLife logErrLife;
	CXhPtrSet<CProcessPlate> plateSet;
	for(CProcessPart *pPart=model.EnumPartFirst();pPart;pPart=model.EnumPartNext())
	{
		if(!pPart->IsPlate())
			continue;
		plateSet.append((CProcessPlate*)pPart);
	}
#ifdef __PNC_
	if(nFileType==CNCPart::PLATE_TXT_FILE)
		CreatePlateTxtFiles(&model,plateSet,model.GetFolderPath());
	else if(nFileType==CNCPart::PLATE_PBJ_FILE)
		CreatePlatePbjFiles(&model,plateSet,model.GetFolderPath());
	else if(nFileType==CNCPart::PLATE_NC_FILE)
		CreatePlateNcFiles(&model,plateSet,model.GetFolderPath());
	else if(nFileType==CNCPart::PLATE_CNC_FILE)
		CreatePlateCncFiles(&model,plateSet,model.GetFolderPath());
	else if(nFileType==CNCPart::PLATE_DXF_FILE)
		CreatePlatePncDxfFiles(&model,plateSet,model.GetFolderPath());
	else if(nFileType==CNCPart::PLATE_PMZ_FILE)
		CreatePlatePmzFiles(&model,plateSet,model.GetFolderPath());
#else
	if(nFileType==CNCPart::PLATE_DXF_FILE)
		CreatePlateDxfFiles(&model,plateSet,model.GetFolderPath());
#endif
	if(nFileType==CNCPart::PLATE_TTP_FILE)
		CreatePlateTtpFiles(&model,plateSet,model.GetFolderPath());
	else if (nFileType == CNCPart::PLATE_WKF_FILE)
		CreatePlateWkfFiles(&model, plateSet, model.GetFolderPath());
}
//�Ǹֲ���
void CNCPart::CreateAngleNcFiles(CPPEModel *pModel,CXhPtrSet<CProcessAngle> &angleSet,
								 const char* drv_path,const char* sPartNoPrefix,const char* work_dir)
{
	CXhChar500 sFilePath,sWorkDir=GetValidWorkDir(work_dir,pModel);
	if(sWorkDir.Length()<=0)
		return;
	CNcJg NcJg;
	if(!NcJg.NcManager.InitJgNcDriver(drv_path))
		return;	//��ʼ��ʧ��

	CProcessAngle *pAngle=NULL;
	char fpath[MAX_PATH],bak_fpath[MAX_PATH];
	strcpy(fpath,sWorkDir);
	strcpy(bak_fpath,fpath);
	FILE *dir_fp=NULL;
	for(CNCDirectoryFile *pDirFile=NcJg.NcManager.directory.GetFirst();pDirFile;pDirFile=NcJg.NcManager.directory.GetNext())
	{	//��Ա�����ˮ��ѹ������������,�������ļ��⻹��һ��Ŀ¼�ļ�,�˴������������ɴ���Ŀ¼�ļ�
		if(pDirFile->contents.GetNodeNum()>0)
		{	//��Ҫ����Ŀ¼
			strcat(fpath,pDirFile->DIR_FILE_NAME);
			if(pDirFile->m_bDirectoryPrintByASCII)
				dir_fp=fopen(fpath,"wt");
			else
				dir_fp=fopen(fpath,"wb");
			if(dir_fp)
			{
				BOOL bak_format=NcJg.NcManager.m_bPrintByASCII;
				NcJg.NcManager.m_bPrintByASCII = pDirFile->m_bDirectoryPrintByASCII;
				if(sPartNoPrefix)
					strncpy(NcJg.sPrefix,sPartNoPrefix,19);
				for(pAngle=angleSet.GetFirst();pAngle;pAngle=angleSet.GetNext())
				{
					NcJg.InitProcessAngle(pAngle);
					for(CVariant *pVar=pDirFile->contents.GetFirst();pVar;pVar=pDirFile->contents.GetNext())
						NcJg.PrintLn(dir_fp,pVar->sVal);
				}
				NcJg.NcManager.m_bPrintByASCII = bak_format;
				fclose(dir_fp);
			}
		}
	}
	for(pAngle=angleSet.GetFirst();pAngle;pAngle=angleSet.GetNext())
	{
		strcpy(fpath,bak_fpath);
		if(strlen(sPartNoPrefix)>0)
		{
			strcat(fpath,sPartNoPrefix);
			strcat(fpath,"-");
		}
		strcat(fpath,pAngle->GetPartNo());
		NcJg.InitProcessAngle(pAngle);
		NcJg.GenNCFile(fpath,sPartNoPrefix);
	}
	ShellExecute(NULL,"open",NULL,NULL,sWorkDir,SW_SHOW);
	//WinExec(CXhChar500("explorer.exe %s",(char*)sWorkDir),SW_SHOW);
}
void CNCPart::CreateAllAngleNcFile(CPPEModel *pModel,const char* drv_path,const char* sPartNoPrefix,const char* work_dir)
{
	CXhPtrSet<CProcessAngle> angleSet;
	for(CProcessPart *pPart=pModel->EnumPartFirst();pPart;pPart=pModel->EnumPartNext())
	{
		if(pPart->IsAngle())
			angleSet.append((CProcessAngle*)pPart);
	}
	CreateAngleNcFiles(pModel,angleSet,drv_path,sPartNoPrefix,work_dir);
}
//����PPI�ļ�
bool CNCPart::CreatePPIFile(CProcessPart *pPart,const char* file_path)
{
	FILE *fp=fopen(file_path,"wb");
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
void CNCPart::CreatePPIFiles(CPPEModel *pModel,CXhPtrSet<CProcessPart> &partSet,const char* work_dir)
{
	CXhChar500 sFilePath,sWorkDir=GetValidWorkDir(work_dir,pModel);
	if(sWorkDir.Length()<=0)
		return;
	CXhChar200 sFileName;
	for(CProcessPart *pPart=partSet.GetFirst();pPart;pPart=partSet.GetNext())
	{
		sFileName=GetValidFileName(pModel,pPart);
		if(sFileName.Length()<=0)
			continue;
		sFilePath.Printf("%s\\%s.ppi",(char*)sWorkDir,(char*)sFileName);
		CreatePlateTtpFile((CProcessPlate*)pPart,sFilePath);
	}
	ShellExecute(NULL,"open",NULL,NULL,sWorkDir,SW_SHOW);
	//WinExec(CXhChar500("explorer.exe %s",(char*)sWorkDir),SW_SHOW);
}
void CNCPart::CreateAllPPIFiles(CPPEModel *pModel,const char* work_dir)
{
	CXhPtrSet<CProcessPart> partSet;
	for(CProcessPart *pPart=pModel->EnumPartFirst();pPart;pPart=pModel->EnumPartNext())
		partSet.append(pPart);
	CreatePPIFiles(pModel,partSet,work_dir);
}
//////////////////////////////////////////////////////////////////////////
//CDrillBolt
#include "TSPAlgorithm.h"
struct BOLT_SORT_ITEM {
	double m_fDist;
	BOLT_INFO* m_pBolt;
};
int compare_func(const BOLT_SORT_ITEM& item1, const BOLT_SORT_ITEM& item2)
{
	return compare_double(item1.m_fDist, item2.m_fDist);
}
void CDrillBolt::OptimizeBoltOrder(f3dPoint& startPos, BYTE ciAlgType /*= 0*/)
{
	if (boltList.GetNodeNum() <= 1)
		return;
	//������˨��startPos�ľ���������򣬲��������˨��Ϊ��ʼ��˨
	ARRAY_LIST<BOLT_SORT_ITEM> boltItemArr;
	boltItemArr.SetSize(0, boltList.GetNodeNum());
	BOLT_INFO* pBolt = NULL, *pNewBolt = NULL;
	for (pBolt = boltList.GetFirst(); pBolt; pBolt = boltList.GetNext())
	{
		BOLT_SORT_ITEM* pItem = boltItemArr.append();
		pItem->m_fDist = DISTANCE(startPos, f3dPoint(pBolt->posX, pBolt->posY, 0));
		pItem->m_pBolt = pBolt;
	}
	CQuickSort<BOLT_SORT_ITEM>::QuickSort(boltItemArr.m_pData, boltItemArr.GetSize(), compare_func);
	//��ʼ����˨�����б����˨����б�ʹ��һһ��Ӧ
	ARRAY_LIST<f3dPoint> ptArr;
	ptArr.SetSize(boltItemArr.GetSize()+1);
	ptArr[0] = startPos;
	CHashListEx<BOLT_INFO> hashBoltList;
	for(int i=0;i<boltItemArr.GetSize();i++)
	{
		pNewBolt=hashBoltList.Add(0);
		pNewBolt->CloneBolt(boltItemArr[i].m_pBolt);
		ptArr[i+1].Set(pNewBolt->posX, pNewBolt->posY, 0);
	}
	//�����Ż����򣬲����ǹ��ɻ�
	int n[300] = { 0 };
	CTSPAlgorithm xOptimize;
	xOptimize.InitData(ptArr,ciAlgType);
	xOptimize.CalBestPath(1,n);
	//����д����˨
	boltList.Empty();
	for(int i=1;i<=(int)hashBoltList.GetNodeNum();i++)
	{
		pBolt=hashBoltList.GetValue(n[i]);
		if(pBolt==NULL)
			continue;
		pNewBolt=boltList.append();
		pNewBolt->CloneBolt(pBolt);
	}
	pBolt=boltList.GetTail();
	startPos.Set(pBolt->posX,pBolt->posY);
}