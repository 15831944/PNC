#pragma once
#include "HashTable.h"
#include "XhCharString.h"
#include "ProcessPart.h"
#include <vector>

using std::vector;
struct ISysPara
{
	const static BYTE TYPE_FLAME_CUT	= 0;	//火焰切割
	const static BYTE TYPE_PLASMA_CUT	= 1;	//等离子切割
	virtual double GetCutInLineLen(double fThick,BYTE cType=-1)=0;
	virtual double GetCutOutLineLen(double fThick,BYTE cType=-1)=0;
	virtual int GetCutEnlargedSpaceLen(BYTE cType=-1)=0;
	virtual int GetCutInitPosFarOrg(BYTE cType=-1)=0;
	virtual int GetCutPosInInitPos(BYTE cType=-1)=0;
	virtual BOOL IsCutSpecialHole(BYTE cType=-1)=0;
};

class CPPEModel
{
private:
	CXhChar500 m_sFolderPath;	//文件夹路径
	CSuperHashStrList<CProcessPart> m_hashPartByPartNo;
	CHashStrList<CXhChar500> m_hashFilePathByPartNo;
public:
	CXhChar50 m_sVersion;		//版本号
	CXhChar100 m_sCompanyName;	//设计单位
	CXhChar100 m_sPrjCode;		//工程编号
	CXhChar100 m_sPrjName;		//工程名称
	CXhChar50 m_sTaType;		//塔型
	CXhChar50 m_sTaAlias;		//代号
	CXhChar50 m_sTaStampNo;		//钢印号
	CXhChar16 m_sOperator;		//操作员（制表人）
	CXhChar16 m_sAuditor;		//审核人
	CXhChar16 m_sCritic;		//评审人
	//文件输出格式定制
	static const CXhChar16 KEY_TA_TYPE;
	static const CXhChar16 KEY_PART_NO;
	static const CXhChar16 KEY_PART_MAT;
	static const CXhChar16 KEY_PART_THICK;
	static const CXhChar16 KEY_SINGLE_NUM;
	static const CXhChar16 KEY_PROCESS_NUM;
	struct FILE_FORMAT 
	{
		vector<CXhChar16> m_sSplitters;
		vector<CXhChar16> m_sKeyMarkArr;
		//
		FILE_FORMAT() { ; }
		CXhChar100 GetFileFormatStr(){
			CXhChar100 sText;
			size_t nSplit = m_sSplitters.size();
			size_t nKeySize = m_sKeyMarkArr.size();
			for (size_t i = 0; i < nKeySize; i++)
			{
				if (sText.GetLength()>0 && nSplit >= i&& m_sSplitters[i - 1].GetLength() > 0)
					sText.Append(m_sSplitters[i - 1]);
				sText.Append(m_sKeyMarkArr[i]);
			}
			//文件格式末尾带特殊分隔符
			if(nKeySize >0 && nSplit == nKeySize && m_sSplitters[nSplit - 1].GetLength() > 0)
				sText.Append(m_sSplitters[nSplit - 1]);
			return sText;
		}
		bool IsValidFormat() { return m_sKeyMarkArr.size() > 0; }
	}file_format;
public:
	static ILog2File* log2file;
	static ILog2File* Log2File();//永远会返回一个有效指针,但只有log2file!=NULL时,才会真正记录错误日志
	static ISysPara* sysPara;
public:
	CPPEModel(void);
	~CPPEModel(void);
	void (*DisplayProcess)(int percent,char *sTitle);	//进度显示回调函数
	BOOL FromBuffer(CBuffer &buffer);
	void ToBuffer(CBuffer &buffer,const char *file_version);
	BOOL InitModelByFolderPath(const char *folder_path);
	void InitPlateMcsAndCutPt(bool bSaveToFile=false);	//初始化钢板加工坐标系及切入点
	void Empty();
	CXhChar500 GetFolderPath(){return m_sFolderPath;}
	//
	CXhChar500 GetPartFilePath(const char *sPartNo);
	bool SavePartToFile(CProcessPart *pPart);
	CProcessPart* AddPart(const char *sPartNo,BYTE cType,const char *sFilePath=NULL);
	BOOL DeletePart(const char *sPartNo);
	CProcessPart* FromPartNo(const char *sPartNo){return m_hashPartByPartNo.GetValue(sPartNo);}
	CProcessPart* EnumPartFirst(){return m_hashPartByPartNo.GetFirst();}
	CProcessPart* EnumPartNext(){return m_hashPartByPartNo.GetNext();}
	DWORD PartCount(){return m_hashPartByPartNo.GetNodeNum();}
	void ModifyPartNo(const char* sOldPartNo,const char* sNewPartNo);
	BOOL IsAllDeformedProfile();
	//得到有序集合(角钢集合和钢板集合)
	void GetSortedAngleSetAndPlateSet(CXhPtrSet<CProcessAngle> &angleSet,CXhPtrSet<CProcessPlate> &plateSet);
	void ReadPrjTowerInfoFromCfgFile(const char* cfg_file_path);
};
extern CPPEModel model;
