#pragma once
#include "ArrayList.h"
#include "ProcessPart.h"

#ifdef __PNC_
class CNCPlate
{
	CProcessPlate *m_pPlate;
public:
	int m_iNo;				//�����
	BYTE m_cCSMode;			//0.��X+��Y+ 1.��Y+��X-
	BOOL m_bClockwise;		//TRUE ˳ʱ�룬FALSE ��ʱ��
	int m_nInLineLen;		//���볤��
	int m_nOutLineLen;		//��������
	int m_nExtraInLen;		//��������볤��
	int m_nExtraOutLen;		//�������������
	int m_nEnlargedSpace;	//����������ֵ
	BOOL m_bCutSpecialHole;	//�Ƿ��и���
	GEPOINT m_xStartPt;		//
	GEPOINT m_ptCutter;			//��ͷλ��
	struct CUT_PT{
		static const BYTE EDGE_LINE		= 0;
		static const BYTE EDGE_ARC		= 1;
		static const BYTE HOLE_CUT_IN	= 2;
		static const BYTE HOLE_CUT_OUT	= 3;
		static const BYTE HOLE_CIR		= 4;	//��Բ
		BYTE cByte;
		BOOL bHasExtraInVertex,bHasExtraOutVertex;
		GEPOINT extraInVertex,extraOutVertex;	//���������㣬������
		GEPOINT vertex,vertex2,vertex3;			//����㡢�����㡢�и�ԭ��
		GEPOINT centerPt;
		double fSectorAngle;
		double radius;		//�뾶
		BOOL bClockwise;	//˳ʱ��
		CUT_PT(){cByte=0;fSectorAngle=0;bHasExtraInVertex=bHasExtraOutVertex=FALSE;}
	};
	CUT_PT m_cutPt;
	ATOM_LIST<CUT_PT> m_xCutPtList;
	ATOM_LIST<CUT_PT> m_xCutHoleList;	//���и����� wht 19-09-24
public:
	CNCPlate(CProcessPlate *pPlate, int iNo = 0);
	/*CNCPlate(CProcessPlate *pPlate,GEPOINT cutter_pos,int iNo=0,BYTE cCSMode=0,bool bClockwise=false,
			 int nInLineLen=0,int nOutLineLen=0,int nExtraInLen=0,int nExtraOutLen=0,int nEnlargedSpace=0,
			 BOOL bCutSpecialHole=FALSE);*/
	//
	void InitPlateNcInfo();
	bool CreatePlateTxtFile(const char* file_path);
	bool CreatePlateNcFile(const char* file_path);
public:
	static GEPOINT ProcessPoint(const double* coord,BYTE cCSMode=0);
	static bool InitVertextListByNcFile(CProcessPlate *pPlate,const char* file_path);
};
#endif