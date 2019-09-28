#include "StdAfx.h"
#include "CadHighlightEntManager.h"

//���������Ӧ��ʵ�弯��Ϊ������ʾ
CHashList<AcDbObjectId> CCadHighlightEntManager::hashHighlightEnts;
void CCadHighlightEntManager::SetEntSetHighlight(ARRAY_LIST<AcDbObjectId> &entIdList)
{
	if(entIdList.GetSize()==0)
		return;
	ads_name ent_name;
	for(AcDbObjectId *pEntId=entIdList.GetFirst();pEntId;pEntId=entIdList.GetNext())
	{
		if(acdbGetAdsName(ent_name,*pEntId)!=Acad::eOk)
			continue;
		ads_redraw(ent_name,3);//������ʾ
		hashHighlightEnts.SetValue((long)pEntId,*pEntId);
	}
	//���½���
	actrTransactionManager->flushGraphics();
	acedUpdateDisplay();
}
void CCadHighlightEntManager::ReleaseHighlightEnts()
{
	for(AcDbObjectId *pEntId=hashHighlightEnts.GetFirst();pEntId;pEntId=hashHighlightEnts.GetNext())
	{
		ads_name ent_name;
		if(acdbGetAdsName(ent_name,*pEntId)!=Acad::eOk)
			continue;
		ads_redraw(ent_name,4);//ȡ��������ʾ
	}
	hashHighlightEnts.Empty();
}