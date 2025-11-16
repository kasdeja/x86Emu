#include "Keyboard.h"

void Keyboard::AddKey(uint8_t key)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_keys.push(key);
}

uint8_t Keyboard::GetKey()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    uint8_t key = 0;

    if (!m_keys.empty())
    {
        key = m_keys.front();
        m_keys.pop();
    }

    return key;
}

bool Keyboard::HasKey()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return !m_keys.empty();
}
