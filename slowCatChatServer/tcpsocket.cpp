#include "tcpsocket.h"

tcpSocket::tcpSocket()
{
    connect(this, &QTcpSocket::readyRead, this, &tcpSocket::recvMsg);
    connect(this, &QTcpSocket::disconnected, this, &tcpSocket::handleUserOffline);
}

tcpSocket &tcpSocket::getSocketInstance()
{
    static tcpSocket instance;
    return instance;
}

void tcpSocket::recvMsg()
{
    uint uiPDULen = 0;
    this -> read((char*)&uiPDULen, sizeof(uint));
    uint uiMsgLen = uiPDULen - sizeof(PDU);
    PDU *pdu = mkPDU(uiMsgLen);
    this -> read((char*)pdu + sizeof(uint), uiPDULen - sizeof(uint));

    PDU* resPdu = nullptr;
    switch(pdu -> uiMsgType){
    case ENUM_MSG_TYPE_REGIST_REQUEST : {//注册请求
        resPdu = handleRegistRequest(pdu);
        break;
    }
    case ENUM_MSG_TYPE_LOGIN_REQUEST : {//登录请求
        resPdu = handleLoginRequest(pdu);
        break;
    }
    case ENUM_MSG_TYPE_SETUSERNAME_REQUEST : {//设置用户名请求
        resPdu = handleSetUserNameRequest(pdu);
        break;
    }
    default:break;
    }
    //将处理结果发送给用户
    this -> write((char*)resPdu,resPdu -> uiPDULen);

    //清理内存
    free(resPdu);
    resPdu = nullptr;
}

void tcpSocket::handleUserOffline()
{
    operatedatabase::getDBoperateInstance().userOffline(m_strAccount.toStdString().c_str());
    emit offline(this);
}

PDU *tcpSocket::handleRegistRequest(PDU *msg)
{
    char account[32];
    char password[32];
    memset(account, '\0', 32);
    memset(password, '\0', 32);
    strncpy(account, msg -> caData, 32);
    strncpy(password, msg -> caData + 32, 32);
    bool insertrDataRet = operatedatabase::getDBoperateInstance().userinsertToDb(account, password);
    PDU *resPdu = mkPDU(0);
    resPdu -> uiMsgType = ENUM_MSG_TYPE_REGIST_RESPOND;
    if(insertrDataRet)
    {
        strcpy(resPdu -> caData, REGIST_OK);
    }
    else strcpy(resPdu -> caData, REGIST_ERROR);

    return resPdu;
}

PDU *tcpSocket::handleLoginRequest(PDU *msg)
{
    char account[32];
    char password[32];
    memset(account, '\0', 32);
    memset(password, '\0', 32);
    strncpy(account, msg -> caData, 32);
    //记录账户名
    m_strAccount = QString(account);

    strncpy(password, msg -> caData + 32, 32);
    uint checkUserLoginRet = operatedatabase::getDBoperateInstance().userLoginCheck(account, password);
    PDU *resPdu = mkPDU(0);
    resPdu -> uiMsgType = ENUM_MSG_TYPE_LOGIN_RESPOND;
    if(ENUM_USER_NO_EXIST == checkUserLoginRet){//用户不存在
        strcpy(resPdu -> caData, LOGIN_USER_NO_EXIST);
    }
    else if(ENUM_USER_PASSWORD_ERROR == checkUserLoginRet){//密码错误
        strcpy(resPdu -> caData, LOGIN_ERROR_PASSWORD);
    }
    else if(ENUM_USER_REPEAT_LOGIN == checkUserLoginRet){//用户重复登录
        strcpy(resPdu -> caData, LOGIN_ERROR_REPEAT);
    }
    else if(ENUM_USER_EXIST_NAMENULL == checkUserLoginRet){//登录成功但用户名为空
        strcpy(resPdu -> caData, LOGIN_OK_NAMENULL);
    }
    else if(ENUM_USER_EXIST_NAMENONULL == checkUserLoginRet){//登录成功且用户名不为空
        strcpy(resPdu -> caData, LOGIN_OK_NAMENONULL);
    }
    return resPdu;
}

PDU *tcpSocket::handleSetUserNameRequest(PDU *msg)
{
    char account[32];
    char username[32];
    memset(account, '\0', 32);
    memset(username, '\0', 32);
    strncpy(account, msg -> caData, 32);
    strncpy(username, msg -> caData + 32, 32);
    uint unSetUserNameResult =  operatedatabase::getDBoperateInstance().userSetNameToDb(account, username);
    PDU* resPdu = mkPDU(0);
    resPdu -> uiMsgType = ENUM_MSG_TYPE_SETUSERNAME_RESPOND;
    if(ENUM_CHECK_USER_NAME_EXIST == unSetUserNameResult){
        strcpy(resPdu -> caData, SETUSERNAME_ERROR);
    }
    else if(ENUM_CHECK_USER_NAME_OK == unSetUserNameResult){
        strcpy(resPdu -> caData, SETUSERNAME_OK);
    }
    return resPdu;
}
