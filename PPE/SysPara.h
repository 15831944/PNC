#pragma once
#include "XhCharString.h"
#include "PropListItem.h"
#include "PPEModel.h"

struct FILE_INFO_PARA
{
	CXhChar100 m_sThick;
	DWORD m_dwFileFlag;
	//���²���Ŀǰֻ���ڿ��Ƽ��⸴�ϻ�DXF�ļ���� wht 19-10-22
	BOOL m_bOutputBendLine;
	BOOL m_bOutputBendType;
public:
	FILE_INFO_PARA();
	DWORD AddFileFlag(DWORD dwFlag);
	bool IsValidFile(DWORD dwFlag);
	BOOL SetOutputBendLine(BOOL bValue) { return (m_bOutputBendLine = bValue); }
	BOOL IsOutputBendLine() { return m_bOutputBendLine; }
	BOOL SetOutputBendType(BOOL bValue) { return (m_bOutputBendType = bValue); }
	BOOL IsOutputBendType() { return m_bOutputBendType; }
};
class CSysPara : public ISysPara
{
public:
	CSysPara(void);
	~CSysPara(void);
public:
	//DockPageͣ������
	struct DOCKPAGEPOS
	{
		BOOL bDisplay;
		UINT uDockBarID;
	};
	struct DOCK_ENV{
		DOCKPAGEPOS pagePartList,pageProp;
		short m_nLeftPageWidth,m_nRightPageWidth;
	}dock;
	//
	double m_fHoleIncrement;	//�׾�����
	//NC���ݲ���
	struct NC_PARA{
		//�ְ�NC����
		BOOL m_bAutoSortHole;	//�Ƿ��Զ��Ż���˨��˳��
		BOOL m_bSortByHoleD;	//�Ƿ񰴿׾����з����Ż�
		BYTE m_ciGroupSortType;	//�������򷽰� 0.���׾��Ӵ�С|1.�������Զ����
		double m_fBaffleHigh;	//����߶�
		BOOL m_bDispMkRect;		//��ʾ�ֺ�
		double m_fMKHoleD,m_fMKRectW,m_fMKRectL;
		BYTE m_iNcMode;			//�и�����(�޿�)|�崲�ӹ�(�п�) |����ӹ�
		double m_fLimitSH;		//����׾�����ֵ
		BOOL m_bNeedSH;			//�Ƿ���Ҫ�����
		BOOL m_bNeedMKRect;		//�Ƿ����ֺ�
		BOOL m_iDxfMode;		//DXF�ļ����� 0.������ 1.�����
		CXhChar50 m_sThickToPBJ;		//��׹��հ���趨
		CXhChar50 m_sThickToPMZ;		//��׹��հ���趨
		double m_fShapeAddDist;	//����������ֵ
		//�ְ��������
		BOOL m_bFlameCut;		//�����и�
		FILE_INFO_PARA m_xFlamePara;
		BOOL m_bPlasmaCut;		//�������и�
		FILE_INFO_PARA m_xPlasmaPara;
		BOOL m_bPunchPress;		//�崲�ӹ�
		FILE_INFO_PARA m_xPunchPara;
		BOOL m_bDrillPress;		//�괲�ӹ�
		FILE_INFO_PARA m_xDrillPara;
		//
		FILE_INFO_PARA m_xLaserPara;
		//�Ǹ�NC����
		CString m_sNcDriverPath;
		//�ְ������ļ�·��
		CString m_sPlateConfigFilePath;
	}nc;
	struct PBJ_PARA{
		BOOL m_bIncVertex;	//���������
		BOOL m_iPbjMode;	//���ģʽ
		BOOL m_bAutoSplitFile;	//��˨�����೬�����ֺ��Զ�������ɶ���ļ�
		BOOL m_bIncSH;		//��������
		BOOL m_bMergeHole;	//���������ٵı�׼�׺ϲ�ʹ��ͬһ��ͷ�ӹ������Ǳ�װ���׼�׼ӹ� wht 19-07-25
	}pbj;
	struct PMZ_PARA {
		int m_iPmzMode;		//�ļ�ģʽ��0.���ļ� 1.���ļ�
		BOOL m_bIncVertex;	//���������
		BOOL m_bPmzCheck;	//���Ԥ��PMZ��ʽ
	}pmz;
	//�����и�
	BYTE m_cDisplayCutType;		//��ǰ��ʾ����
	struct FLAME_CUT_PARA{
		CXhChar16 m_sOutLineLen;
		CXhChar16 m_sIntoLineLen;
		BOOL m_bInitPosFarOrg;
		BOOL m_bCutPosInInitPos;
		WORD m_wEnlargedSpace;
	}flameCut,plasmaCut;
	//�׾�����ֵ
	struct HOLE_INCREMENT{
		double m_fDatum;	//����
		double m_fM12;		//M12
		double m_fM16;		//M16
		double m_fM20;		//M20
		double m_fM24;		//M24
		double m_fMSH;		//����׾�����ֵ
	}holeIncrement;
	//��ɫ����
	struct COLOR_MODE{
		COLORREF crLS12;
		COLORREF crLS16;
		COLORREF crLS20;
		COLORREF crLS24;
		COLORREF crOtherLS;
		COLORREF crMark;
	}crMode;
	//�Ǹֹ��տ�����
	struct JGDRAWING_PARA{
		int iDimPrecision;			//�ߴ羫ȷ��
		double fRealToDraw;			//������ͼ�����ߣ�ʵ�ʳߴ�/��ͼ�ߴ磬��1:20ʱ��fRealToDraw=20
		double	fDimArrowSize;		//�ߴ��ע��ͷ��
		double fTextXFactor;
		int		iPartNoFrameStyle;	//��ſ����� //0.ԲȦ 1.��Բ���ľ��ο� 2.���ο�	3.�Զ��ж�
		double	fPartNoMargin;		//����������ſ�֮��ļ�϶ֵ 
		double	fPartNoCirD;		//�������Ȧֱ��
		double fPartGuigeTextSize;	//����������ָ�
		int iMatCharPosType;
		BOOL bModulateLongJg;		//�����Ǹֳ��� ��δʹ�ã���iJgZoomSchema����ñ��� wht 11-05-07
		int iJgZoomSchema;			//�Ǹֻ��Ʒ�����0.1:1���� 1.ʹ�ù���ͼ���� 2.����ͬ������ 3.����ֱ�����
		BOOL bMaxExtendAngleLength;	//����޶�����Ǹֻ��Ƴ���
		double fLsDistThreshold;	//�Ǹֳ����Զ�������˨�����ֵ(���ڴ˼��ʱ��Ҫ���е���);
		double fLsDistZoomCoef;		//��˨�������ϵ��
		BOOL bOneCardMultiPart;		//�Ǹ�����һ��������
		int  iJgGDimStyle;			//0.ʼ�˱�ע  1.�м��ע 2.�Զ��ж�
		int  nMaxBoltNumStartDimG;	//������ʼ�˱�ע׼��֧�ֵ������˨��
		int  iLsSpaceDimStyle;		//0.X�᷽��	  1.Y�᷽��  2.�Զ��ж� 3.����ע  4.�ޱ�ע����(X�᷽��)  4.����ߴ��ߣ��ޱ�ע����(X�᷽��)��Ҫ���ڽ���(��)�����ʼ���ä������
		int  nMaxBoltNumAlongX;		//��X�᷽���ע֧�ֵ������˨����
		BOOL bDimCutAngle;			//��ע�Ǹ��н�
		BOOL bDimCutAngleMap;		//��ע�Ǹ��н�ʾ��ͼ
		BOOL bDimPushFlatMap;		//��עѹ��ʾ��ͼ
		BOOL bJgUseSimpleLsMap;		//�Ǹ�ʹ�ü���˨ͼ��
		BOOL bDimLsAbsoluteDist;	//��ע��˨���Գߴ�
		BOOL bMergeLsAbsoluteDist;	//�ϲ����ڵȾ���˨���Գߴ� �����������:��ʱҲ��Ҫ�� wjh-2014.6.9
		BOOL bDimRibPlatePartNo;	//��ע�Ǹ��߰���
		BOOL bDimRibPlateSetUpPos;	//��ע�Ǹ��߰尲װλ��
		//�нǱ�ע��ʽһ
		//�нǱ�ע��ʽ�� B:��ͷ�ߴ� L:֫�߳ߴ� C:��߳ߴ� 
		//BXL �н�  CXL ��֫ BXC �н�  �д��=�н�+��֫
		int	 iCutAngleDimType;		//�нǱ�ע��ʽ 0.��ʽһ  1.��ʽ�� wht 10-11-01
		//
		BOOL bDimKaiHe;				//��ע�Ǹֿ��Ͻ�
		BOOL bDimKaiheAngleMap;		//��ע�Ǹֿ��Ͻ�ʾ��ͼ
		double fKaiHeJiaoThreshold; //���ϽǱ�ע��ֵ(��) wht 11-05-06
		//�������ϽǱ�ע���� wht 12-03-13
		BOOL bDimKaiheSumLen;		//��ע���������ܳ�
		BOOL bDimKaiheAngle;		//��ע���϶���	
		BOOL bDimKaiheSegLen;		//��ע��������ֶγ�
		BOOL bDimKaiheScopeMap;		//��ע���������ʶ��
		//
		CString sAngleCardPath;
	}jgDrawing;
	struct FONT{
		double  fTextHeight;		//��ͨ��������߶�
		double	fDimTextSize;		//���ȳߴ��ע�ı���
		double	fPartNoTextSize;	//����������ָ�
	}font;
	BOOL Read(CString file_path);	//�������ļ�
	BOOL Write(CString file_path);	//д�����ļ�
	void WriteSysParaToReg(LPCTSTR lpszEntry);	//���湲�ò�����ע���
	void ReadSysParaFromReg(LPCTSTR lpszEntry);	//��ȡ���ò�����ע���
	void UpdateHoleIncrement(double fHoleInc);
	DECLARE_PROP_FUNC(CSysPara);
	int GetPropValueStr(long id, char *valueStr,UINT nMaxStrBufLen=100);//ͨ������Id��ȡ����ֵ
	//
	virtual double GetCutInLineLen(double fThick,BYTE cType=-1);
	virtual double GetCutOutLineLen(double fThick,BYTE cType=-1);
	virtual int GetCutEnlargedSpaceLen(BYTE cType=-1);
	virtual BOOL GetCutInitPosFarOrg(BYTE cType=-1);
	virtual BOOL GetCutPosInInitPos(BYTE cType=-1);
	void AngleDrawingParaToBuffer(CBuffer &buffer);
	//�ְ�NCģʽ����
	DWORD AddNcFlag(BYTE ciNcFlag) {
		nc.m_iNcMode |= ciNcFlag;
		return nc.m_iNcMode;
	}
	bool IsValidNcFlag(BYTE ciNcFlag) {
		if ((ciNcFlag&nc.m_iNcMode) > 0)
			return true;
		else
			return false;
	}
};
extern CSysPara g_sysPara;
