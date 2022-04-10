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

#ifndef LIBSSH_CPP_WRAP_SCP
#define LIBSSH_CPP_WRAP_SCP

#include <algorithm>
#include <cassert>
#include <concepts>
#include <memory>
#include <iostream>

#include "libssh/libssh.h"

#include "connection.hpp"
#include "error_reporting.hpp"
#include "file_permissions.hpp"

namespace libssh_wrap
{

    enum class ScpAccessMode : int
    {
        Read = SSH_SCP_READ,
        Write = SSH_SCP_WRITE,
    };

    /**
     * \brief a scp session
     */
    class ScpSession
    {
    public:
        ScpSession() noexcept = default;

        ScpSession(std::shared_ptr<AuthenticatedConnection>&& connection, char const* location, ScpAccessMode mode, bool recursive = false)
        {
            if (!connection)
            {
                throw std::runtime_error("no valid connection passed");
            }

            if (location == nullptr)
            {
                throw std::runtime_error("null passed as location");
            }

            auto session = ssh_scp_new(connection->GetSession(), (recursive ? SSH_SCP_RECURSIVE : 0) | static_cast<int>(mode), location);
            if (session == nullptr)
            {
                throw std::runtime_error("error generating scp session");
            }
            m_session.reset(session);
            if (ssh_scp_init(session) != SSH_OK)
            {
                throw std::runtime_error("error initializing the sftp session");
            }
            m_connection = std::move(connection);
        }

        ScpSession(std::shared_ptr<AuthenticatedConnection>&&, std::nullptr_t, ScpAccessMode, bool) = delete;

        ScpSession(std::shared_ptr<AuthenticatedConnection> const& connection, char const* location, ScpAccessMode mode, bool recursive = false)
            : ScpSession(std::shared_ptr(connection), location, mode, recursive)
        {
        }

        ScpSession(std::shared_ptr<AuthenticatedConnection> const&, std::nullptr_t, ScpAccessMode, bool) = delete;

        ScpSession(ScpSession&&) noexcept = default;
        ScpSession& operator=(ScpSession&&) noexcept = default;

        ~ScpSession() noexcept
        {
            if (m_session)
            {
                [[maybe_unused]] auto err = ssh_scp_close(m_session.get());
                assert(err == SSH_OK);
            }
        }

        void PushDirectory(char const* directory, FilePermissions mode)
        {
            if (!m_session)
            {
                throw std::runtime_error("no active scp session");
            }
            if (directory == nullptr)
            {
                throw std::runtime_error("no directory name provided");
            }
            auto res = ssh_scp_push_directory(m_session.get(), directory, mode);
            if (res != SSH_OK)
            {
                ReportError("ssh_scp_push_directory", m_connection->GetSession());
            }
            ++m_directoryDepth;
        }

        void PushDirectory(std::nullptr_t) = delete;

        void LeaveDirectory()
        {
            if (!m_session)
            {
                throw std::runtime_error("no active scp session");
            }
            if (m_directoryDepth == 0)
            {
                throw std::runtime_error("not inside a directory");
            }
            auto res = ssh_scp_leave_directory(m_session.get());
            if (res != SSH_OK)
            {
                ReportError("ssh_scp_push_directory", m_connection->GetSession());
            }
            --m_directoryDepth;
        }

        template<size_t bufferSize = 1024>
        void WriteFile(const char* filename, std::istream& input, size_t inputSize, FilePermissions mode)
        {
            if (!m_session)
            {
                throw std::runtime_error("no active scp session");
            }

            if (filename == nullptr)
            {
                throw std::runtime_error("null provided as filename");
            }

            {
                auto err = ssh_scp_push_file(m_session.get(), filename, inputSize, mode);
                if (err != SSH_OK)
                {
                    ReportError("ssh_scp_push_file", m_connection->GetSession());
                }
            }

            {
                char buffer[bufferSize];

                while (inputSize != 0)
                {
                    size_t const readCount = (std::min)(inputSize, bufferSize);
                    input.read(buffer, readCount);

                    auto err = ssh_scp_write(m_session.get(), buffer, readCount);
                    if (err != SSH_OK)
                    {
                        ReportError("ssh_scp_write", m_connection->GetSession());
                    }
                    inputSize -= readCount;
                }
            }
        }

        template<size_t bufferSize = 1024>
        void WriteFile(std::nullptr_t, std::istream& input, size_t inputSize, FilePermissions mode) = delete;

        template<size_t bufferSize = 1024>
        void ReadFile(std::ostream& out)
        {
            if (!m_session)
            {
                throw std::runtime_error("no active scp session");
            }

            {
                auto err = ssh_scp_pull_request(m_session.get());
                if (err != SSH_SCP_REQUEST_NEWFILE)
                {
                    ReportError("ssh_scp_pull_request", m_connection->GetSession());
                }
            }

            char buffer[bufferSize];

            auto const size = ssh_scp_request_get_size(m_session.get());

            int read = 0;
            while (read < size) {
                int readCount = (std::min)(size - read, bufferSize);
                int numBytes = ssh_scp_read(m_session.get(), buffer, readCount);
                if (numBytes < 0)
                {
                    ReportError("ssh_scp_read", m_connection->GetSession());
                }
                else if (numBytes != 0)
                {
                    out.write(buffer, static_cast<size_t>(numBytes));
                    read += numBytes;
                }
            }
        }
    private:

        size_t m_directoryDepth{ 0 };

        std::shared_ptr<AuthenticatedConnection> m_connection;

        struct SessionDeleter
        {
            void operator()(ssh_scp session) const noexcept
            {
                ssh_scp_free(session);
            }
        };

        std::unique_ptr<std::remove_pointer_t<ssh_scp>, SessionDeleter> m_session;
    };

}

#endif