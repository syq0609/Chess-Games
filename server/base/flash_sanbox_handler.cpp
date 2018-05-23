#include  "flash_sanbox_handler.h"

#include "componet/logger.h"
#include "net/buffered_connection.h"

namespace water{
namespace process{

void FlashSandboxHandler::addFlashSandboxConn(net::BufferedConnection::Ptr conn)
{
    try
    {
        conn->setNonBlocking();
        ConnHolder connHolder;
        connHolder.state = ConnState::recving;
        connHolder.tp = componet::Clock::now();
        connHolder.conn = conn;

        m_lock.lock();
        m_conns.push_back(connHolder);
        m_lock.unlock();
    }
    catch(const componet::ExceptionBase& ex)
    {
        LOG_ERROR("flash sandbox 验证出错, remoteEp={}, {}", conn->getRemoteEndpoint(), ex);
    }
}

bool FlashSandboxHandler::exec()
{
    const std::chrono::seconds timeOutDuration(5);
    const char revText[] = "<policy-file-request/>";
    const char sendText[] = 
    "<cross-domain-policy> <allow-access-from domain=\"*\" to-ports=\"*\"/> </cross-domain-policy>\0";
    while(checkSwitch())
    {
        componet::TimePoint now = componet::Clock::now();

        m_lock.lock();
        for(auto it = m_conns.begin(); it != m_conns.end(); )
        {
            if(it->tp + timeOutDuration < now)
            {
                LOG_TRACE("flash sandbox 验证, 超时, {}", it->conn->getRemoteEndpoint());
                it = m_conns.erase(it);
                continue;
            }
            try
            {
                switch (it->state)
                {
                case ConnState::recving:
                    {
                        it->conn->tryRecv();
                        const auto& revBuf = it->conn->recvBuf().readable();
                        if (revBuf.second < sizeof(revText))
                            continue;
                        std::string text(reinterpret_cast<const char*>(revBuf.first), sizeof(revText));
                        it->conn->recvBuf().commitRead(sizeof(revText));
                        if(text != revText)
                        {
                            LOG_TRACE("Flash Sandbox 验证失败, {}, 收到{}", 
                                      it->conn->getRemoteEndpoint(), text);
                            it = m_conns.erase(it);
                            continue;
                        }
                        const auto& sndBuf = it->conn->sendBuf().writeable(sizeof(sendText));
                        if (sndBuf.second < sizeof(sendText))
                        {
                            LOG_ERROR("Flash Sandbox 发送失败, 缓冲区不足");
                            it = m_conns.erase(it);
                            it->conn->close();
                            continue;
                        }
                        strncpy(reinterpret_cast<char*>(sndBuf.first), sendText, sndBuf.second);
                        it->conn->sendBuf().commitWrite(sizeof(sendText));
                        it->state = ConnState::sending;
                    }
                    //break; 故意不要break
                case ConnState::sending:
                    {
                        if( !it->conn->trySend() )
                            continue;

                        LOG_TRACE("Flash Sandbox 回复{}, {}", it->conn->getRemoteEndpoint(), sendText);
                        it->state = ConnState::waitclose;
                    }
                    //break; 故意不要break
                case ConnState::waitclose:
                    {
                        char buf[1];
                        //收到0字节，说明该socket已被关闭
                        if( 0 == it->conn->recv(buf, sizeof(buf)) ) 
                        {
                            LOG_TRACE("Flash Sandbox 验证完成 {}, 对方已关闭",
                                      it->conn->getRemoteEndpoint());
                            it = m_conns.erase(it);
                            continue;
                        }
                    }
                    break;
                default:
                    break;
                }
            }
            catch (const net::ReadClosedConnection& ex)
            {
                LOG_TRACE("flash sandbox 验证失败 {}, 对方提前断开了连接", 
                          it->conn->getRemoteEndpoint());
                it = m_conns.erase(it);
                continue;
            }
            catch (const net::NetException& ex)
            {
                LOG_ERROR("flash sandbox 检查时出错 {}, {}", 
                          it->conn->getRemoteEndpoint(), ex.what());
                it = m_conns.erase(it);
                continue;
            }
            ++it;
        }
        m_lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return true;
}


}}

