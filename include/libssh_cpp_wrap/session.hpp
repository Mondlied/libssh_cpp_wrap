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

#ifndef LIBSSH_CPP_WRAP_SSH_SESSION_OPTIONS
#define LIBSSH_CPP_WRAP_SSH_SESSION_OPTIONS

#include <memory>
#include <stdexcept>
#include <type_traits>

#include "libssh/libssh.h"

#include "connection.hpp"

namespace libssh_wrap
{
    class Connection;

    class Session
    {
    public:
        /**
         * \brief takes ownership of a given session
         */
        Session(ssh_session sshSession = nullptr) noexcept
            : m_sshSession(sshSession)
        {}

        Session(Session&&) noexcept = default;
        Session& operator=(Session&&) noexcept = default;

        /**
         * Creates a new session
         */
        static [[nodiscard("dropping the return value results in a session destruction")]] Session Create()
        {
            auto session = ssh_new();
            if (session == nullptr)
            {
                throw std::runtime_error("could not create a ssh session");
            }
            return { session };
        }

        /**
         * \return true if and only if there's a session owned by this object
         */
        [[nodiscard]]  operator bool() const noexcept
        {
            return static_cast<bool>(m_sshSession);
        }

        template<class T>
        void SetOption(T const& option)
        {
            if (!m_sshSession)
            {
                throw std::runtime_error("no session active");
            }
            if (ApplyOption(*m_sshSession, option) != SSH_OK)
            {
                throw std::runtime_error("error setting the option");
            }
        }

    private:
        friend class Connection;

        struct SessionDeleter
        {
            void operator()(ssh_session session) const noexcept
            {
                ssh_free(session);
            }
        };

        std::unique_ptr<std::remove_pointer_t<ssh_session>, SessionDeleter> m_sshSession;
    };
}

#endif