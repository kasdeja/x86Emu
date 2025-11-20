#include "Keyboard.h"

void Keyboard::AddKey(uint8_t key)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_keys.push(key);
}

void Keyboard::RemoveKey()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_keys.empty())
    {
        m_keys.pop();
    }
}

uint8_t Keyboard::GetKey()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_keys.empty())
    {
        return m_keys.front();
    }

    return 0;
}

bool Keyboard::HasKey()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return !m_keys.empty();
}
