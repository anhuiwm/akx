#pragma once
#include "FishResData.h"
#include "RuntimeInfo.h"
#include "TArray.h"
#include "FishManager.h"
class Bullet
{
public:
	Bullet()
	{
		ReboundCount = 0;
		LockFishID = 0;
	}
	bool UpdateLockFish(FishManager *pmgr)
	{
		Fish *pfish = pmgr->GetFish(LockFishID);
		if (pfish == NULL)
			return false;

		Vector2 dir = pfish->ScreenPos - ScreenPos;
		float dirlen = Vec2Length(dir);
		dir /= dirlen;
		Dir.x = dir.x;
		Dir.y = dir.y;
		return true;
	}

	bool UpdateBomb(Vector3 bombPos)
	{
		Vector3 op = WorldToProjection(bombPos);
		bombScreenPos = ProjectionToViewport(op);
		Vector2 dir = bombScreenPos - ScreenPos;
		float dirlen = Vec2Length(dir);
		dir /= dirlen;

		//LogInfoToFile("WmCollideLog.txt", "pre BulletID=%d  Dir.x=%f,  Dir.y=%f ", BulletID, Dir.y, Dir.y);


		Dir.x = dir.x;
		Dir.y = dir.y;

		//LogInfoToFile("WmCollideLog.txt", "after BulletID=%d  Dir.x=%f,  Dir.y=%f ", BulletID, Dir.y, Dir.y);
		return true;
	}


	bool ChangeLockFish(FishManager *pmgr)
	{
		//if (lock && LockFishID == 0)//锁定但是没有锁定的鱼
		//{
		//	LockFishID = pmgr->GetNearFish(ScreenPos);
		//	if (LockFishID != 0) changelock = true;
		//}

		if (lock && LockFishID == 0)//锁定穿透弹没有锁定的鱼
		{
			if (curve)//没有出基础区域
			{
				//LogInfoToFile("WmTestLog.txt", "LockFishID=%d, op.x=%f, op.y=%f ,op.z=%f  Dir.x=%f, Dir.y=%f ,Dir.z=%f", LockFishID, Pos.x, Pos.y, Pos.z, Dir.x, Dir.y, Dir.z);
				Vector3 tPos = Pos;
				Vector3 op = WorldToProjection(tPos);
				Vector2 tempScreenPos = ProjectionToViewport(op);

				Vector3 startp = WorldToProjection(StartPos);
				Vector2 startScreenPos = ProjectionToViewport(startp);

				Vector2 dir = tempScreenPos - startScreenPos;
				float dirlen = Vec2Length(dir);
				if (dirlen >= 5.0f)
				{
					LockFishID = pmgr->GetNearFish(ScreenPos);
					if (LockFishID != 0)
					{
						changelock = true;
						//LogInfoToFile("WmTestLog.txt", "ok op.x=%f, op.y=%f ,op.z=%f, dirlen=%f", op.x, op.y, op.z, dirlen);
						if (!UpdateLockFish(pmgr))
						{
							/*if (this->BulletType != 4)
							return false;
							else*/
							ClearLockFishID();
						}
						curve = false;
					}
				}
			}
		}
		else
		{
			if (!UpdateLockFish(pmgr))
			{
				/*if (this->BulletType != 4)
				return false;
				else*/
				ClearLockFishID();
			}
		}
		
		return true;
#if 0
		if (lock && LockFishID == 0)//锁定但是没有锁定的鱼
		{
			LockFishID = pmgr->GetNearFish(ScreenPos);
			if (LockFishID != 0) changelock = true;
		}

		if (changelock)
		{
			if (curve)
			{
				LogInfoToFile("WmTestLog.txt", "LockFishID=%d, op.x=%f, op.y=%f ,op.z=%f  Dir.x=%f, Dir.y=%f ,Dir.z=%f", LockFishID, Pos.x, Pos.y, Pos.z, Dir.x , Dir.y, Dir.z );
				Vector3 tPos = Pos;
				Vector3 op = WorldToProjection(tPos);
				Vector2 tempScreenPos = ProjectionToViewport(op);

				Vector3 startp = WorldToProjection(StartPos);
				Vector2 startScreenPos = ProjectionToViewport(startp);

				Vector2 dir = tempScreenPos - startScreenPos;
				float dirlen = Vec2Length(dir);
				if (dirlen >= 5.0f)
				{
					LogInfoToFile("WmTestLog.txt", "ok op.x=%f, op.y=%f ,op.z=%f, dirlen=%f", op.x, op.y, op.z, dirlen );
					if (!UpdateLockFish(pmgr))
					{
						/*if (this->BulletType != 4)
						return false;
						else*/
						ClearLockFishID();
					}
					curve = false;
				}
			}
			else
			{
				if (!UpdateLockFish(pmgr))
				{
					/*if (this->BulletType != 4)
					return false;
					else*/
					ClearLockFishID();
				}
			}
		}
		return true;
#endif
	}



	bool Update(FishManager *pmgr, float deltaTime)
	{
		if (!lock && LockFishID != 0)
		{
			if (!UpdateLockFish(pmgr))
			{
				/*if (this->BulletType != 4)
				return false;
				else*/
				ClearLockFishID();
			}
		}

		Time += deltaTime;
		Pos += Dir * deltaTime * Speed;
		Vector3 p = WorldToProjection(Pos);
		ScreenPos = ProjectionToViewport(p);
		return CheckBoundary();
#if 0
		if (lock && LockFishID == 0)//锁定但是没有锁定的鱼
		{
			LockFishID = pmgr->GetNearFish(ScreenPos);
			if (LockFishID != 0) changelock = true;
		}

		if (LockFishID != 0)
		{
			if (curve /*|| (skill == SkillType::SKILL_LOCK&&BulletType== BULLET_TYPE_SHOT)*/ )
			{
				//LogInfoToFile("WmTestLog.txt", "LockFishID=%d, op.x=%f, op.y=%f ,op.z=%f  Dir.x=%f, Dir.y=%f ,Dir.z=%f", LockFishID, Pos.x, Pos.y, Pos.z, Dir.x , Dir.y, Dir.z );
				Vector3 tPos = Pos;
				Vector3 op = WorldToProjection(tPos);
				Vector2 tempScreenPos = ProjectionToViewport(op);

				Vector3 startp = WorldToProjection(StartPos);
				Vector2 startScreenPos = ProjectionToViewport(startp);

				Vector2 dir = tempScreenPos - startScreenPos;
				float dirlen = Vec2Length(dir);
				if (dirlen >= 5.0f)
				{
					//LogInfoToFile("WmTestLog.txt", "op.x=%f, op.y=%f ,op.z=%f, dirlen=%f", op.x, op.y, op.z, dirlen );
					if (!UpdateLockFish(pmgr))
					{
						/*if (this->BulletType != 4)
						return false;
						else*/
						ClearLockFishID();
					}
					curve = false;
				}
			}
			//else
			//{
			//	if (!UpdateLockFish(pmgr))
			//	{
			//		/*if (this->BulletType != 4)
			//		return false;
			//		else*/
			//		ClearLockFishID();
			//	}
			//}
		}
#endif

#if 0
		if (bomb && !changeBomb)
		{
			//UpdateBomb(bombPos);
			changeBomb = true;
		}
#endif
	}
	bool CheckBoundary()
	{
		//裁剪掉屏幕外的子弹
		if (Pos.x > RuntimeInfo::NearRightBottomPoint.x || Pos.x < RuntimeInfo::NearLeftTopPoint.x ||
			Pos.y > RuntimeInfo::NearLeftTopPoint.y || Pos.y < RuntimeInfo::NearRightBottomPoint.y)
		{
			if (ReboundCount > 0)
			{
				Vector2 dir = Dir;
				if (Pos.x > RuntimeInfo::NearRightBottomPoint.x)
				{
					Pos.x = RuntimeInfo::NearRightBottomPoint.x;
					Dir.x = -Dir.x;
				}
				else if (Pos.x < RuntimeInfo::NearLeftTopPoint.x)
				{
					Pos.x = RuntimeInfo::NearLeftTopPoint.x;
					Dir.x = -Dir.x;
				}
				if (Pos.y > RuntimeInfo::NearLeftTopPoint.y)
				{
					Pos.y = RuntimeInfo::NearLeftTopPoint.y;
					Dir.y = -Dir.y;

				}
				else if (Pos.y < RuntimeInfo::NearRightBottomPoint.y)
				{
					Pos.y = RuntimeInfo::NearRightBottomPoint.y;
					Dir.y = -Dir.y;
				}
				StartPos = Pos;
				Time = 0;
				--ReboundCount;
				return true;
			}
			else
				return false;
		}
		return true;
	} 


	bool CheckCollideBoundary()// 与屏幕碰撞 
	{
		float rad = radius/*+ catchRadius*/;
		if ( (Pos.x+rad) > RuntimeInfo::NearRightBottomPoint.x || (Pos.x-rad) < RuntimeInfo::NearLeftTopPoint.x ||
			(Pos.y+rad) > RuntimeInfo::NearLeftTopPoint.y || (Pos.y-rad) < RuntimeInfo::NearRightBottomPoint.y )
		{
			return true;
		}
		return false;
	}

	void AddCollideFishID(USHORT id, bool bCatched)
	{
		if(CheckCollideFish(id))
		{
			InnerFishLog(L"存在相同的碰撞鱼ID:%d", id);
			return;
		}
		CatchedNum += bCatched;
		CollideFishList.insert(make_pair(id, bCatched));
	}
	bool CheckCollideFish(USHORT id)
	{
		return CollideFishList.find(id) != CollideFishList.end();
	}
	void ClearLockFishID()
	{
		LockFishID = 0;
		Dir = OrgDir;
	}
	HashMap<USHORT, byte> CollideFishList;
	
	Vector3		OrgDir;
	USHORT		LockFishID;
	BYTE		ReboundCount;	//反弹次数
	BYTE		CollideCount;	//已碰撞次数
	BYTE		CatchedNum;		//捕获数量
	BYTE		MaxCatchNum;	//最大碰撞次数
	PlayerID	Player;			//子弹所属玩家ID;
	USHORT		BulletID;		//子弹的ID
	USHORT		BulletType;		//子弹的类型
	Vector2		ScreenPos;
	Vector3		Dir;			//运行方向
	Vector3		StartPos;		//起始位置
	Vector3		Pos;
	float		Speed;			//速度
	float		Time;			//运行时间
	DWORD       intervalTime = 0;   //碰撞间隔时间 ms
	DWORD       updateTime = 0;       //下次更新时间
	short		Degree;
	byte		RateIndex;
	UINT		CatchGold;
	int			bombTime = 1;       //3表示爆炸三次 穿透反弹用
	bool 		curve;                 //曲线 锁定穿透用
	bool        lock = false;          //锁定 锁定穿透用
	bool        changelock = false;    //改变锁定  锁定穿透用
	bool        bomb = false; //定点爆炸属性用
	//bool        changeBomb = false;//已经朝向定点爆炸点
	Vector3		bombPos;
	Vector2     bombScreenPos;
	float       radius = 0.0f;
	float       catchRadius = 0.0f;
	SkillType   skill = SkillType::SKILL_MAX;
	BYTE        VipSkill = 0;//0无技能  1为冰冻
	BYTE        tableType = 0;
};