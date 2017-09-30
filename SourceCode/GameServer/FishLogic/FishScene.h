#pragma once
#include "FishScene\FishCollider.h"
class FishManager;
class FishLauncher;
class BulletManager;
class FishSendInterface;
class FishMap;
class CTableRoleManager;
struct NetCmd;

#include <list>
class FishScene
{
public:
	FishScene();
	~FishScene();

	bool Init(CTableRoleManager *pm, FishSendInterface *pSend);
	void Shutdown();
	void Update(float deltaTime);
	bool SetMap(const WCHAR *pcMapName);
	void Reset();
	BulletManager * GetBulletMgr()
	{
		return m_pBulletMgr;
	}
	FishManager * GetFishMgr()
	{
		return m_pFishMgr;
	}
	FishLauncher * GetFishLauncher()
	{
		return m_pFishLauncher;
	}
	bool IsEndScene()const
	{
		return m_bSceneEnd && m_pFishMgr->FishCount() == 0;
	}
	const WCHAR *GetMapName()const;

	USHORT GetMapFishBoss();
	void Clear();
	byte GetSceneBackground()const;

	USHORT GetAngleByFish(WORD& LoackFishID, BYTE SeatID, Vector2& Pos, Vector2& pCenter);
protected:
	bool				m_bFishTide;//发现固定流程为true
	CTableRoleManager*  m_pRoleMgr;
	bool				m_bFlowInterval;//切换鱼流程用，中间3.5s为true
	float				m_SwapInterval;
	bool				m_SwapScene;
	UINT				m_FlowInterval;//流程结束，间隔开始时间戳
	USHORT				m_FlowIndex;
	bool				m_bSceneEnd;	//场景中的鱼发射结束。
	FishSendInterface	*m_pSender;
	FishManager			*m_pFishMgr;
	FishLauncher		*m_pFishLauncher;
	BulletManager		*m_pBulletMgr;
	FishMap				*m_pFishMap;
};
