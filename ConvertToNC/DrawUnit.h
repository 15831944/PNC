#pragma once
#ifndef __DRAW_UNIT_H_
#define __DRAW_UNIT_H_

#include "CadToolFunc.h"
//////////////////////////////////////////////////////////////////////////
//�Ű沼��
class CDrawingRect
{
public:	
	BOOL m_bException;		//�쳣��
	BOOL m_bLayout;			//�ڲ��Ż�����ʱ���ڱ�ʶ�Ƿ��Ѿ�����λ��
	f3dPoint topLeft;		//���ݾ������²��ֺ�����Ͻ�����λ��
	f3dPoint initTopLeft;	//�����²���ǰ��ͼ���ݾ��εĳ�ʼ���Ͻ����꣬���������²��ֺ�topLeft����ֵһ��ȷ�����ݾ��ε�ƫ����
	double width,height;	//��ͼ���ƾ��εĿ�Ⱥ͸߶�
	ARRAY_LIST<f3dPoint> m_vertexArr;
	long feature;
	void *m_pDrawing;
	CDrawingRect() { m_pDrawing = NULL; m_bLayout = FALSE; feature = 0; width = height = 0; }
	BOOL GetPolygon(POLYGON &rgn);
	CDrawingRect(const CDrawingRect &srcRect);
	CDrawingRect& operator=(const CDrawingRect &srcRect);
	void Clone(const CDrawingRect &srcRect);
};
class CDrawingRectLayout
{
	double m_fFrameWidth;	//�߿���
	double m_fFrameHeight;	//�߿�߶�
	CArray<double,double&>m_arrRowHeight;	//
	ARRAY_LIST<GEPOINT> m_arrSideLinePt;	//�Ѳ�������߽��� wht 19-07-27
	void UpdateSideLinePosX(CDrawingRect &rect);
public:
	CArray<CDrawingRect,CDrawingRect&> drawRectArr;
public:
	CDrawingRectLayout();
	//
	BOOL CloseToToLeftTopCorner(CDrawingRect &rect);
	BOOL GetBlankRectTopLeft(CArray<double,double&>&startXArray,CDrawingRect &rect);
	BOOL Relayout(double hight=1000,double width=10000000000000000);
};
#endif