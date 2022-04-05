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

#ifndef LIBSSH_CPP_WRAP_CONNECTION
#define LIBSSH_CPP_WRAP_CONNECTION

#include <cassert>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "libssh/libssh.h"

#include "error_reporting.hpp"
#include "session.hpp"

namespace libssh_wrap
{

    class AuthenticatedConnection;

    class Connection
    {
    public:

        Connection() noexcept = default;

        /**
         * \brief Create a ssh connection and take ownership of the object
         * 
         * \exception ::std::runtime_error If \p session is not valid
         */
        Connection(Session&& session)
        {
            if (!session)
            {
                ReportInvalidSession();
            }
            auto errorCode = ssh_connect(session.m_sshSession.get());
            if (errorCode != SSH_OK)
            {
                ReportError("ssh_connect unsuccessful", session.m_sshSession.get());
            }
            m_session = std::move(session);
        }

        ~Connection() noexcept
        {
            if (m_session)
            {
                ssh_disconnect(m_session.m_sshSession.get());
            }
        }

        Connection(Connection&&) noexcept = default;
        Connection& operator=(Connection&&) noexcept = default;

        /**
         * \return true if and only if there's a session owned by this object
         */
        [[nodiscard]] operator bool() const noexcept
        {
            return static_cast<bool>(m_session);
        }

        /**
         * \brief disconnect and transfer session ownership to the returned object.
         * 
         * \exception ::std::runtime_error If there is no valid session for this object
         */
        [[nodiscard("dropping the return value results in a session destruction")]] Session Disconnect()
        {
            ssh_disconnect(GetSession());
            return std::move(m_session);
        }

        [[nodiscard("dropping the return value results in a session destruction")]]
        std::shared_ptr<AuthenticatedConnection> Authenticate(char const* password) &&;

        std::shared_ptr<AuthenticatedConnection> Authenticate(std::nullptr_t) = delete;

    private:
        ssh_session GetSession()
        {
            if (!m_session)
            {
                ReportInvalidSession();
            }
            return m_session.m_sshSession.get();
        }

        friend class AuthenticatedConnection;

        void ReportInvalidSession()
        {
            throw std::runtime_error("the session object is invalid");
        }

        Session m_session;
    };

    class ExecutionChannel;
    class ScpSession;
    class SftpChannel;

    class AuthenticatedConnection
    {
    public:
        AuthenticatedConnection() noexcept = default;
        AuthenticatedConnection(AuthenticatedConnection&&) noexcept = default;
        AuthenticatedConnection& operator=(AuthenticatedConnection&&) noexcept = default;

        /**
         * \brief disconnect and transfer session ownership to the returned object.
         *
         * \exception ::std::runtime_error If there is no valid session for this object
         */
        [[nodiscard("dropping the return value results in a session destruction")]] Session ForceDisconnect()
        {
            std::lock_guard guard(m_mutex);
            return m_connection.Disconnect();
        }

        /**
         * Create a password authentication
         */
        [[deprecated("for internal use only")]] AuthenticatedConnection(Connection&& connection, char const* password)
        {
            auto errorCode = ssh_userauth_password(connection.GetSession(), nullptr, password);
            if (errorCode != SSH_OK)
            {
                ReportError("password authentication failed", connection.GetSession());
            }
            m_connection = std::move(connection);
        }
    private:

        std::mutex m_mutex;

        ssh_session GetSession()
        {
            std::lock_guard guard(m_mutex);
            return m_connection.GetSession();
        }

        friend class Connection;
        friend class ExecutionChannel;
        friend class ScpSession;
        friend class SftpChannel;

        Connection m_connection;
    };

    inline std::shared_ptr<AuthenticatedConnection> libssh_wrap::Connection::Authenticate(char const* password) &&
    {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996 )
#endif
        return std::make_shared<AuthenticatedConnection>(std::move(*this), password);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    }
}

#endif