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

#ifndef LIBSSH_CPP_WRAP_SSH_SESSION
#define LIBSSH_CPP_WRAP_SSH_SESSION

#include <cstddef>
#include <string_view>
#include <type_traits>

#include "libssh/libssh.h"

#include "ip.hpp"

namespace libssh_wrap
{
    /**
     * Sets the ssh host to the given ip
     */
    inline int ApplyOption(std::remove_pointer_t<ssh_session>& session, IpV4 const& ip) noexcept
    {
        char buffer[IpV4::MaxCStringLength];
        ip.FillToCString(buffer);
        return ssh_options_set(&session, SSH_OPTIONS_HOST, buffer);
    }

    /**
     * \todo add check, if call is noexcept
     */
    template<class T>
    auto ApplyOption(std::remove_pointer_t<ssh_session>& session, T const& option) noexcept(noexcept(option(session)))
        -> std::enable_if_t<std::is_nothrow_convertible_v<decltype(option(session)), int>, int>
    {
        return static_cast<int>(option(session));
    }

    /**
     * \brief A ssh session option specifying the target port
     */
    struct Port
    {
        int m_port{ 22 };

        int operator()(std::remove_pointer_t<ssh_session>& session) const noexcept
        {
            return ssh_options_set(&session, SSH_OPTIONS_PORT, &m_port);
        }
    };

    /**
     * \brief A ssh session option specifying the user name
     */
    struct UserName
    {
        constexpr UserName(char const* name)
            : m_name(name)
        {
        }

        UserName(std::nullptr_t) = delete;

        char const* m_name;

        int operator()(std::remove_pointer_t<ssh_session>& session) const noexcept
        {
            return ssh_options_set(&session, SSH_OPTIONS_USER, m_name);
        }
    };

}

#endif