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
		//if (lock && LockFishID == 0)//��������û����������
		//{
		//	LockFishID = pmgr->GetNearFish(ScreenPos);
		//	if (LockFishID != 0) changelock = true;
		//}

		if (lock && LockFishID == 0)//������͸��û����������
		{
			if (curve)//û�г���������
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
		if (lock && LockFishID == 0)//��������û����������
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
		if (lock && LockFishID == 0)//��������û����������
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
		//�ü�����Ļ����ӵ�
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


	bool CheckCollideBoundary()// ����Ļ��ײ 
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
			InnerFishLog(L"������ͬ����ײ��ID:%d", id);
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
	BYTE		ReboundCount;	//��������
	BYTE		CollideCount;	//����ײ����
	BYTE		CatchedNum;		//��������
	BYTE		MaxCatchNum;	//�����ײ����
	PlayerID	Player;			//�ӵ��������ID;
	USHORT		BulletID;		//�ӵ���ID
	USHORT		BulletType;		//�ӵ�������
	Vector2		ScreenPos;
	Vector3		Dir;			//���з���
	Vector3		StartPos;		//��ʼλ��
	Vector3		Pos;
	float		Speed;			//�ٶ�
	float		Time;			//����ʱ��
	DWORD       intervalTime = 0;   //��ײ���ʱ�� ms
	DWORD       updateTime = 0;       //�´θ���ʱ��
	short		Degree;
	byte		RateIndex;
	UINT		CatchGold;
	int			bombTime = 1;       //3��ʾ��ը���� ��͸������
	bool 		curve;                 //���� ������͸��
	bool        lock = false;          //���� ������͸��
	bool        changelock = false;    //�ı�����  ������͸��
	bool        bomb = false; //���㱬ը������
	//bool        changeBomb = false;//�Ѿ����򶨵㱬ը��
	Vector3		bombPos;
	Vector2     bombScreenPos;
	float       radius = 0.0f;
	float       catchRadius = 0.0f;
	SkillType   skill = SkillType::SKILL_MAX;
	BYTE        VipSkill = 0;//0�޼���  1Ϊ����
	BYTE        tableType = 0;
};