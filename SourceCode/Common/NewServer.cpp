#include "stdafx.h"
#include "NewServer.h"
static const int SOCKADDR_SIZE_16 = sizeof(SOCKADDR_IN)+16;
static GUID GuidAcceptEx = WSAID_ACCEPTEX;
static GUID GuidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;
#define THREAD_ACCEPT_COUNT		256			//单个线程接收的容器数量
#define ACCEPT_CLIENT_COUNT		256			//ACCEPT同时接收的最大数量
#define HEARBEAT_TICK			2000
#define ACCEPT_WAIT_TIMEOUT		3000		//连接验证超时
#define USE_PING_TEST			0			//开启测速
#define UDP_MTU					512			//UDP每个包的最大传输长度
#define UDP_SEND_INTERVAL		10			//UDP间隔多长时间发送一个包
#define UDP_BUFF_SIZE			1024
UINT CONNECT_OK = SERVER_CONNECT_MAGIC;


UINT WINAPI ThreadRecvTCP(void *p)
{
	((NewServer*)p)->_ThreadRecvTCP();
	return 0;
}

UINT WINAPI ThreadSendTCP(void *p)
{
	((NewServer*)p)->_ThreadSendTCP();
	return 0;
}
UINT WINAPI ThreadAccept(void *p)
{
	((NewServer*)p)->_ThreadAccept();
	return 0;
}
NewServer::NewServer() :m_NewClientList(ACCEPT_CLIENT_COUNT)
{
	m_pHandler = NULL;
}
NewServer::~NewServer()
{

}

bool NewServer::Init(const ServerInitData &data, bool bTCP)
{
	m_bAccept = true;
	memset(m_RecvThreadData, 0, sizeof(m_RecvThreadData));
	memset(m_SendThreadData, 0, sizeof(m_SendThreadData));
	m_ExitIndex = 0;
	m_OnlineNum = 0;
	m_RecvIndex = 0;
	m_SendIndex = 0;
	m_SendDataIndex = 0;
	m_bRun		= true;
	memcpy(&m_InitData, &data, sizeof(data));
	
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		Log("WSAStartup Failed.");
		return false;
	}

	CreateSocketData csd;
	bool bret = CreateSocket(CST_TCP | CST_BIND, data.Port, data.SocketRecvSize, data.SocketSendSize, csd);
	if (!bret)
	{
		Log("CreateSocket Failed.");
		return false;
	}
	m_Socket = csd.Socket;
	int nRet = ::listen(m_Socket, data.ListenCount);
	if (nRet != 0)
	{
		Log("listen Failed:%d.", WSAGetLastError());
		return false;
	}

	//创建线程数据
	for (int i = 0; i < data.RecvThreadNum; ++i)
	{
		m_RecvThreadData[i] = new RecvThreadData(THREAD_ACCEPT_COUNT);
		m_RecvThreadData[i]->OnlineNum = 0;
	}
	for (int i = 0; i < data.SendThreadNum; ++i)
	{
		m_SendThreadData[i] = new SendThreadData(THREAD_ACCEPT_COUNT);
		m_SendThreadData[i]->OnlineNum = 0;
	}
	
	//创建线程
	::_beginthreadex(0, 0, ThreadAccept, this, 0, 0);
	for (int i = 0; i < data.RecvThreadNum; ++i)
	{
		::_beginthreadex(0, 0, ThreadRecvTCP, this, 0, 0);
	}
	for (int i = 0; i < data.SendThreadNum; ++i)
	{
		::_beginthreadex(0, 0, ThreadSendTCP, this, 0, 0);
	}
	//ClientIO *pio = new ClientIO;
	//PostAccept(pio);
	Log("%d号服务已启动(SendThreadNum:%d, RecvThreadNum:%d, SendBuff:%d, RecvBuff:%d, SendCmdCount:%d, RecvCmdCount:%d, BuffSize:%d, SceneTick:%d, Timeout:%d, Valid:%d)",
		m_InitData.ServerID,
		m_InitData.SendThreadNum,
		m_InitData.RecvThreadNum,
		m_InitData.SocketSendSize,
		m_InitData.SocketRecvSize,
		m_InitData.MaxSendCmdCount,
		m_InitData.MaxRecvCmdCount,
		m_InitData.BuffSize,
		m_InitData.SceneHearbeatTick,
		m_InitData.Timeout,
		m_InitData.AcceptRecvData
		);

	return true;
}

void NewServer::GetSendAndRecvIndex(ushort &sendIdx, ushort &recvIdx)
{
	recvIdx = USHRT_MAX;
	int num = USHRT_MAX;
	for (WORD i = 0; i < m_InitData.RecvThreadNum; ++i)
	{
		if (m_RecvThreadData[i]->NewClientList.HasSpace() && m_RecvThreadData[i]->OnlineNum < num)
		{
			num = m_RecvThreadData[i]->OnlineNum;
			recvIdx = i;
		}
	}
	sendIdx = USHRT_MAX;
	num = USHRT_MAX;
	for (WORD i = 0; i < m_InitData.SendThreadNum; ++i)
	{
		if (m_SendThreadData[i]->NewClientList.HasSpace() && m_SendThreadData[i]->OnlineNum < num)
		{
			num = m_SendThreadData[i]->OnlineNum;
			sendIdx = i;
		}
	}
}
void NewServer::GetRecvIndex(ushort &recvIdx)
{
	recvIdx = USHRT_MAX;
	int num = USHRT_MAX;
	for (WORD i = 0; i < m_InitData.RecvThreadNum; ++i)
	{
		if (m_RecvThreadData[i]->NewClientList.HasSpace() && m_RecvThreadData[i]->OnlineNum < num)
		{
			num = m_RecvThreadData[i]->OnlineNum;
			recvIdx = i;
		}
	}
}
bool NewServer::RecvDataByTCP(ClientData *pc, int nSize)
{
	pc->RecvSize += nSize;
	int count = 0;
	while (pc->RecvSize >= sizeof(UINT))
	{
		++count;
		if (count > 1000)
		{
			Log("****RecvDataByTCP Loop Count:%d, recvSie:%d****", count, pc->RecvSize);
			pc->RemoveCode = REMOVE_RECVBACK_NOT_SPACE;
			pc->Removed = true;
			break;
		}
		/*else if (count > 10)
		{
			Log("****RecvDataByTCP Loop Count:%d, recvSie:%d****", count, pc->RecvSize);
		}*/

		char *pBuff = pc->Buff + pc->Offset;
		UINT recvID = *((UINT*)(pBuff));
		if (recvID == HEARBEAT_ID)
		{
			pc->Offset += sizeof(UINT);
			pc->RecvSize -= sizeof(UINT);
		}
		else if (recvID == PING_ID)
		{
			pc->Offset += sizeof(UINT);
			pc->RecvSize -= sizeof(UINT);
		}
		else if (pc->RecvSize >= sizeof(NetCmd))
		{
			NetCmd *pCmdRecv = (NetCmd*)pBuff;
			UINT cmdSize = pCmdRecv->GetCmdSize();
			printf("recv tcp cmdSize=%d\n", cmdSize);
			if (cmdSize == 0)
			{
				ASSERT(false);
			}
			if (cmdSize > m_InitData.BuffSize)
			{
#if 0  //wm todo
				pc->RemoveCode = REMOVE_CMD_SIZE_ERROR;
				pc->Removed = true;
				return false;
#endif
			}
			if (pc->RecvSize >= cmdSize)
			{
				if (pc->RecvList.HasSpace())
				{
					NetCmd *pCmd = CreateCmd(static_cast<WORD>(cmdSize), pBuff);
					pc->RecvList.AddItem(pCmd);
					pc->Offset += cmdSize;
					pc->RecvSize -= cmdSize;

					printf("type=%d:%d\n", pCmd->CmdType,pCmd->SubCmdType);
				}
				else
				{
					pc->RemoveCode = REMOVE_CMD_RECV_OVERFLOW;
					pc->Removed = true;
					return false;
				}
			}
			else
				break;
		}
		else
			break;
	}

	UINT freeBuffSize = m_InitData.BuffSize - (pc->Offset + pc->RecvSize);
	if (freeBuffSize < 128)
	{
		if (pc->RecvSize > 0)
			memmove(pc->Buff, pc->Buff + pc->Offset, pc->RecvSize);
		pc->Offset = 0;
	}
	return true;
}

enum WS_Status
{
	WS_STATUS_CONNECT = 0,
	WS_STATUS_UNCONNECT = 1,
};

enum WS_FrameType
{
	WS_EMPTY_FRAME = 0xF0,
	WS_ERROR_FRAME = 0xF1,
	WS_TEXT_FRAME = 0x01,
	WS_BINARY_FRAME = 0x02,
	WS_PING_FRAME = 0x09,
	WS_PONG_FRAME = 0x0A,
	WS_OPENING_FRAME = 0xF3,
	WS_CLOSING_FRAME = 0x08
};

int wsDecodeFrame(string inFrame, string &outMessage)
{
	int ret = WS_OPENING_FRAME;
	const char *frameData = inFrame.c_str();
	const int frameLength = inFrame.size();
	if (frameLength < 2)
	{
		ret = WS_ERROR_FRAME;
	}

	// 检查扩展位并忽略  
	if ((frameData[0] & 0x70) != 0x0)
	{
		ret = WS_ERROR_FRAME;
	}

	// fin位: 为1表示已接收完整报文, 为0表示继续监听后续报文  
	ret = (frameData[0] & 0x80);
	if ((frameData[0] & 0x80) != 0x80)
	{
		ret = WS_ERROR_FRAME;
	}

	// mask位, 为1表示数据被加密  
	if ((frameData[1] & 0x80) != 0x80)
	{
		ret = WS_ERROR_FRAME;
	}

	// 操作码  
	uint16_t payloadLength = 0;
	uint8_t payloadFieldExtraBytes = 0;
	uint8_t opcode = static_cast<uint8_t >(frameData[0] & 0x0f);
	if (opcode == WS_TEXT_FRAME)
	{
		// 处理utf-8编码的文本帧  
		payloadLength = static_cast<uint16_t >(frameData[1] & 0x7f);
		if (payloadLength == 0x7e)
		{
			uint16_t payloadLength16b = 0;
			payloadFieldExtraBytes = 2;
			memcpy(&payloadLength16b, &frameData[2], payloadFieldExtraBytes);
			payloadLength = ntohs(payloadLength16b);
		}
		else if (payloadLength == 0x7f)
		{
			// 数据过长,暂不支持  
			ret = WS_ERROR_FRAME;
		}
	}
	else if (opcode == WS_BINARY_FRAME || opcode == WS_PING_FRAME || opcode == WS_PONG_FRAME)
	{
		// 二进制/ping/pong帧暂不处理  
	}
	else if (opcode == WS_CLOSING_FRAME)
	{
		ret = WS_CLOSING_FRAME;
	}
	else
	{
		ret = WS_ERROR_FRAME;
	}

	// 数据解码  
	if ((ret != WS_ERROR_FRAME) && (payloadLength > 0))
	{
		// header: 2字节, masking key: 4字节  
		const char *maskingKey = &frameData[2 + payloadFieldExtraBytes];
		char *payloadData = new char[payloadLength + 1];
		memset(payloadData, 0, payloadLength + 1);
		memcpy(payloadData, &frameData[2 + payloadFieldExtraBytes + 4], payloadLength);
		for (int i = 0; i < payloadLength; i++)
		{
			payloadData[i] = payloadData[i] ^ maskingKey[i % 4];
		}

		outMessage = payloadData;
		delete[] payloadData;
	}

	return ret;
}

int wsEncodeFrame(string inMessage, string &outFrame, enum WS_FrameType frameType)
{
	int ret = WS_EMPTY_FRAME;
	const uint32_t messageLength = inMessage.size();
	if (messageLength > 32767)
	{
		// 暂不支持这么长的数据
		return WS_ERROR_FRAME;
	}

	uint8_t payloadFieldExtraBytes = (messageLength <= 0x7d) ? 0 : 2;
	// header: 2字节, mask位设置为0(不加密), 则后面的masking key无须填写, 省略4字节
	uint8_t frameHeaderSize = 2 + payloadFieldExtraBytes;
	uint8_t *frameHeader = new uint8_t[frameHeaderSize];
	memset(frameHeader, 0, frameHeaderSize);
	// fin位为1, 扩展位为0, 操作位为frameType
	frameHeader[0] = static_cast<uint8_t>(0x80 | frameType);

	// 填充数据长度
	if (messageLength <= 0x7d)
	{
		frameHeader[1] = static_cast<uint8_t>(messageLength);
	}
	else
	{
		frameHeader[1] = 0x7e;
		uint16_t len = htons(messageLength);
		memcpy(&frameHeader[2], &len, payloadFieldExtraBytes);
	}

	// 填充数据
	uint32_t frameSize = frameHeaderSize + messageLength;
	char *frame = new char[frameSize + 1];
	memcpy(frame, frameHeader, frameHeaderSize);
	memcpy(frame + frameHeaderSize, inMessage.c_str(), messageLength);
	frame[frameSize] = '\0';
	outFrame = frame;

	delete[] frame;
	delete[] frameHeader;
	return ret;
}
void NewServer::_ThreadRecvTCP()
{
	int idx = ::InterlockedIncrement(&m_RecvIndex) - 1;
	vector<ClientData*> clientList;
	RecvThreadData *pRecvData = m_RecvThreadData[idx];
	fd_set  *pSet = CreateFDSet();
	timeval time = { 0, 0 };
	while (m_bRun)
	{
		//1.接收新客户端
		//-----------------------------------------------------
		UINT tick = timeGetTime();
		while (pRecvData->NewClientList.HasItem())
		{
			ClientData *pc = pRecvData->NewClientList.GetItem();
			pc->RecvTick = tick;
			clientList.push_back(pc);
			printf("\n recv add:%d\n", pc->Socket);
		}

		//2.检查状态
		//-----------------------------------------------------
		FD_ZERO(pSet);
		for (UINT i = 0; i < clientList.size();)
		{
			ClientData *pc = clientList[i];
			bool bTimeOut = false;// wm todo int(tick - pc->RecvTick) > m_InitData.Timeout;
			if (pc->Removed || bTimeOut)
			{
				RemoveClient(pc, REMOVE_TIMEOUT);
				ListRemoveAt(clientList, i);
				::InterlockedDecrement(&pRecvData->OnlineNum);
				continue;
			}
			FD_ADD(pc->Socket, pSet);
			++i;
		}

		//3.接收数据
		//-----------------------------------------------------
		if (FD_COUNT(pSet) == 0)
			goto SLEEP;
		int nRet = select(0, pSet, NULL, NULL, &time);
		if (nRet == 0)
			goto SLEEP;
		printf("\n recv ret:%d\n", nRet);
		tick = timeGetTime();
		for (uint i = 0; i < clientList.size(); ++i)
		{
			ClientData *pc = clientList[i];
			if (!FD_ISSET(pc->Socket, pSet))
				continue;

			int curPos = pc->Offset + pc->RecvSize;
			int nSize = recv(pc->Socket, (char*)pc->Buff + curPos, m_InitData.BuffSize - curPos, 0);
			printf("\n recv socket=%d nSize:%d\n",pc->Socket, nSize);
			if (nSize == 0 || (nSize == SOCKET_ERROR && (WSAGetLastError() == WSAECONNRESET || WSAGetLastError() == WSAECONNABORTED)))
			{
				if (m_InitData.BuffSize - curPos == 0)
				{
					Log("接收缓冲区已满!buffSize:%d, curSize:%d", m_InitData.BuffSize, curPos);
				}
				pc->Removed = true;
			}
			else
			{
				string in = (char*)pc->Buff;
				string out;
				int ret = wsDecodeFrame(in,out);
				RecvDataByTCP(pc, nSize);


				pc->RecvTick = tick;
			}
		}// end for
SLEEP:
		Sleep(m_InitData.SleepTime);
	}
	DeleteFDSet(pSet);
	::InterlockedIncrement(&m_ExitIndex);
}

void NewServer::_ThreadSendTCP()
{
	int idx = ::InterlockedIncrement(&m_SendIndex) - 1;
	SendThreadData *pSendData = m_SendThreadData[idx];
	vector<ClientData*> clientList;
	//int scenetick = m_InitData.SceneHearbeatTick;
	int	halfTimeout = min(HEARBEAT_TICK, m_InitData.Timeout >> 1);
	UINT hearbeat = HEARBEAT_ID;
	//UINT ping = PING_ID;
	NetCmd hearbeatCmd;
	hearbeatCmd.SetCmdSize(sizeof(NetCmd));

	hearbeatCmd.SetCmdType(m_InitData.CmdHearbeat);
	//hearbeatCmd.SetCmdType(3);
	while (m_bRun)
	{
		UINT tick = timeGetTime();
		while (pSendData->NewClientList.HasItem())
			clientList.push_back(pSendData->NewClientList.GetItem());

		for (UINT i = 0; i < clientList.size(); )
		{
			ClientData *pc = clientList[i];
			if (pc->Removed)
			{
				RemoveClient(pc, REMOVE_NORMAL);
				ListRemoveAt(clientList, i);
				::InterlockedDecrement(&pSendData->OnlineNum);
				continue;
			}
			UINT k = 0;
			while (k < m_InitData.MaxSendCountPerFrame && pc->SendList.HasItem())
			{
				NetCmd *pcmd = pc->SendList.GetItemNoRemove();
				
				int ret = send(pc->Socket, (char*)pcmd, pcmd->GetCmdSize(), 0);
				printf("send ret: %d", ret);
				if (pcmd->GetCmdSize() == 13)
				{
				
					for (int i = 0; i < 13; i++)
					{
						//printf(" %2x", (char *) ((void *) pcmd [i]) );
						printf(" %2x");
					}
					printf("\n");
				}
				if (ret == pcmd->GetCmdSize())
				{
					free(pc->SendList.GetItem());
					pc->SendTick = tick;
					pc->SendError = 0;
				}
				else
				{
					if (++pc->SendError >= 10)
					{
						//发送失败，对方缓冲区已满;
						Log("发送缓冲区已满!CmdSize:%d, LastCode:%d", pcmd->GetCmdSize(), WSAGetLastError());
						pc->Removed = true;
					}
					break;
				}
				++k;
			}
			if (k == 0)
			{
#if 0
				//心跳
				if (int(tick - pc->SendTick) > halfTimeout)
				{
				//	hearbeatCmd.SubCmdType = 1;
					//Log("SendHearbeat:%d, %d", scenetick , halfTimeout);
					int ret = send(pc->Socket, (char*)&hearbeatCmd, hearbeatCmd.GetCmdSize(), 0);
					printf("2: %d", ret);
					if (ret == hearbeatCmd.GetCmdSize())
						 pc->SendTick = tick;
				}
#endif
			}
			++i;
		}
		Sleep(m_InitData.SleepTime);
	}
	::InterlockedIncrement(&m_ExitIndex);
}
void NewServer::AddNewClient(const AcceptClientData &acd)
{
	// 寻找适合的线程
	ushort recvIdx =0, sendIdx=0;
	GetSendAndRecvIndex(sendIdx, recvIdx);
	if (recvIdx == USHRT_MAX || sendIdx == USHRT_MAX)
	{
		Log("没有适合的空间加入新玩家:%d, %d", sendIdx, recvIdx);
		closesocket(acd.Socket);
		return;
	}

	//成功
	UINT size  = sizeof(ClientData) + m_InitData.BuffSize;
	ClientData *pc = new(malloc(size))ClientData(m_InitData.MaxSendCmdCount, m_InitData.MaxRecvCmdCount);
	pc->Removed = false;
	pc->RemoveCode = REMOVE_NONE;
	pc->IsInScene = false;
	pc->Socket = acd.Socket;
	pc->IP = acd.IP;
	pc->Port = acd.Port;
	pc->OutsideExtraData = NULL;
	pc->RefCount = 3;
	pc->SendTick = 0;
	pc->SendCmdTick = 0;
	pc->Offset = 0;
	pc->RecvSize = 0;
	pc->SendError = 0;
	m_pHandler->NewClient(m_InitData.ServerID, pc, (void*)acd.Buff, acd.RecvSize);

	RecvThreadData *precv = m_RecvThreadData[recvIdx];
	SendThreadData *psend = m_SendThreadData[sendIdx];
	::InterlockedIncrement(&m_OnlineNum);
	::InterlockedIncrement(&precv->OnlineNum);
	::InterlockedIncrement(&psend->OnlineNum);
	precv->NewClientList.AddItem(pc);
	psend->NewClientList.AddItem(pc);
	
}
bool SendFirstDataToUDPClient(AcceptClientData &acd, int recvbuff, int sendbuff)
{
	//发送新的端口到客户端
	CreateSocketData csd;
	if (!CreateSocket(CST_UDP | CST_BIND, 0, (uint)recvbuff, sendbuff, csd))
		return false;
	UINT sendData[3];
	UINT randID = RandUInt();
	sendData[0] = CONNECT_OK;
	sendData[1] = randID;
	sendData[2] = csd.Port;

	int ret = send(acd.Socket, (char*)sendData, sizeof(sendData), 0);
	if (ret != sizeof(sendData))
	{
		Log("SendFirstDataToUDPClient FAILED.");
		return false;
	}
	closesocket(acd.Socket);
	acd.Socket = csd.Socket;
	acd.RandID = randID;
	acd.Tick = timeGetTime();
	return true;
}
bool NewServer::CheckNewClient(AcceptClientData &data)
{
	printf("\n serverID:%d\n", g_ServerID);
	char resData[512] = {0};
	//外部进行验证
	UINT ret = m_pHandler->CanConnected(m_InitData.ServerID, data.IP, data.Port, (void*)data.Buff, data.RecvSize, resData);
	if (ret == CONNECT_WEB_SOCKET)
	{
		int ret = send(data.Socket, resData, strlen(resData), 0);
		printf("\nstrlen:%d\n\nres:%d\n%s\n",strlen(resData), ret,resData);
		//wm  todo if (ret != strlen((char*)data.Buff))
			return true;
	}
	else
	{
		if (ret == CONNECT_CHECK_FAILED)
		{
			//关闭
			return false;
		}
		else if (ret > 0)
		{
			//发送错误数据再关闭
			send(data.Socket, (char*)data.Buff, ret, 0);
			return false;
		}
		else
		{
			int ret = send(data.Socket, (char*)&CONNECT_OK, sizeof(CONNECT_OK), 0);
			printf("accept send ret: %d", ret);
			if (ret != sizeof(CONNECT_OK))
				return false;
			else
				return true;
		}
	}

}

void NewServer::_ThreadAccept()
{
	const int sleepTime = 10;
	typedef list<AcceptClientData> AcceptList;
	vector<AcceptClientData> clientList;
	SOCKADDR_IN addr;
	int addrSize = sizeof(addr);
	fd_set  *pSet = CreateFDSet();
	timeval time = { 0, 0 };
	AcceptList::iterator it;
	UINT hearbeat = HEARBEAT_ID;
	while (m_bRun)
	{
		UINT addCount = 0;
		UINT tick = timeGetTime();
		while (addCount++ < m_InitData.MaxAcceptNumPerFrame)
		{
			SOCKET socket = accept(m_Socket, (sockaddr*)&addr, &addrSize);
			if (socket == INVALID_SOCKET)
				break;

			AcceptClientData acd;
			UINT ip			= addr.sin_addr.S_un.S_addr;
			ushort port		= ntohs(addr.sin_port);
			acd.IP			= ip;
			acd.Port		= port;
			acd.Socket		= socket;
			acd.RecvSize	= 0;
			acd.Tick		= tick;
			InitSocket(socket, m_InitData.SocketSendSize, m_InitData.SocketRecvSize, true);

			if (m_InitData.AcceptRecvData)
			{
				//接收数据。
				acd.WaitType = WAIT_0;
			}
			else
			{
				AddNewClient(acd);
				continue;
			}
			clientList.push_back(acd);
		}
		FD_ZERO(pSet);
		tick = timeGetTime();
		for (uint i = 0; i < clientList.size();)
		{
			AcceptClientData &acd = clientList[i];
			if (acd.Socket == NULL /*|| tick - acd.Tick > ACCEPT_WAIT_TIMEOUT*/) //wm todo
			{
				if (acd.Socket != NULL)
					closesocket(acd.Socket);
				ListRemoveAt(clientList, i);
				if (acd.Socket != NULL)
				{
					char xx[100];
					GetIPString(acd.IP, acd.Port, xx);
					Log("TCP Client recv timeout, level:%d, ip:%s", acd.WaitType, xx);
				}
				printf("\n accept remove:%d\n", acd.Socket);
				continue;
			}
			FD_ADD(acd.Socket, pSet);
			++i;
		}

		if (FD_COUNT(pSet) == 0)
			goto EXIT;

		int nRet = select(0, pSet, NULL, NULL, &time);
		tick = timeGetTime();
		for (UINT i = 0; i < clientList.size(); ++i)
		{
			AcceptClientData &acd = clientList[i];
			SOCKET s = acd.Socket;
			if (!FD_ISSET(s, pSet))
				continue;
			int ret = recv(s, acd.Buff, sizeof(acd.Buff), 0);
			if (ret == 0 || (ret == SOCKET_ERROR && (WSAGetLastError() == WSAECONNRESET || WSAGetLastError() == WSAECONNABORTED)))
			{
				closesocket(acd.Socket);
				acd.Socket = NULL;
				continue;
			}
			acd.Buff[ret] = 0;
			printf("\n accept recv:socket=%d\n%s\n", acd.Socket,acd.Buff);
			acd.RecvSize += ret;
			if (CheckNewClient(acd) == false)
			{
				Log("验证失败.");
				closesocket(acd.Socket);
				closesocket(acd.Socket);
				acd.Socket = NULL;
				continue;
			}
			AddNewClient(acd);
			acd.Socket = NULL;
		}
	EXIT:
		Sleep(sleepTime);
		/*static UINT k = 0; 
		if (tick - k > 2000)
		{
			k = tick;
			if (curNum != m_OnlineNum)
			{
				curNum = m_OnlineNum;
				Log("Onlien:%d", m_OnlineNum);
			}
		}*/
	}
	DeleteFDSet(pSet);
	::InterlockedIncrement(&m_ExitIndex);
}
bool NewServer::Kick(ServerClientData *pClient, RemoveType rt)
{
	RemoveClient((ClientData*)pClient, rt);
	return true;
}
void NewServer::Shutdown()
{
	m_bRun = false;
	//PostQueuedCompletionStatus(m_Handle, 0, NULL, NULL);

	int count = m_InitData.RecvThreadNum + m_InitData.SendThreadNum + 1;
	while (m_ExitIndex != count)
		Sleep(100);
}
void NewServer::SetCmdHandler(INetHandler *pHandler)
{
	m_pHandler = pHandler;
}
bool NewServer::Send(ServerClientData *pClient, NetCmd *pCmd)
{
	ClientData *pc = (ClientData*)pClient;
	if (pc->SendList.HasSpace() == false)
	{
		pc->Removed = true;
		pc->RemoveCode = REMOVE_CMD_SEND_OVERFLOW;
		return false;
	}
	NetCmd *pNewCmd = CreateCmd(pCmd->GetCmdType(), pCmd->GetCmdSize());
	if (!pNewCmd)
		return false;
	memcpy_s(pNewCmd, pCmd->GetCmdSize(), pCmd, pCmd->GetCmdSize());
	pc->SendList.AddItem(pNewCmd);
	
	return true;
}
UINT NewServer::JoinNum()const
{
	return m_OnlineNum;
}
void NewServer::SwitchAccept(bool bEnable)
{
	if (m_bAccept != bEnable)
	{
		m_bAccept = bEnable;
	}
}
void NewServer::RemoveClient(ClientData *pc, RemoveType rt)
{
	pc->Removed = true;
	if (pc->RemoveCode == REMOVE_NONE)
		pc->RemoveCode = rt;

	if (::InterlockedDecrement(&pc->RefCount) == 0)
	{
		closesocket(pc->Socket);
		m_pHandler->Disconnect(m_InitData.ServerID, pc, pc->RemoveCode);
		while (pc->SendList.HasItem())
			free(pc->SendList.GetItem());
		while (pc->RecvList.HasItem())
			free(pc->RecvList.GetItem());
		pc->~ClientData();
		free(pc);
		::InterlockedDecrement(&m_OnlineNum);
	}
}