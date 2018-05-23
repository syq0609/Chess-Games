#include "packet.h"
#include <cstring>

namespace water{
namespace net{

const Packet::SizeType Packet::MIN_SIZE;
const Packet::SizeType Packet::MAX_SIZE;
const Packet::SizeType Packet::MIN_CONTENT_SIZE;
const Packet::SizeType Packet::MAX_CONTENT_SIZE;
const Packet::SizeType Packet::HEAD_SIZE;

Packet::Packet()
{
    m_buf.reserve(64);
}

Packet::Packet(SizeType size)
    :m_buf(size, 0)
{
}

Packet::Packet(const void* data, SizeType size)
: m_buf(reinterpret_cast<const char*>(data), reinterpret_cast<const char*>(data) + size)
{
}

char* Packet::data()
{
    return m_buf.data(); 
}

const char* Packet::data() const
{
    return m_buf.data();
}

Packet::SizeType Packet::append(const void* data, SizeType size)
{
    m_buf.resize(m_buf.size() + size);
    memcpy(m_buf.data() + m_buf.size() - size, data, size);
    return size;
}

void Packet::pop(SizeType size)
{
    if (size >= m_buf.size())
    {
        m_buf.clear();
        return;
    }

    auto moveSize = m_buf.size() - size;
    
    memmove(m_buf.data(), m_buf.data() + size, moveSize);
    m_buf.resize(moveSize);
}

Packet::SizeType Packet::copy(void* buff, SizeType maxSize)
{
    SizeType copySize = maxSize < m_buf.size() ? maxSize : m_buf.size();
    std::memcpy(buff, m_buf.data(), copySize);
    return copySize;
}

Packet::SizeType Packet::size() const
{
    return m_buf.size();
}


}}

