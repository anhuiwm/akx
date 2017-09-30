#include "stdafx.h"
#include "Config.h"
#if !SIMPLE_SERVER_MODE
#include "..\Role.h"
#include "..\TableManager.h"
#endif
#include"FishScene\FishResManager.h"
#ifndef ASSERT
#define ASSERT assert
#endif
//#include"..\CommonFile\tinyxml\tinyxml.h"
#include"FishScene\tinyxml2.h"
#include"..\FishServer.h"
#include "..\CommonFile\FishServerConfig.h"
//#include "FishServer.h"
const float SPEED_SCALE = 1.0f / 255;
enum CANNON_TYPE //nature:��̨���� Ĭ��0����ģʽ,1ɢ������,2����ը��͸����,3��͸����,4������͸
{
	CANNON_0,  //��������                  nature="0"
	CANNON_1,  //����						 nature="0"
	CANNON_2,  //����						 nature="0"
	CANNON_3,  //����						 nature="0"
	CANNON_4,  //ɢ�� 					 nature="1"
	CANNON_5,  //ɢ��						 nature="1"
	CANNON_6,  //������͸				   nature="4"
	CANNON_7,  //������͸				   nature="4"
	CANNON_8,  //����ը��͸				 nature="2"
	CANNON_9,  //��͸����				   nature="3"
	CANNON_10,  //��ʱ ����ը��͸		   nature="2"
	CANNON_11,  //��ʱ ����				   nature="0"
	CANNON_12,  //��ʱ ������͸			 nature="4"
	CANNON_13,  //��ʱ ɢ��				   nature="1"
};
//float Con_Energy::s_energyratio = 0;
int Con_Combo::s_sustaintime = 0;
int Con_Combo::s_buff_cycle = 0;
float Con_Combo::s_bufferchance;

float Con_Cannon::s_ratiofactor = 0;
float Con_Cannon::s_maxratio = 0;
float Con_Cannon::s_finalratio = 0;
float Con_Cannon::s_lockratio = 0;

float Con_Fish::s_revies_Activity = 0;
int Con_Fish::s_flashfishid;
int Con_Fish::s_flashfishCatch;
int Con_Fish::s_flashfishradius;

float Con_Rp::s_cycle = 0;
float Con_Rp::s_effecttime = 0;

//int Randomtime::s_cycle = 0;
//float Randomtime::s_reviseratio1 = 0.0f;
//float Randomtime::s_reviseratio2 = 0.0f;

const float PRECISION = 0.0001f;//���ƾ���
const float ENLARGE = 100000.0f;

int Con_LuckItem::s_threshold;
float Con_LuckItem::s_base;
float Con_LuckItem::s_part;

int Con_Rate::rough_id = 0;
int Con_Rate::qianpao_id = 45;
vector<int> Con_Rate::material_ids;

float Con_StockItem::s_stockscore[MAX_TABLE_TYPE + 1] = {0.0f};
float Con_StockItem::s_tax[MAX_TABLE_TYPE + 1] = { 0.0f };
float Con_StockItem::s_taxscore[MAX_TABLE_TYPE + 1] = { 0.0f };
using namespace tinyxml2;
bool Attribute(const XMLElement*pElement, const char*pName, int&nvalue)
{
	nvalue = 0;
	const char*pItem = pElement->Attribute(pName);
	if (pItem)
	{
		nvalue = atol(pItem);
		return true;
	}
	return false;
}

bool Attribute(const XMLElement*pElement, const char*pName, float&fvalue)
{
	fvalue = 0;
	const char*pItem = pElement->Attribute(pName);
	if (pItem)
	{
		fvalue = static_cast<float>(atof(pItem));
		return true;
	}
	return false;
}

float Con_RandomTime::GetRandomTimeRatio(int& TimeSec)
{
	if (DroupVec.empty())
		return 0;
	int RandValue = RandUInt() % TotalRateValue;
	vector<RandomTimeInfo>::iterator Iter = DroupVec.begin();
	for (; Iter != DroupVec.end(); ++Iter)
	{
		if (RandValue < Iter->RateValue)
		{
			TimeSec = Iter->TimeSec;
			return CConfig::CalRandom(Iter->reviseratio1, Iter->reviseratio2);
		}
	}
	return 0.0f;
}
void Con_RandomTime::clear()
{
	TotalRateValue = 0;
	DroupVec.clear();
}
CConfig::CConfig(TableManager *pTableManager)
{
	m_nServerVersion = 0;
	//ZeroMemory(m_nRandomByTime, sizeof(m_nRandomByTime));
	m_pTableManager = pTableManager;
}

CConfig::~CConfig()
{

}

bool CConfig::LoadConfig(char szDir[])
{
	{
		m_Energy.clear();
		m_Skill.clear();
		m_Nature.clear();
		m_Rate.clear();
		m_Cannon.clear();
		m_Fish.clear();
		m_Rp.clear();
		m_production.clear();
		m_Rank.clear();
		m_PlayTime.clear();
		m_Money.clear();
		Con_Rate::material_ids.clear();
		//m_vecgood.clear();
		m_VceLuck.clear();
		m_GoldPool.clear();
		m_Stock.clear();
		m_Recharge.clear();
		m_RandomTime.clear();
	}

	tinyxml2::XMLDocument xml_doc;
	char szConfig[MAX_PATH] = { 0 };
	sprintf_s(szConfig, sizeof(szConfig), "%s\\config.xml", szDir);

	if (xml_doc.LoadFile(szConfig) != XML_NO_ERROR) return false;

	XMLElement* xml_element = xml_doc.FirstChildElement("Config");
	if (xml_element == NULL) return false;

	const XMLElement* xml_child = NULL;
	for (xml_child = xml_element->FirstChildElement(); xml_child; xml_child = xml_child->NextSiblingElement())
	{
		if (!strcmp(xml_child->Value(), "ServerVersion"))
		{
			Attribute(xml_child, "version", m_nServerVersion);

			int nMaxBullet;
			int nMaxFish;
			Attribute(xml_child, "maxbullet", nMaxBullet);
			Attribute(xml_child, "maxfish", nMaxFish);


		}
		else if (!strcmp(xml_child->Value(), "Combo"))
		{
			Attribute(xml_child, "sustain_time", Con_Combo::s_sustaintime);
			Attribute(xml_child, "buff_cycle", Con_Combo::s_buff_cycle);
			Attribute(xml_child, "bufferchance", Con_Combo::s_bufferchance);						
		}
		else if (!strcmp(xml_child->Value(), "Energy"))
		{
			for (const XMLElement* xml_data = xml_child->FirstChildElement(); xml_data; xml_data = xml_data->NextSiblingElement())
			{
				Con_Energy  energy_item = { 0 };
				Attribute(xml_data, "id", energy_item.nid);


				Attribute(xml_data, "cditme", energy_item.ncdtime);
				Attribute(xml_data, "mincatch", energy_item.nmincatch);
				Attribute(xml_data, "maxcatch", energy_item.nmaxcatch);
				Attribute(xml_data, "revise", energy_item.nrevise);
				Attribute(xml_data, "energyratio", energy_item.denergyratio);

				Attribute(xml_data, "radius", energy_item.nradius);

				//xml_data->Attribute("playtime", energy_item.nplaytime);
				Attribute(xml_data, "speed", energy_item.npseed);
				Attribute(xml_data, "time1", energy_item.ntime1);
				Attribute(xml_data, "time2", energy_item.ntime2);
				Attribute(xml_data, "time3", energy_item.ntime3);
				m_Energy.push_back(energy_item);
			}
		}
		else if (!strcmp(xml_child->Value(), "Skill"))
		{

			for (const XMLElement* xml_data = xml_child->FirstChildElement(); xml_data; xml_data = xml_data->NextSiblingElement())
			{
				Con_Skill skill_item = { 0 };
				Attribute(xml_data, "id", skill_item.nid);

				Attribute(xml_data, "cditme", skill_item.ncdtime);
				Attribute(xml_data, "mincatch", skill_item.nmincatch);
				Attribute(xml_data, "maxcatch", skill_item.nmaxcatch);
				Attribute(xml_data, "revise", skill_item.nrevise);

				Attribute(xml_data, "goodsid", skill_item.ngoodsid);
				Attribute(xml_data, "multiple", skill_item.multiple);


				char *pInfo = const_cast<char*>(xml_data->Attribute("goodsconsume"));
				while (pInfo&&*pInfo)
				{
					SkillConsume consume = { 0 };
					consume.byorder = ConvertIntToBYTE(strtol(pInfo, &pInfo, 10));
					consume.byCount = ConvertIntToBYTE(strtol(pInfo, &pInfo, 10));
					skill_item.vecConsume.push_back(consume);
				}

				Attribute(xml_data, "playtime", skill_item.nplaytime);
				Attribute(xml_data, "speed", skill_item.npseed);
				Attribute(xml_data, "time1", skill_item.ntime1);
				Attribute(xml_data, "time2", skill_item.ntime2);
				Attribute(xml_data, "time3", skill_item.ntime3);
				Attribute(xml_data, "launcherinterval", skill_item.launcherinterval);
				m_Skill.push_back(skill_item);
			}
		}
		else if (!strcmp(xml_child->Value(), "Nature"))
		{

			for (const XMLElement* xml_data = xml_child->FirstChildElement(); xml_data; xml_data = xml_data->NextSiblingElement())
			{
				Con_Nature nature_item = { 0 };
				Attribute(xml_data, "id", nature_item.nid);
				Attribute(xml_data, "shootcount", nature_item.shootcount);
				Attribute(xml_data, "rebound", nature_item.rebound);
				Attribute(xml_data, "lock", nature_item.lock);
				Attribute(xml_data, "interval", nature_item.intervaltime);
				m_Nature.push_back(nature_item);
			}
		}
		else if (!strcmp(xml_child->Value(), "Rate"))
		{
			Attribute(xml_child, "rough_id", Con_Rate::rough_id);
			Attribute(xml_child, "qianpao_id", Con_Rate::qianpao_id);
			//Con_Rate::material_ids.push_back(Con_Rate::rough_id);
			//��ȡ�����ײ����
			char* pCollision = const_cast<char*>(xml_child->Attribute("material_ids"));
			while (pCollision&&*pCollision)
			{
				Con_Rate::material_ids.push_back(static_cast<int>(strtod(pCollision, &pCollision)));
			}

			for (const XMLElement* xml_data = xml_child->FirstChildElement(); xml_data; xml_data = xml_data->NextSiblingElement())
			{
				Con_Rate rate_item = { 0 };
				Attribute(xml_data, "id", rate_item.nid);
				Attribute(xml_data, "value", rate_item.nValue);
				Attribute(xml_data, "unlock", rate_item.nUnlock);
				Attribute(xml_data, "unlockreward", rate_item.unlockreward);
				Attribute(xml_data, "drop", rate_item.drop);
				Attribute(xml_data, "material", rate_item.material);
				Attribute(xml_data, "rough", rate_item.rough);
				Attribute(xml_data, "success_rate", rate_item.success_rate);
				Attribute(xml_data, "match_add_score", rate_item.match_add_score);
				m_Rate.push_back(rate_item);
			}
		}
		else if (!strcmp(xml_child->Value(), "Cannon"))
		{
			Attribute(xml_child, "ratiofactor", Con_Cannon::s_ratiofactor);
			Attribute(xml_child, "maxratio", Con_Cannon::s_maxratio);
			Attribute(xml_child, "finalration", Con_Cannon::s_finalratio);
			Attribute(xml_child, "lockratio", Con_Cannon::s_lockratio);
			
			for (const XMLElement* xml_data = xml_child->FirstChildElement(); xml_data; xml_data = xml_data->NextSiblingElement())
			{
				Con_Cannon cannon_item = { 0 };
				Attribute(xml_data, "id", cannon_item.nid);

				Attribute(xml_data, "bulletspeed", cannon_item.bulletspeed);		

				Attribute(xml_data, "launcherinterval", cannon_item.dlauncherinterval);

				Attribute(xml_data, "consume", cannon_item.nconsume);
				Attribute(xml_data, "radius", cannon_item.nradius);
				Attribute(xml_data, "catchradius", cannon_item.ncatchradius);
				Attribute(xml_data, "maxcatch", cannon_item.nmaxcatch);

				Attribute(xml_data, "itemid", cannon_item.nItemId);
				Attribute(xml_data, "itemcount", cannon_item.nItemCount);
				Attribute(xml_data, "skill", cannon_item.nskill);	
				Attribute(xml_data, "nature", cannon_item.nNature);
				//
				Attribute(xml_data, "speed", cannon_item.npseed);
				Attribute(xml_data, "time1", cannon_item.ntime1);
				Attribute(xml_data, "time2", cannon_item.ntime2);
				Attribute(xml_data, "time3", cannon_item.ntime3);

				//��ȡ�����ײ����
				char* pCollision = const_cast<char*>(xml_data->Attribute("collisionrate"));
				while (pCollision&&*pCollision)
				{
					cannon_item.vCollision.push_back(static_cast<float>(strtod(pCollision, &pCollision)));
				}
				m_Cannon.push_back(cannon_item);
			}
		}
		else if (!strcmp(xml_child->Value(), "Fish"))
		{
			Attribute(xml_child, "revies_Activity", Con_Fish::s_revies_Activity);
			Attribute(xml_child,"flashfishid", Con_Fish::s_flashfishid);
			Attribute(xml_child,"flashfishCatch", Con_Fish::s_flashfishCatch);
			Attribute(xml_child,"flashfishradius", Con_Fish::s_flashfishradius);

			for (const XMLElement* xml_data = xml_child->FirstChildElement(); xml_data; xml_data = xml_data->NextSiblingElement())
			{
				Con_Fish fish_item = { 0 };
				Attribute(xml_data, "id", fish_item.nid);
				Attribute(xml_data, "value", fish_item.nvalue);
				Attribute(xml_data, "maxcount", fish_item.maxcount);
				//Attribute(xml_data, "reviseratio", fish_item.reviseratio);
				Attribute(xml_data, "chance", fish_item.chance);
				Attribute(xml_data, "type", fish_item.type);
				Attribute(xml_data, "isfishboss", fish_item.isfishboss);
				Attribute(xml_data, "flashchance", fish_item.flashchance);			

				const char* nStr = xml_data->Attribute("name");
				if ( nStr == NULL)
				{
					return false;
				}
				::MultiByteToWideChar(CP_UTF8, 0, nStr, -1, fish_item.name, MAX_NAME_LENTH);

				char* pfishrate = const_cast<char*>(xml_data->Attribute("fishrate"));
				while (pfishrate&&*pfishrate)
				{
					fish_item.fishrate.push_back((float)strtod(pfishrate, &pfishrate));
				}

				m_Fish.push_back(fish_item);

			}
		}
		else if (!strcmp(xml_child->Value(), "Rp"))
		{
			Attribute(xml_child, "cycle", Con_Rp::s_cycle);
			Attribute(xml_child, "effecttime", Con_Rp::s_effecttime);

			for (const XMLElement* xml_data = xml_child->FirstChildElement(); xml_data; xml_data = xml_data->NextSiblingElement())
			{
				Con_Rp rp_item = { 0 };
				Attribute(xml_data, "id", rp_item.nid);

				Attribute(xml_data, "rp", rp_item.rp);
				Attribute(xml_data, "reviseratio1", rp_item.reviseratio1);

				if (!Attribute(xml_data, "reviseratio2", rp_item.reviseratio2))//û��Ĭ��
				{
					rp_item.reviseratio2 = rp_item.reviseratio1 + PRECISION;
				}
				m_Rp.push_back(rp_item);
			}
		}
		else if (!strcmp(xml_child->Value(), "Recharges"))
		{
			for (const XMLElement* xml_data = xml_child->FirstChildElement(); xml_data; xml_data = xml_data->NextSiblingElement())
			{
				Con_Recharge re_item = { 0 };
				Attribute(xml_data, "price", re_item.price);
				Attribute(xml_data, "timesec",re_item.timesec);
				Attribute(xml_data, "ratio", re_item.ratio);

				m_Recharge.insert(make_pair(re_item.price,re_item));
			}
		}
		else if (!strcmp(xml_child->Value(), "Randomtime"))
		{
			for (const XMLElement* xml_data = xml_child->FirstChildElement(); xml_data; xml_data = xml_data->NextSiblingElement())
			{
				RandomTimeInfo item = { 0 };
				Attribute(xml_data, "Chances", item.RateValue);
				Attribute(xml_data, "timesec", item.TimeSec);
				Attribute(xml_data, "reviseratio1", item.reviseratio1);
				Attribute(xml_data, "reviseratio2", item.reviseratio2);
				m_RandomTime.TotalRateValue += item.RateValue;
				item.RateValue = m_RandomTime.TotalRateValue;
				m_RandomTime.DroupVec.push_back(item);
			}
		}
		else if (!strcmp(xml_child->Value(), "Production"))
		{
			for (const XMLElement* xml_data = xml_child->FirstChildElement(); xml_data; xml_data = xml_data->NextSiblingElement())
			{
				Con_Production item = { 0 };
				Attribute(xml_data, "id", item.nid);

				Attribute(xml_data, "income", item.nincome);
				Attribute(xml_data, "reviseratio1", item.reviseratio1);

				if (!Attribute(xml_data, "reviseratio2", item.reviseratio2))//û��Ĭ��
				{
					item.reviseratio2 = item.reviseratio1 + PRECISION;
				}
				m_production.push_back(item);
			}
		}
		else if (!strcmp(xml_child->Value(), "Rank"))
		{
			for (const XMLElement* xml_data = xml_child->FirstChildElement(); xml_data; xml_data = xml_data->NextSiblingElement())
			{
				Con_Rank item = { 0 };
				Attribute(xml_data, "id", item.nid);

				item.level = item.nid;
				Attribute(xml_data, "experience", item.experience);
				Attribute(xml_data, "reviseratio", item.reviseratio);
				Attribute(xml_data, "drop", item.drop);
				m_Rank.push_back(item);
			}
		}
		else if (!strcmp(xml_child->Value(), "PlayTime"))
		{
			for (const XMLElement* xml_data = xml_child->FirstChildElement(); xml_data; xml_data = xml_data->NextSiblingElement())
			{
				Con_Playtime item = { 0 };
				Attribute(xml_data, "id", item.nid);

				Attribute(xml_data, "time", item.ntime);
				Attribute(xml_data, "reviseratio1", item.reviseratio1);
				if (!Attribute(xml_data, "reviseratio2", item.reviseratio2))//û��Ĭ��
				{
					item.reviseratio2 = item.reviseratio1 + PRECISION;
				}
				Attribute(xml_data, "drop", item.drop);
				m_PlayTime.push_back(item);
			}
		}
		else if (!strcmp(xml_child->Value(), "Money"))
		{
			//xml_child->
			for (const XMLElement* xml_data = xml_child->FirstChildElement(); xml_data; xml_data = xml_data->NextSiblingElement())
			{
				Con_Money item = { 0 };

				Attribute(xml_data, "id", item.nid);

				Attribute(xml_data, "money", item.money);
				Attribute(xml_data, "reviseratio1", item.reviseratio1);
				if (!Attribute(xml_data, "reviseratio2", item.reviseratio2))//û��Ĭ��
				{
					item.reviseratio2 = item.reviseratio1 + PRECISION;
				}
				m_Money.push_back(item);
			}
		}
		else if (!strcmp(xml_child->Value(), "Luck"))
		{
			Attribute(xml_child,"threshold", Con_LuckItem::s_threshold);
			Attribute(xml_child,"base", Con_LuckItem::s_base);
			Attribute(xml_child,"part", Con_LuckItem::s_part);

			for (const XMLElement* xml_data = xml_child->FirstChildElement(); xml_data; xml_data = xml_data->NextSiblingElement())
			{
				Con_LuckItem luck_item = { 0 };
				Attribute(xml_data,"id", luck_item.nid);
				Attribute(xml_data,"nluck", luck_item.nluck);
				Attribute(xml_data,"reviseratio1", luck_item.reviseratio1);
				if (!Attribute(xml_data,"reviseratio2", luck_item.reviseratio2))//û��Ĭ��
				{
					luck_item.reviseratio2 = luck_item.reviseratio1 + PRECISION;
				}
				m_VceLuck.push_back(luck_item);
			}
		}
		else if (!strcmp(xml_child->Value(), "Stock"))
		{
			for (const XMLElement* xml_type_data = xml_child->FirstChildElement(); xml_type_data; xml_type_data = xml_type_data->NextSiblingElement())
			{
				StockType_Map typeStockMap;
				int type = 0;
				int basescore = 0;
				Attribute(xml_type_data, "type", type);
				Attribute(xml_type_data, "basescore", basescore);
				//for (int i = 0; i <= MAX_TABLE_TYPE; i++)
				if (type > MAX_TABLE_TYPE)
				{
					return false;
				}
				if (m_Stock.find(type) != m_Stock.end())
				{
					continue;
				}
				Con_StockItem::s_stockscore[type] = basescore;
				Attribute(xml_type_data, "tax", Con_StockItem::s_tax[type]);

				for (const XMLElement* xml_data = xml_type_data->FirstChildElement(); xml_data; xml_data = xml_data->NextSiblingElement())
				{
					int score = 0;
					Con_StockItem item = { 0 };
					Attribute(xml_data, "stockmaxscore", score);
					Attribute(xml_data, "reviseratio1", item.reviseratio1);
					if (!Attribute(xml_data, "reviseratio2", item.reviseratio2))//û��Ĭ��
					{
						item.reviseratio2 = item.reviseratio1 + PRECISION;
					}
					item.stockscore = score;
					typeStockMap.insert(make_pair(item.stockscore, item));
				}
				m_Stock.insert(make_pair(type, typeStockMap));
			}
		}
		else if (!strcmp(xml_child->Value(), "Pool"))
		{
			for (const XMLElement* xml_data = xml_child->FirstChildElement(); xml_data; xml_data = xml_data->NextSiblingElement())
			{
				Con_Pool pool_item = { 0 };				
				Attribute(xml_data,"id", pool_item.nid);
				Attribute(xml_data,"pool", pool_item.pool);
				Attribute(xml_data,"reviseratio1", pool_item.reviseratio1);
				Attribute(xml_data,"reviseratio2", pool_item.reviseratio2);

				m_GoldPool.push_back(pool_item);
			}
		}
		else  if (!strcmp(xml_child->Value(), "FishDeadTime"))//
		{
			DeadStayTime temp = { 0 };
			Attribute(xml_child, "TianZai_DouDong", temp.TianZai_DouDong_Time);           //���ֶ���ʱ��

			Attribute(xml_child, "TianZai_Stay_Min", temp.TianZai_Stay_Time1);             //���ֵȴ���ʯ����ͣ�����ʱ��
			Attribute(xml_child, "TianZai_Stay_Max", temp.TianZai_Stay_Time2);             //���ֵȴ���ʯ����ͣ���ʱ��

			Attribute(xml_child, "TianZai_Dead_Min", temp.TianZai_Dead_Time1);             //��������ͣ�����ʱ��
			Attribute(xml_child, "TianZai_Dead_Max", temp.TianZai_Dead_Time2);             //��������ͣ���ʱ��

			Attribute(xml_child, "Bullet_BingDong_Dead_Min", temp.Bullet_BingDong_Dead_Time1);     //�����ڻ�������ͣ�����ʱ��
			Attribute(xml_child, "Bullet_BingDong_Dead_Max", temp.Bullet_BingDong_Dead_Time2);     //�����ڻ�������ͣ������ʱ��

			Attribute(xml_child, "BingDong_Dead_Min", temp.BingDong_Dead_Time1);            //���������������ʱ��
			Attribute(xml_child, "BingDong_Dead_Max", temp.BingDong_Dead_Time2);            //�������������ʱ��

			Attribute(xml_child, "ShanDian_Dead_Min", temp.ShanDian_Dead_Time1);            //���缼���������ʱ��
			Attribute(xml_child, "ShanDian_Dead_Max", temp.ShanDian_Dead_Time2);            //���缼�������ʱ��

			Attribute(xml_child, "LongJuanFeng_Dead_Min", temp.LongJuanFeng_Dead_Time1);        //����缼���������ʱ��
			Attribute(xml_child, "LongJuanFeng_Dead_Max", temp.LongJuanFeng_Dead_Time2);        //����缼�������ʱ��

			Attribute(xml_child, "JiGuang_Stay_Time", temp.JiGuang_Stay_Time);
			Attribute(xml_child, "JiGuang_Dead_Min", temp.JiGuang_Dead_Time1);             //��������������ʱ��
			Attribute(xml_child, "JiGuang_Dead_Max", temp.JiGuang_Dead_Time2);             //������������ʱ��

			Attribute(xml_child, "ShandDian_Speed", temp.ShandDian_Speed);
			Attribute(xml_child, "JiGuang_Speed", temp.JiGuang_Speed);
			Attribute(xml_child, "JiGuang_BingDong_Speed", temp.JiGuang_BingDong_Speed);
			Attribute(xml_child, "TianZai_Speed", temp.TianZai_Speed);
			Attribute(xml_child, "BingDong_Speed", temp.BingDong_Speed);

			FishResManager::Inst()->SetDeadStayTime(temp);

		}
	}
	if (m_Energy.size() != MAX_LAUNCHER_NUM || m_Cannon.size() != MAX_LAUNCHER_NUM)
	{
		cout << "MAX_LAUNCHER_NUM" << endl;
		return false;
	}
	if (m_Skill.size() != SKILL_MAX)
	{
		cout << "SKILL_MAX" << endl;
		return false;
	}
	if (m_Nature.size() != BULLET_TYPE_MAX)
	{
		cout << "BULLET_TYPE_MAX" << endl;
		return false;
	}
	/*if (m_Fish.size() != FISH_TYPES)
	{
		cout << "FISH_TYPES" << endl;
		return false;
	}*/	

	sort(m_Energy.begin(), m_Energy.end(), SortByid<Con_Energy>);
	sort(m_Skill.begin(), m_Skill.end(), SortByid<Con_Skill>);
	sort(m_Rate.begin(), m_Rate.end(), SortByid<Con_Rate>);
	sort(m_Cannon.begin(), m_Cannon.end(), SortByid<Con_Cannon>);
	sort(m_Fish.begin(), m_Fish.end(), SortByid<Con_Fish>);
	sort(m_Rp.begin(), m_Rp.end(), SortByid<Con_Rp>);
	sort(m_production.begin(), m_production.end(), SortByid<Con_Production>);
	sort(m_Rank.begin(), m_Rank.end(), SortByid<Con_Rank>);
	sort(m_PlayTime.begin(), m_PlayTime.end(), SortByid<Con_Playtime>);
	sort(m_Money.begin(), m_Money.end(), SortByid<Con_Money>);
	sort(m_VceLuck.begin(), m_VceLuck.end(), SortByid<Con_LuckItem>);
	sort(m_GoldPool.begin(), m_GoldPool.end(), SortByid<Con_Pool>);


	int nFishs = 0;
	int nFishValues = 0;
	for (Fish_Vec::iterator it = m_Fish.begin(); it != m_Fish.end(); it++)
	{
		nFishs += it->maxcount;
		nFishValues += it->maxcount*it->nvalue;
	}

	for (Energy_Vec::iterator it = m_Energy.begin(); it != m_Energy.end(); it++)
	{
		Con_Energy &item = *it;
		item.nincome = ConvertFloatToInt((item.nmincatch + item.nmaxcatch) / 2.0f / nFishs*nFishValues + item.nrevise);
		item.nthreshold = ConvertFloatToInt(item.nincome*item.denergyratio);//����
	}

	for (Skill_Vec::iterator it = m_Skill.begin(); it != m_Skill.end(); it++)
	{
		Con_Skill &item = *it;
		item.nincome = ConvertFloatToInt((item.nmincatch + item.nmaxcatch) / 2.0f / nFishs*nFishValues + item.nrevise);
	}

	//read goods
	//if (!xml_doc.LoadFile(szGood, TIXML_ENCODING_UTF8)) return false;
	//xml_element = xml_doc.FirstChildElement("Goods");
	//if (xml_element == NULL) return false;
	//for (xml_child = xml_element->FirstChildElement(); xml_child; xml_child = xml_child->NextSiblingElement())
	//{
	//	FISH_GOODS item = { 0 };
	//	xml_child->Attribute("id", &item.nid);
	//	//xml_child->Attribute("name", item.chname);
	//	xml_child->Attribute("price", &item.nprice);
	//	
	//	m_vecgood.push_back(item);
	//}
	//sort(m_vecgood.begin(), m_vecgood.end(), SortByid<FISH_GOODS>);

	return true;
}

//pei zhi
UINT  CConfig::GetVersion()
{
	return m_nServerVersion;
}

float CConfig::BulletRadius(USHORT bulletID)				//�ӵ������뾶
{
	ASSERT(bulletID < m_Cannon.size());
	return m_Cannon[bulletID].nradius;
}

float CConfig::BulletCatchRadius(USHORT bulletID)					//�ӵ���ײ֮��Ĳ���뾶
{
	ASSERT(bulletID < m_Cannon.size());
	return m_Cannon[bulletID].ncatchradius;
}

UINT  CConfig::MaxCatch(USHORT bulletID)				//�ӵ������������
{
	ASSERT(bulletID < m_Cannon.size());
	return m_Cannon[bulletID].nmaxcatch;
}

float CConfig::CatchChance(PlayerID player, USHORT bulletId, byte bulletType, byte fishType, int byOrder, int byMaxCatch, BYTE byPackageType,bool bLocked)
{
#if SIMPLE_SERVER_MODE
	return 0.5f;
#else

	ASSERT(bulletType < m_Cannon.size());
	ASSERT(fishType<m_Fish.size());
	//��ӻ����˹����� ��������Ƶ�� ��ʱ�ر�assert 2017.5.17
	//ASSERT(ConvertIntToDWORD(byOrder) < m_Cannon[bulletType].vCollision.size()/2);

	if (bulletType >= m_Cannon.size())
	{
		return 0;
	}
	if (fishType >= m_Fish.size())
	{
		return 0;
	}
	if (ConvertIntToDWORD(byOrder) >= m_Cannon[bulletType].vCollision.size()/2)
	{
		return 0;
	}

	CRole *pPlayer = m_pTableManager->SearchUser(player);

	if (pPlayer)
	{

		float fcombobuff = 0.0f;
		if (pPlayer->IsComboBuff(bulletId))
		{
			if (g_FishServer.GetFishConfig().GetChestConfig().ChestMap.find(byPackageType) == g_FishServer.GetFishConfig().GetChestConfig().ChestMap.end())
			{
				/*return */ fcombobuff = Con_Combo::s_bufferchance;
			}
		}
		float fstock = 0.0f;
		GameTable* pTable = g_FishServer.GetTableManager()->GetTable(pPlayer->GetTableID());
		if (NULL != pTable)
		{
			__int64 stockscore = GetStaticStockScore(pTable->GetTableTypeID());
			fstock = StockRate(pTable->GetTableTypeID(),stockscore);
		}
		float fbase = CalBaseRatio(bulletType, fishType);//��������
		int TimeSec = 0;
		float fRandTime = pPlayer->RandTimeRate();// m_RandomTime.GetRandomTimeRatio(TimeSec);//pPlayer->RandTimeRate(Randomtime::s_reviseratio1, Randomtime::s_reviseratio2, fishType);
		float fLockRate = bLocked ? Con_Cannon::s_lockratio : 0;//�������Ӹ���
		//std::cout <<"rand:"<< fRandTime << endl;
		float frp = RpRate(pPlayer->GetRp());
		//float fProduction = ProductionRate(pPlayer->GetProduction());
		//float fMoney = MoneyRate(pPlayer->GetScore());			
		float fRank = RankRate(pPlayer->GetLevel());
		float fGameTime = GameTimeRate(pPlayer->GetGameTime());
		float fRecharge = pPlayer->RechargeRate();
		int64 nPoolGold;
		int nLuck = pPlayer->GetRoleLuckyValue();		
		if (m_pTableManager->QueryPool(pPlayer->GetTableID(), nPoolGold) && nLuck!=0)//���ش���,������ֵ
		{						
			float fluck = LuckRate(nLuck);
			//fbase *= (1 + fRandTime + fluck + fLockRate + fcombobuff + fstock);
			fbase *= (1 + fRandTime + fRank + fGameTime + fluck + fLockRate + fcombobuff + fstock + frp + fRecharge);
		}
		else
		{
	
			float fPool = PoolRate((int)nPoolGold / pPlayer->TableRate());		
			if (pPlayer->GetRoleExInfo() && pPlayer->GetRoleExInfo()->GetRoleMonth().IsInMonthTable())
			{
				fPool=0;
			}
			fbase *= (1 + fRandTime + fRank + fGameTime + fPool + fLockRate + fcombobuff + fstock + frp + fRecharge);

	/*		{
				char szKey[20] = { 0 };
				char szValue[128] = { 0 };
				char szFile[128] = { 0 };
				sprintf_s(szKey, sizeof(szKey), "%d",timeGetTime());

				sprintf_s(szValue, sizeof(szValue), "%d,rp%f,randtime%f,production%f,rank%f,gametime%f,money%f,Pool%f,base%f", fishType, frp, fRandTime, fProduction, fRank, fGameTime, fMoney, fPool, fbase);
				sprintf_s(szFile, sizeof(szFile), "%s\\%d.txt", g_FishServerConfig.GetFTPServerConfig()->FTPFilePath, pPlayer->GetRoleExInfo()->GetRoleInfo().dwUserID);
				WritePrivateProfileStringA("fish", szKey, szValue, szFile);
			}*/
		}
		byte byIndex = 2 * byOrder;			
		if (!pPlayer->HaveNormalLauncher(bulletType))//��ʱ��
		{
			byIndex += 1;
		}
		fbase *= m_Cannon[bulletType].vCollision[byIndex];
		fbase *= PackageFactor(byPackageType);

		CRoleEx* pRole = pPlayer->GetRoleExInfo();
		if (pRole)
		{
			fbase *= pRole->GetRoleVip().AddCatchFishRate();
		}
		fbase = min(fbase, Con_Cannon::s_finalratio);
		//fbase = 1.0f; // wm todo
		if (m_pTableManager->Isabhor(player))
		{
			fbase /= 2;
		}
		//std::cout << fbase << endl;
		return fbase;
	}
	return 0;
#endif
}


SkillFailedType  CConfig::UseSkill(PlayerID playerID, SkillType skill)
{
#if SIMPLE_SERVER_MODE
	return SFT_OK;
#else
	CRole *pUser = m_pTableManager->SearchUser(playerID);
	if (!pUser)
	{
		return SFT_INVALID;
	}

	if ((size_t)skill >= m_Skill.size())
	{
		return SFT_INVALID;
	}
	return pUser->UseSkill(skill);
#endif
}

LaunchFailedType  CConfig::UseLaser(PlayerID playerID, byte launcherType)
{
#if SIMPLE_SERVER_MODE
	return true;
#else
	ASSERT(launcherType < m_Energy.size());

	CRole *pUser = m_pTableManager->SearchUser(playerID);
	if (!pUser)
	{
		return LFT_INVALID;
	}
	return pUser->UseLaser(launcherType);
#endif
}

void  CConfig::GetSkillRange(SkillType skill, int &minNum, int &maxNum)
{
	ASSERT(static_cast<DWORD>(skill) < m_Skill.size());
	minNum = m_Skill[skill].nmincatch;
	maxNum = m_Skill[skill].nmaxcatch;
}

void  CConfig::GetLaserRange(byte laser, int &minNum, int &maxNum)
{
	ASSERT(laser<m_Energy.size());
	minNum = m_Energy[laser].nmincatch;
	maxNum = m_Energy[laser].nmaxcatch;
}

void  CConfig::GetSkillFreezeReduction(byte &speedScaling, byte &duration1, byte& duration2, byte &duration3)
{
	ASSERT(SKILL_FREEZE<m_Skill.size());
	speedScaling = ConvertFloatToBYTE(m_Skill[SKILL_FREEZE].npseed / SPEED_SCALE);
	duration1 = ConvertFloatToBYTE(m_Skill[SKILL_FREEZE].ntime1 / FISH_DURATION_TIME);
	duration2 = ConvertFloatToBYTE(m_Skill[SKILL_FREEZE].ntime2 / FISH_DURATION_TIME);
	duration3 = ConvertFloatToBYTE(m_Skill[SKILL_FREEZE].ntime3 / FISH_DURATION_TIME);
}

void  CConfig::GetSkillFrozenReduction(byte &speedScaling, byte &duration1, byte& duration2, byte &duration3)
{
	ASSERT(SKILL_FROZEN < m_Skill.size());
	speedScaling = ConvertFloatToBYTE(m_Skill[SKILL_FROZEN].npseed / SPEED_SCALE);
	duration1 = ConvertFloatToBYTE(m_Skill[SKILL_FROZEN].ntime1 / FISH_DURATION_TIME);
	duration2 = ConvertFloatToBYTE(m_Skill[SKILL_FROZEN].ntime2 / FISH_DURATION_TIME);
	duration3 = ConvertFloatToBYTE(m_Skill[SKILL_FROZEN].ntime3 / FISH_DURATION_TIME);
}

float CConfig::GetSkillChance(SkillType skill, USHORT fishIndex, BYTE byPackageType)
{
	ASSERT(ConvertIntToDWORD(skill)<m_Skill.size());
	ASSERT(fishIndex<m_Fish.size());
	return  1.0f*m_Skill[skill].nincome / (m_Fish[fishIndex].nvalue* m_Skill[skill].nmaxcatch)*PackageFactor(byPackageType);
}

float CConfig::GetLaserChance(byte launcherType, USHORT fishIndex, BYTE byPackageType)
{
	ASSERT(launcherType<m_Energy.size());
	ASSERT(fishIndex<m_Fish.size());
	return  1.0f*m_Energy[launcherType].nincome / (m_Fish[fishIndex].nvalue* m_Energy[launcherType].nmaxcatch) *PackageFactor(byPackageType);
}

float CConfig::GetLaserRadius(byte launcherType)
{
	ASSERT(launcherType<m_Energy.size());
	return m_Energy[launcherType].nradius * 1.0f;
}


int CConfig::GetBulletNature(USHORT launcherType)
{
	ASSERT(launcherType<m_Cannon.size());
	return m_Cannon[launcherType].nNature;
}

BulletType CConfig::GetBulletNatureType(USHORT bulletID)
{
	int res = GetBulletNature(bulletID);
	if (res >= BulletType::BULLET_TYPE_MAX)
	{
		res = BulletType::BULLET_TYPE_NORMAL;
	}
	return BulletType(res);
}

DWORD CConfig::GetBulletNatureIntervalTime(USHORT launcherType)
{
	size_t bty = CConfig::GetBulletNatureType(launcherType);
	ASSERT(bty<m_Nature.size());
	return m_Nature[bty].intervaltime;
}

int CConfig::GetBulletNatureShootcount(USHORT launcherType)
{
	size_t bty = CConfig::GetBulletNatureType(launcherType);
	ASSERT( bty<m_Nature.size());
	return m_Nature[bty].shootcount;
}
int CConfig::GetBulletNatureRebound(USHORT launcherType)
{
	size_t bty = CConfig::GetBulletNatureType(launcherType);
	ASSERT(bty<m_Nature.size());
	return m_Nature[bty].rebound;
}
int CConfig::GetBulletNatureLock(USHORT launcherType)
{
	size_t bty = CConfig::GetBulletNatureType(launcherType);
	ASSERT(bty<m_Nature.size());
	return m_Nature[bty].lock;
}

void CConfig::GetBulletFreezeReduction(byte &speedScaling, byte &duration1, byte& duration2, byte &duration3)
{
	// san hao pao shi bin dong  ?????
	ASSERT(CANNON_4<m_Cannon.size());
	speedScaling = ConvertFloatToBYTE(m_Cannon[CANNON_4].npseed / SPEED_SCALE);
	duration1 = ConvertFloatToBYTE(m_Cannon[CANNON_4].ntime1 / FISH_DURATION_TIME);
	duration2 = ConvertFloatToBYTE(m_Cannon[CANNON_4].ntime2 / FISH_DURATION_TIME);
	duration3 = ConvertFloatToBYTE(m_Cannon[CANNON_4].ntime3 / FISH_DURATION_TIME);
}

//
//notify 
//
UINT   CConfig::CatchFish(PlayerID playerID, USHORT fishIndx, CatchType catchType, byte subType, byte byPackageType, byte byRateIndex)
{
#if SIMPLE_SERVER_MODE
	return fishIndx;
#else
	ASSERT(fishIndx<m_Fish.size());

	CRole *pUser = m_pTableManager->SearchUser(playerID);
	if (pUser&&fishIndx<m_Fish.size())
	{
		int nIncome = m_Fish[fishIndx].nvalue;//bu kao lv beishu *pUser->
		int nMultiple = BulletMultiple(byRateIndex);
		if (catchType == CATCH_SKILL)//�������б���
		{
			nMultiple = SkillMultiple(subType);
		}
		pUser->OnCatchFish(catchType, subType, fishIndx, byPackageType, nIncome*nMultiple, nIncome);
		//LogInfoToFile("WmGoldLog.txt", "fishIndx=%d nIncome=%d  nMultiple=%d ", fishIndx, nIncome, nMultiple);
		//std::cout << "fishIndex=" << fishIndx << std::endl;
		//std::cout << "Income=" << nIncome << std::endl;
		//pUser->GetRoleExInfo()->ChangeRoleGlobe(nIncome*nMultiple, false);
		return nIncome*nMultiple;
	}
	return 0;
#endif
}


//
//notify 
//
UINT   CConfig::CatchFishGold(USHORT fishIndx, CatchType catchType, byte subType, byte byRateIndex)
{
#if SIMPLE_SERVER_MODE
	return fishIndx;
#else
	ASSERT(fishIndx < m_Fish.size());

	if (fishIndx < m_Fish.size())
	{
		int nIncome = m_Fish[fishIndx].nvalue;//bu kao lv beishu *pUser->
		int nMultiple = BulletMultiple(byRateIndex);
		if (catchType == CATCH_SKILL)//�������б���
		{
			nMultiple = SkillMultiple(subType);
		}
		return nIncome*nMultiple;
	}
	return 0;
#endif
}


USHORT    CConfig::GetCombo(PlayerID playerID, USHORT bulletID)
{
#if SIMPLE_SERVER_MODE
	return 0;
#else
	CRole *pUser = m_pTableManager->SearchUser(playerID);
	if (pUser)
	{
		return pUser->Combo(bulletID);
	}
	return 0;
#endif
}
bool CConfig::CanUploadImg(PlayerID playerID, USHORT imgSize)
{
	return true;
}
byte  CConfig::GetSkillCDTime(SkillType st)
{
	ASSERT(ConvertIntToDWORD(st)<m_Skill.size());
	return ConvertFloatToBYTE(m_Skill[st].ncdtime);
}

void  CConfig::GetLaserReduction(byte LauncherType, byte &speedScaling, byte &duration1, byte& duration2, byte &duration3)
{
	ASSERT(LauncherType<m_Energy.size());
	speedScaling = ConvertFloatToBYTE(m_Energy[LauncherType].npseed / SPEED_SCALE);
	duration1 = ConvertFloatToBYTE(m_Energy[LauncherType].ntime1 / FISH_DURATION_TIME);
	duration2 = ConvertFloatToBYTE(m_Energy[LauncherType].ntime2 / FISH_DURATION_TIME);
	duration3 = ConvertFloatToBYTE(m_Energy[LauncherType].ntime3 / FISH_DURATION_TIME);
}

byte  CConfig::GetBulletSpeed(byte LauncherType)
{
	ASSERT(LauncherType<m_Cannon.size());
	return ConvertIntToBYTE(m_Cannon[LauncherType].bulletspeed); //Con_Cannon::s_bulletspeed;
}

byte  CConfig::GetLauncherInterval(byte LauncherType)
{
	ASSERT(LauncherType<m_Cannon.size());
	return ConvertFloatToBYTE(m_Cannon[LauncherType].dlauncherinterval / FISH_DURATION_TIME);
}

byte  CConfig::GetLaserCDTime(byte LauncherType)
{
	ASSERT(LauncherType<m_Energy.size());
	return ConvertFloatToBYTE(m_Energy[LauncherType].ncdtime / FISH_DURATION_TIME);
}
UINT  CConfig::LaunchPackage(UINT fishid_packageid)
{
	return 1;
}
byte  CConfig::GetRateIndex(byte seat, PlayerID id)
{
#if SIMPLE_SERVER_MODE
	return 0;
#else
	CRole *pPlayer = m_pTableManager->SearchUser(id);
	if (pPlayer)
	{
		return pPlayer->GetRateIndex();
	}
	return 0;
#endif
}
void  CConfig::BulletGain(PlayerID id, byte BulletType, UINT goldNum)
{
#if SIMPLE_SERVER_MODE
#else
	CRole *pPlayer = m_pTableManager->SearchUser(id);
	if (pPlayer)
	{
		pPlayer->BulletGain(BulletType, goldNum);
	}
#endif
}

float CConfig::GameTimeDrop(int nTime)
{
	for (Playtime_Vec::reverse_iterator it = m_PlayTime.rbegin(); it != m_PlayTime.rend(); it++)
	{
		if (nTime >= it->ntime)
		{
			return it->drop;
		}
	}
	return 0;
}

bool CConfig::IsBossFish(BYTE id)
{
	if (id >= m_Fish.size())
	{
		ASSERT(false);
		return false;
	}
	return (m_Fish[id].isfishboss == 1);
}


CRoleEx*  CConfig::GetRole(PlayerID id)
{
	CRole *pPlayer = m_pTableManager->SearchUser(id);
	CRoleEx *pRoleEx = pPlayer->GetRoleExInfo();
	return pRoleEx;
}

USHORT CConfig::FishRewardDrop(PlayerID id, BYTE byPackageType,USHORT byType, bool& dropBullet, UINT goldNum)
{
	dropBullet = false;
	ASSERT(byType < m_Fish.size());
	CRole *pPlayer = m_pTableManager->SearchUser(id);
	ushort nReward = 0;
	if (pPlayer)
	{
		CRoleEx *pRoleEx = pPlayer->GetRoleExInfo();
		if (pRoleEx&&!pRoleEx->GetRoleMonth().IsInMonthTable())
		{
			HashMap<BYTE, tagChestConfig>::iterator it = g_FishServer.GetFishConfig().GetChestConfig().ChestMap.find(byPackageType);
			if (it != g_FishServer.GetFishConfig().GetChestConfig().ChestMap.end() && (it->second.ImmediateRewardid == 0 || g_FishServer.GetFishConfig().GetFishRewardConfig().RewardMap.count(it->second.ImmediateRewardid) > 0))
			{
				return 0;
			}			
			float fDropChance = m_Fish[byType].chance;//base	
			bool bBossFish = IsBossFish(byType);// m_Fish[byType].isfishboss == 1);//����boss��
			if (!bBossFish)
			{
				float fdropother = 1;
				fdropother += GameTimeDrop(pPlayer->GetGameTime());// GameTimeDrop
				fdropother += MoneyRate(pPlayer->GetProduction());//Produ			
				BYTE cbMaxRate = pRoleEx->GetRoleRate().GetCanUseMaxRate();
				if (cbMaxRate < m_Rate.size())
				{
					fdropother += m_Rate[cbMaxRate].drop;
				}
				fdropother += RankDrop(pPlayer->GetLevel());
				fDropChance *= fdropother;
			}
			
			//LogInfoToFile("WmDmq.txt", " playerID=%d   FishType=%d  catchChance=%f", id, byType, fDropChance);

			if (RandFloat() < fDropChance)
			{
				HashMap<BYTE, std::map<BYTE, tagFishDropOnce> >::iterator it = g_FishServer.GetFishConfig().GetFishDropConfig().FishDropMap.find(m_Fish[byType].type);
				if (it != g_FishServer.GetFishConfig().GetFishDropConfig().FishDropMap.end())
				{
					BYTE player_rate = pPlayer->GetRateIndex();
					std::map<BYTE, tagFishDropOnce>::iterator rit = it->second.lower_bound(player_rate);
					if(rit != it->second.end())
					{
						nReward = rit->second.GetFishDrop();// it->second.GetFishDrop();
						HashMap<WORD, tagRewardOnce>::iterator Iter = g_FishServer.GetFishConfig().GetFishRewardConfig().RewardMap.find(nReward);
						if (Iter != g_FishServer.GetFishConfig().GetFishRewardConfig().RewardMap.end())
						{
							if (bBossFish)
							{
								//bool Send = false;
								for (vector<tagItemOnce>::iterator it = Iter->second.RewardItemVec.begin(); it != Iter->second.RewardItemVec.end(); it++)
								{
									if (it->ItemID >= 900 && it->ItemID <= 903
										|| it->ItemID == pPlayer->GetRoleExInfo()->GetItemManager().GetSilverBulletID()
										|| it->ItemID == pPlayer->GetRoleExInfo()->GetItemManager().GetBronzeBulletID()
										)
									{
										dropBullet = true;
									}
									else if (it->ItemID == pPlayer->GetRoleExInfo()->GetItemManager().GetGoldBulletID())
									{
										g_FishServer.SendBroadCast(pPlayer->GetRoleExInfo(), NoticeType::NT_DropBullet, m_Fish[byType].name, it->ItemID, it->ItemSum);
										dropBullet = true;
									}
								}

							}
							pPlayer->AddDropReward(nReward);
							//return nReward;
						}
					}
				}
			}
			if (bBossFish && !dropBullet && goldNum > 0)
			{
				g_FishServer.SendBroadCast(pPlayer->GetRoleExInfo(), NoticeType::NT_DropGold, m_Fish[byType].name, 0, goldNum);
			}
		}
	}
	return nReward;
}


byte  CConfig::BulletConsume(byte LauncherType)
{
	ASSERT(LauncherType<m_Cannon.size());
	return ConvertIntToBYTE(m_Cannon[LauncherType].nconsume);
}

ushort  CConfig::BulletMultiple(byte byIndex)
{
	ASSERT(byIndex<m_Rate.size());
	return ConvertIntToWORD(m_Rate[byIndex].nValue);

}

float CConfig::CalBaseRatio(BYTE cbCannoIndex, BYTE cbFishIndex)
{
	ASSERT(cbCannoIndex<m_Cannon.size());
	ASSERT(cbFishIndex<m_Fish.size());

	float fratio = 1.0f*m_Cannon[cbCannoIndex].nconsume / m_Fish[cbFishIndex].nvalue*Con_Cannon::s_ratiofactor;
	if (fratio > Con_Cannon::s_maxratio)
	{
		fratio = Con_Cannon::s_maxratio;
	}

	if (cbCannoIndex < m_Fish[cbFishIndex].fishrate.size())
	{
		fratio *= m_Fish[cbFishIndex].fishrate[cbCannoIndex];
	}
	
	//fratio *= m_Fish[cbFishIndex].reviseratio;
	fratio *= Con_Fish::s_revies_Activity;
	//fratio *= 1;
	return fratio;
}
float CConfig::CalRandom(float d1, float d2)
{
	if (d1 == d2)
		return d1;
	return d1 + rand() % ((int)((d2 - d1)*ENLARGE)) / ENLARGE;
}



int CConfig::FishCount()
{
	return m_Fish.size();
}

int CConfig::CannonCount()
{
	return m_Cannon.size();
}

int CConfig::SkillCount()
{
	return m_Skill.size();
}

int CConfig::RankCount()
{
	return m_Rank.size();
}

int CConfig::RateCount()
{
	return m_Rate.size();
}

//���ݾ���õ��ȼ�
int CConfig::GetLevle(int nExperience)
{
	for (Rank_Vec::reverse_iterator it = m_Rank.rbegin(); it != m_Rank.rend(); it++)
	{
		if (nExperience >= it->experience)
		{
			return it->level;
		}
	}
	return 0;
}
//int CConfig::RandomCatchCycle()
//{
//	return Randomtime::s_cycle;
//}
int CConfig::RpCycle()
{
	return ConvertFloatToInt(Con_Rp::s_cycle);
}
int CConfig::RpEffectCycle()
{
	return ConvertFloatToInt(Con_Rp::s_effecttime);
}

//��ֵӰ�첶����
int CConfig::RechargeTimeSec(DWORD dwRecharge)
{
	std::map<DWORD, Con_Recharge>::iterator rit = m_Recharge.find(dwRecharge);
	if (rit != m_Recharge.end())
	{
		return rit->second.timesec;
	}
	return 0;
}

//��ֵӰ�첶����
float CConfig::RechargeRate(DWORD dwRecharge)
{
	std::map<DWORD, Con_Recharge>::iterator rit = m_Recharge.find(dwRecharge);
	if (rit != m_Recharge.end())
	{
		return rit->second.ratio;
	}
	return 0.0f;
}

//rpӰ�����
float CConfig::RpRate(float nRp)
{
	for (Rp_Vec::iterator it = m_Rp.begin(); it != m_Rp.end(); it++)
	{
		if (nRp < it->rp)
		{
			return CalRandom(it->reviseratio1, it->reviseratio2);
		}
	}
	return 0;
}
////���ʱ��Ӱ��
//float CConfig::RandomTimeRate(int nFishKind)
//{
//	return 0;
//}



//����Ӱ�����
float CConfig::ProductionRate(int nProducntion)
{
	//�Ӻ���ǰ��
	for (Production_Vec::reverse_iterator it = m_production.rbegin(); it != m_production.rend(); it++)
	{
		if (nProducntion >= it->nincome)
		{
			return CalRandom(it->reviseratio1, it->reviseratio2);
		}
	}
	return 0;
}

//�ȼ�Ӱ�����
float CConfig::RankRate(int nLevel)
{
	//�Ӻ���ǰ��
	for (Rank_Vec::reverse_iterator it = m_Rank.rbegin(); it != m_Rank.rend(); it++)
	{
		if (nLevel >= it->level)
		{			
			return it->reviseratio;
		}
	}
	return 0;
}

//�ȼ�Ӱ�����
float CConfig::RankDrop(int nLevel)
{
	//�Ӻ���ǰ��
	for (Rank_Vec::reverse_iterator it = m_Rank.rbegin(); it != m_Rank.rend(); it++)
	{
		if (nLevel >= it->level)
		{
			return it->drop;
		}
	}
	return 0;
}

//����ʱ��Ҳ���Ǹ�����,���һ�ε���
float CConfig::GameTimeRate(int nMinutes)
{
	//return
	for (Playtime_Vec::reverse_iterator it = m_PlayTime.rbegin(); it != m_PlayTime.rend(); it++)
	{
		if (nMinutes >= it->ntime)
		{
			return CalRandom(it->reviseratio1, it->reviseratio2);
		}
	}
	return 0;
}

float CConfig::MoneyRate(int nMoney)
{
	for (Money_Vec::reverse_iterator it = m_Money.rbegin(); it != m_Money.rend(); it++)
	{
		if (nMoney >= it->money)
		{
			return CalRandom(it->reviseratio1, it->reviseratio2);
		}
	}
	return 0;
}
float CConfig::LuckRate(int nLuck)
{
	for (Luck_Vec::reverse_iterator it = m_VceLuck.rbegin(); it!= m_VceLuck.rend(); it++)
	{
		if (nLuck > it->nluck)
		{
			return CalRandom(it->reviseratio1,it->reviseratio2);
		}
	}
	return 0;
}

float CConfig::RandomTimeRate(int& TimeSec)
{
	return m_RandomTime.GetRandomTimeRatio(TimeSec);
}

float CConfig::PoolRate(int nPool)
{
	for (Pool_Vec::iterator it = m_GoldPool.begin(); it != m_GoldPool.end();it++)
	{
		if (nPool < it->pool)
		{
			return CalRandom(it->reviseratio1, it->reviseratio2);
		}
	}
	return 0;
}


int  CConfig::LaserThreshold(byte byIndex)
{
	ASSERT(byIndex<m_Energy.size());
	return m_Energy[byIndex].nthreshold;
}

float  CConfig::LauncherInterval(byte byIndex)
{
	ASSERT(byIndex<m_Cannon.size());
	return m_Cannon[byIndex].dlauncherinterval;
}
float  CConfig::LaserCdTime(byte byIndex)
{
	ASSERT(byIndex<m_Energy.size());
	return m_Energy[byIndex].ncdtime;
}
float  CConfig::SkillCdTime(byte byIndex)
{
	ASSERT(byIndex<m_Skill.size());
	return m_Skill[byIndex].ncdtime;
}

float  CConfig::SkillLauncherIntervalTime(byte byIndex)
{
	ASSERT(byIndex < m_Skill.size());
	return m_Skill[byIndex].launcherinterval;
}

void CConfig::GoodsInfo(int nSkill, int norder, int &nGoodsid, int &nGoodsConsume)
{
	nGoodsid = m_Skill[nSkill].ngoodsid;

	if (m_Skill[nSkill].vecConsume.size() == 0)
	{
		nGoodsConsume = 0;
		return;
	}
	for (size_t i = 0; i < m_Skill[nSkill].vecConsume.size(); i++)
	{
		if (norder < m_Skill[nSkill].vecConsume[i].byorder)
		{
			nGoodsConsume = m_Skill[nSkill].vecConsume[i].byCount; //ngoodsconsume;
			return;
		}
	}

	nGoodsConsume = m_Skill[nSkill].vecConsume.back().byCount;//the last one
}
void CConfig::GoodsInfo(int nCannon, int &nGoodsid, int &nGoodsCount)
{
	ASSERT(ConvertIntToDWORD(nCannon)<m_Cannon.size());
	nGoodsid = m_Cannon[nCannon].nItemId;
	nGoodsCount = m_Cannon[nCannon].nItemCount;
}

bool CConfig::FindCannon(byte nSkill, int& nCannon)
{
	for (Cannon_Vec::iterator it = m_Cannon.begin(); it != m_Cannon.end(); it++)
	{
		if (it->nskill == nSkill)
		{
			nCannon = it->nid;
			return true;
		}
	}
	return false;
}

bool CConfig::LevelUp(WORD cbLevel, DWORD nExp, WORD& nNewLevel, DWORD&nNewExp)
{
	ASSERT(cbLevel<m_Rank.size());
	int nSize = m_Rank.size();
	nNewLevel = cbLevel;
	nNewExp = nExp;
	while (nNewLevel + 1 < (WORD)nSize && (nNewExp >= (DWORD)(m_Rank[nNewLevel + 1].experience - m_Rank[nNewLevel].experience)))
	{
		nNewExp -= (DWORD)(m_Rank[nNewLevel + 1].experience - m_Rank[nNewLevel].experience);
		nNewLevel += 1;
	}
	return nNewLevel != cbLevel;
}
float CConfig::ComboSustainTime()
{
	return Con_Combo::s_sustaintime* 1.0f;
}
int CConfig::ComboBuffCycle()
{
	return Con_Combo::s_buff_cycle;
}

float CConfig::PackageFactor(BYTE cbPackage)
{	
	HashMap<BYTE, tagChestConfig>::iterator it=g_FishServer.GetFishConfig().GetChestConfig().ChestMap.find(cbPackage);
	if (it == g_FishServer.GetFishConfig().GetChestConfig().ChestMap.end())
	{
		//ASSERT(false);
		return 1.0f;
	}
	if (it->second.CatchChance==0)
	{
		//ASSERT(false);
		return 1.0f;
	}
	return 1.0f / it->second.CatchChance;
}
USHORT CConfig::RateUnlock(BYTE byIndex)
{
	ASSERT(byIndex<m_Rate.size());
	return m_Rate[byIndex].nUnlock;
}

int CConfig::RateMaterial(BYTE byIndex)
{
	ASSERT(byIndex < m_Rate.size());
	return m_Rate[byIndex].material;
}

vector<int>& CConfig::GetRateMaterialIDs()
{
	return Con_Rate::material_ids;
}

int CConfig::GetRateRoughID()
{
	return Con_Rate::rough_id;
}

int CConfig::GetRateQianPaoID()
{
	return Con_Rate::qianpao_id;
}

__int64 CConfig::GetStaticStockScore(int type)
{
	if (type > MAX_TABLE_TYPE)
	{
		return 0;
	}
	__int64 score = __int64(Con_StockItem::s_stockscore[type]);
	return score >0 ? score : 0;
}

float CConfig::GetStockTax(int type)
{
	if (type > MAX_TABLE_TYPE)
	{
		return 0.0f;
	}
	return  Con_StockItem::s_tax[type];;
}

float CConfig::StockRate(int type, __int64 stockscore)
{
	Stock_Map::iterator rit = m_Stock.find(type);
	if (rit != m_Stock.end())
	{
		StockType_Map::iterator trit = rit->second.lower_bound(stockscore);
		if (trit != rit->second.end())
		{
			return CalRandom(trit->second.reviseratio1, trit->second.reviseratio2);
		}
		
	}
	return 0.0f;
}

void CConfig::AddStaticStockScore(int type, float score)
{
	if (type > MAX_TABLE_TYPE /*|| score <= 0*/)
	{
		return;
	}
	Con_StockItem::s_stockscore[type] += score;
//#ifdef _DEBUG
//	std::cout << type << ":"<<Con_StockItem::s_stockscore[type] << endl;
//#endif
}

void CConfig::AddTaxStockScore(int type, float score)
{
	if (type > MAX_TABLE_TYPE /*|| score <= 0*/)
	{
		return;
	}
	Con_StockItem::s_taxscore[type] += score;
	//#ifdef _DEBUG
	//	std::cout << type << ":"<<Con_StockItem::s_stockscore[type] << endl;
	//#endif
}

int CConfig::RateRough(BYTE byIndex)
{
	ASSERT(byIndex < m_Rate.size());
	return m_Rate[byIndex].rough;
}

float CConfig::RateSuccessRate(BYTE byIndex)
{
	ASSERT(byIndex < m_Rate.size());
	return m_Rate[byIndex].success_rate;
}

float CConfig::RateMatchAddScore(BYTE byIndex)
{
	ASSERT(byIndex < m_Rate.size());
	return m_Rate[byIndex].match_add_score;
}

ushort CConfig::SkillMultiple(byte byIndex)
{
	ASSERT(byIndex<m_Skill.size());
	return m_Skill[byIndex].multiple;
}
USHORT CConfig::UnlockRateReward(BYTE byIndex)
{
	if (byIndex < m_Rate.size())
	{
		return m_Rate[byIndex].unlockreward;
	}
	return 0;
}

bool CConfig::IsLightingFish(USHORT fishIndex)
{
	return fishIndex == Con_Fish::s_flashfishid;
}
int  CConfig::IsLightingFish(USHORT fishIndex, PlayerID id)
{
	if (IsLightingFish(fishIndex))
	{
		return Con_Fish::s_flashfishCatch;
	}
	return false;
	//����������㣬���ش���0�Ĳ�������
	//return (fishIndex < 8) ? 5 : 0;
}
//ĳһ�����Ƿ���Ա����粶��fishIndex��ID�����ID��dist:��Ļ���롣
bool CConfig::CanCatchLightingFish(USHORT fishIndex, PlayerID id, float dist)
{
	if (fishIndex >= m_Fish.size())
	{
		return false;		
	}
	return dist<Con_Fish::s_flashfishradius&&RandFloat()<m_Fish[fishIndex].flashchance;
	//return RandRange(0, 10) > 7 && dist < 25;
}
BYTE CConfig::CannonOwnSkill(BYTE byIndex)
{
	if (byIndex < m_Cannon.size())
	{
		return m_Cannon[byIndex].nskill;
	}
	return 255;
}