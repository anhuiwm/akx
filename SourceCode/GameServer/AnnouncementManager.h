//�������
#pragma once
#include <deque>
class AnnouncementManager
{
public:
	AnnouncementManager();
	virtual ~AnnouncementManager();
	void OnConnectionCenter();//�������������������ʱ�� ����������
	void OnLoadAllAnnouncementInfoByCenter(CG_Cmd_GetAllAnnouncement* pMsg);
	//void OnLoadAllAnnouncementInfoFinish();

	void OnAddNewAnnouncementOnce(const TCHAR *pNickName,BYTE ShopID,BYTE ShopOnlyID);//���һ������ �����ϴ���Centerȥ ���·����ͻ���
	void OnAddNewAnnouncementOnceByCenter(AnnouncementOnce& pOnce);//������������������µĹ���
	void SendNewAnnouncementToClent(DWORD dwUserID);//���ͻ���
private:
	bool								m_IsFinish;
	std::deque<AnnouncementOnce>		m_AnnouncementList;//˫�˶���
	std::vector<DWORD>					m_RoleVec;
};