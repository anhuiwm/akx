#pragma once
#include "Stdafx.h"
#include"SkillInfo.h"
#include "RoleChest.h"
#include "FishLogic\FishUtility.h"
#include "FishLogic\FishCallbak.h"
class CTableRoleManager;
class CRoleEx;
class CConfig;

class CRole
{
	struct ComboInfo
	{
		USHORT m_ComboBulletID;
		USHORT m_crit;
		DWORD m_dwEndTime;
		std::vector<WORD>	m_vecBulletid;
	};
	struct LauncherInfo //����Ϣ
	{
		int nEnergy;
		//bool nEnable;
	};
	typedef std::vector<LauncherInfo>VecLauncherInfo;

public:
	CRole();
	virtual ~CRole();

	bool		OnInit(WORD TableID, BYTE SeatID, CTableRoleManager* pManager);//��ʼ�������ϵ���ҵ�����
	bool		IsActionUser() { return m_pRoleEx != NULL; }//�жϵ�ǰλ���Ƿ�Ϊ�յ�
	void		Clear();
	void		SetUser(CRoleEx* pUser);

	WORD		GetTableID(){ return m_TableID; }
	BYTE		GetSeatID(){ return m_SeatID; }

	DWORD		GetID();
	LPTSTR		GetNickName();
	DWORD		GetFaceID();
	BYTE        GetVipLevel();
	CRoleEx*    GetRoleExInfo(){ return m_pRoleEx; }

	//�Զ���ͷ����
	/*void		OnBeginUpLoadFaceData(WORD Size);
	void		OnUpLoadFaceData(WORD StarIndex, WORD Length, BYTE* pData);*/

	void Update(DWORD dwTimer);

	bool CheckBulletIdx(BYTE &bulletID);

	byte GetLauncherType()const;

	void SetLauncherType(BYTE launcher);

	byte GetTempLauncherType()const;

	void SetTempLauncherType(BYTE launcher);

	byte GetRateIndex()
	{
		return m_nMultipleIndex;
	}
	void AddLauncher(bool badd);
	void SetLauncher(BYTE LauncherType);
	void ResetData();

	void OnCatchFish(CatchType catchType, byte subType,WORD FishType, BYTE byPackageType, int nScore, int nExp);
	bool CheckFire(BYTE &tableType, BYTE byLauncher, DWORD mul = 1);
	bool IsFullEnergy();
	SkillFailedType UseSkill(SkillType skill);
	LaunchFailedType UseLaser(byte launcherType);

	float GetRp();
	__int64 GetGlobel();
	DWORD GetExp();
	WORD  GetLevel();
	DWORD GetProduction();
	DWORD GetGameTime();
	void SetRpEffect(bool badd);
	void AddGameTime(int nSecond);
	USHORT Combo(WORD wCounter);
	void BulletGain(USHORT uBulletType, UINT uGain);
	bool IsComboBuff(USHORT BulletID);
	void OnSkillUsed(float nCdtime);
	void SetBulletRate(USHORT uPoolRate,BYTE byMin, BYTE byMax);
	void SetRoomLauncher();

	RoleChestManager& GetChestManager(){ return m_ChestManager; }

	BYTE GetBulletIdx(){ return m_BulletIdx; }
	ushort SkillInfo(SkillType skill);
	void SetRateIndex(byte idex);
	//void ChangeRateIndex(byte idex);
	void OnChangeRateToIndex(bool UpOrDown, bool& IsCanUse);
	void OnChangeRateToIndex(byte RateIndex, bool& IsCanUse);
	int GetEnergy()
	{
		if (GetLauncherType() >= m_vecLauncherInfo.size())
		{
			SetLauncherType(0);
			ASSERT(false);
		}
		return m_vecLauncherInfo[GetLauncherType()].nEnergy;
	}

	bool IsCanSendTableMsg(){ return m_IsSendTableMsg; }
	void SetRoleIsCanSendTableMsg(bool States){ m_IsSendTableMsg = States; }

	void OnResetRoleLauncher();
	
	bool HaveNormalLauncher(BYTE byType);

	void SetRoleLuckyValue(DWORD Value){ m_LuckyValue = Value; }
	void DelRoleLuckValue(DWORD Value);
	void AddRoleLuckValue(DWORD Value);
	DWORD GetRoleLuckyValue();

	USHORT TableRate();
	float RandTimeRate(float fmin, float fmax, BYTE byFishType);
	void ClearComb();
	void AddDropReward(ushort nReward);

	bool IsNeedLeaveTable();
	void SetLockEndTime();
	void SetAccelerateTime();
	bool IsCanLock();
	bool IsUseLock();
	bool IsCanAccelerate();
	bool IsUseAccelerate();
	ushort MinBulletCosume();
	ushort BulletCosume();
	USHORT CombCount();
	void DelayCombo();
private:
	WORD		m_TableID;//���Ӻ�
	BYTE		m_SeatID;//λ�ú�

	CRoleEx*	m_pRoleEx;//�ⲿ����ҵ���
	byte		m_BulletIdx;//�ӵ� 
	//byte		m_LauncherType;//��̨
	byte        m_nMultipleIndex;//����ʹ��
	VecLauncherInfo				  m_vecLauncherInfo;

	int                         m_nPay;			         //����,��r/p
	int                         m_nRevenue;				 //����  r/p
	float                       m_fRp;
	DWORD						m_dwLastFireTime;
	BYTE						m_byMinRate;
	BYTE						m_byMaxRate;
	USHORT						m_nTableRate;

	CSkillInfo                  m_skill;
	ComboInfo                   m_Combo;
	CConfig                     *m_pConfig;
	CTableRoleManager			*m_pTableRolemanager;

	bool						m_IsSendTableMsg;

	RoleChestManager			m_ChestManager;//���������
	DWORD						m_LuckyValue;//��ҵ�����ֵ

	DWORD						m_dwUpdateRandomtime;
	vector<int> m_vecRandomByTime;;

	DWORD						m_dwLastUseSkillTime;

	DWORD						m_dwLockEndTime;//��������ʱ��
	DWORD						m_dwAccelerateTime;//���ٽ���ʱ��
	BYTE						m_nBulletCount;
	BYTE                        m_TempLanucher = 0;//������ʱ��̨
};
class CTableRoleManager
{
public:
	CTableRoleManager();
	virtual ~CTableRoleManager();

	void		Destroy();
	void		OnInit(WORD TableID, BYTE TableMaxPlayerSum);

	bool		IsFull(){ return GetRoleSum() >= GetMaxPlayerSum(); }
	int			GetRoleSum();//��ȡ�û����� ��ȡ������û�����
	bool		OnDelRole(PlayerID RoleID);
	bool		OnDleRole(WORD ChairID);
	void		OnDelAllRole();
	bool		OnInitRole(CRoleEx* pEx);
	BYTE		GetMaxPlayerSum(){ return m_MaxTableUserSum; }
	CRole*		GetRoleBySeatID(BYTE ChairID);
	CRole*		GetRoleByUserID(PlayerID RoleID);
	void		OnUpdate(DWORD dwTimer);
	//���������л�����
	void		SwitchFishTide();
private:
	WORD			m_TableID;
	BYTE			m_MaxTableUserSum;
	CRole*			m_TableRoleArray;
};

