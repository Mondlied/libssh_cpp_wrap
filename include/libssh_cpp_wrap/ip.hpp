// libssh_cpp_wrap (A C++ wrapper for libssh)
// 
// Copyright(C) 2022 Fabian Klein
//
// This library is free software; you can redistribute itand /or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301
// USA

#ifndef LIBSSH_CPP_WRAP_IP
#define LIBSSH_CPP_WRAP_IP

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>

namespace libssh_wrap
{

    /**
     * \brief An ip v4 address
     * 
     * Can be used as ssh session option; in that case the connection target is set to the address.
     */
    class IpV4
    {
    public:

        constexpr IpV4(uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4)
            : m_parts{p1, p2, p3, p4}
        {
        }

        constexpr IpV4()
            : IpV4(0, 0, 0, 0)
        {}

        constexpr IpV4(char const* stringRep)
        {
            uint16_t part = 0;
            size_t outIndex = 0;
            size_t inIndex = 0;
            bool error = false;
            for (; !error && stringRep[inIndex] != '\0' && outIndex < 4 && part <= 255; ++inIndex)
            {
                char const c = stringRep[inIndex];
                if (c == '.')
                {
                    m_parts[outIndex] = part;
                    ++outIndex;
                    part = 0;
                }
                else if (c < '0' || c > '9')
                {
                    error = true;
                }
                else
                {
                    part = part * 10 + static_cast<uint16_t>(c - '0');
                }
            }

            if (!error && outIndex == 3 && stringRep[inIndex] == '\0' && part <= 255u)
            {
                m_parts[3] = part;
            }
            else
            {
                // some error occured -> set to 0
                m_parts[0] = 0;
                m_parts[1] = 0;
                m_parts[2] = 0;
                m_parts[3] = 0;
            }
        }

        constexpr IpV4(IpV4 const& other) noexcept
            : m_parts(other.m_parts)
        {
        }

        IpV4& operator=(IpV4 const& other) noexcept
        {
            m_parts = other.m_parts;
            return *this;
        }

        static constexpr size_t MaxCStringLength = sizeof("255.255.255.255");

        void FillToCString(char(&out)[MaxCStringLength]) const noexcept
        {
            std::snprintf(out, MaxCStringLength, "%hhu.%hhu.%hhu.%hhu", m_parts[0], m_parts[1], m_parts[2], m_parts[3]);
        }

        int FillToCString(char out[], size_t outSize) const noexcept
        {
            return std::snprintf(out, outSize, "%hhu.%hhu.%hhu.%hhu", m_parts[0], m_parts[1], m_parts[2], m_parts[3]);
        }

        operator std::string() const
        {
            char buffer[MaxCStringLength];
            FillToCString(buffer);
            return buffer;
        }

        friend auto operator<=>(IpV4 const&, IpV4 const&) noexcept = default;

        friend std::ostream& operator<<(std::ostream& s, IpV4 const& ip)
        {
            s
                << static_cast<uint32_t>(ip.m_parts[0]) << '.'
                << static_cast<uint32_t>(ip.m_parts[1]) << '.'
                << static_cast<uint32_t>(ip.m_parts[2]) << '.'
                << static_cast<uint32_t>(ip.m_parts[3]);
            return s;
        }

        constexpr uint8_t operator[](uint32_t index) const
        {
            return m_parts[index];
        }

        constexpr uint8_t& operator[](uint32_t index)
        {
            return m_parts[index];
        }

        [[nodiscard]] constexpr bool PrefixMatches(IpV4 const& other, size_t byteCount) const noexcept
        {
            return std::equal(m_parts.begin(),
                              m_parts.begin() + (std::min)(byteCount, m_parts.size()),
                              other.m_parts.begin());
        }

        [[nodiscard]] constexpr IpV4 WithSuffix(uint8_t p4)
        {
            return IpV4(m_parts[0], m_parts[1], m_parts[2], p4);
        }
        
        [[nodiscard]] constexpr IpV4 WithSuffix(uint8_t p3, uint8_t p4)
        {
            return IpV4(m_parts[0], m_parts[1], p3, p4);
        }
        
        [[nodiscard]] constexpr IpV4 WithSuffix(uint8_t p2, uint8_t p3, uint8_t p4)
        {
            return IpV4(m_parts[0], p2, p3, p4);
        }

        [[nodiscard]] constexpr IpV4 WithPrefix(uint8_t p1)
        {
            return IpV4(p1, m_parts[1], m_parts[2], m_parts[3]);
        }

        [[nodiscard]] constexpr IpV4 WithPrefix(uint8_t p1, uint8_t p2)
        {
            return IpV4(p1, p2, m_parts[2], m_parts[3]);
        }

        [[nodiscard]] constexpr IpV4 WithPrefix(uint8_t p1, uint8_t p2, uint8_t p3)
        {
            return IpV4(p1, p2, p3, m_parts[3]);
        }
    private:
        std::array<uint8_t, 4> m_parts;
    };

}

#endif