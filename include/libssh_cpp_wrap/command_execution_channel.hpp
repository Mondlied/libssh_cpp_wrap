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

#ifndef LIBSSH_CPP_WRAP_COMMAND_EXECUTION_CHANNEL
#define LIBSSH_CPP_WRAP_COMMAND_EXECUTION_CHANNEL

#include <future>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "libssh/libssh.h"

#include "connection.hpp"

namespace libssh_wrap
{

    /**
     * \brief a channel for executing a ssh command
     */
    class ExecutionChannel
    {
    public:
        ExecutionChannel() noexcept = default;

        ExecutionChannel(std::shared_ptr<AuthenticatedConnection>&& connection)
        {
            if (!connection)
            {
                throw std::runtime_error("no valid connection passed");
            }
            auto channel = ssh_channel_new(connection->GetSession());
            if (channel == nullptr)
            {
                throw std::runtime_error("error generating ssh command channel");
            }
            m_channel.reset(channel);
            m_connection = std::move(connection);
        }

        ExecutionChannel(std::shared_ptr<AuthenticatedConnection> const& connection)
            : ExecutionChannel(std::shared_ptr(connection))
        {
        }

        ExecutionChannel(ExecutionChannel&&) noexcept = default;
        ExecutionChannel& operator=(ExecutionChannel&&) noexcept = default;

        ~ExecutionChannel() noexcept
        {
            if (m_executed && m_channel) // note: test for channel necessary to avoid double close, if moved after m_executed is set to true
            {
                [[maybe_unused]] auto result = ssh_channel_close(m_channel.get());
                assert(result == SSH_OK); // potential issues? TODO: check error sources
            }
        }

        template<size_t bufferSize = 1024>
        auto Execute(std::nullptr_t, std::ostream& outStream, std::ostream& errorStream) = delete;

        template<size_t bufferSize = 1024>
        void Execute(const char* command, std::ostream& outStream, std::ostream& errorStream)
        {
            if (m_executed)
            {
                throw std::runtime_error("there was already a command executed with this executor");
            }
            if (!m_channel)
            {
                throw std::runtime_error("no connection available");
            }
            auto rc = ssh_channel_request_exec(m_channel.get(), command);
            if (rc != SSH_OK)
            {
                throw std::runtime_error("command execution failed");
            }
            m_executed = true;

            ConsumeStreams<bufferSize>(outStream, errorStream);
        }

        /**
         * \return a future that completes when the execution is done returning the 
         */
        template<size_t bufferSize = 1024>
        std::future<std::shared_ptr<AuthenticatedConnection>> ExecuteAsync(const char* command, std::ostream& outStream, std::ostream& errorStream)
        {
            if (m_executed)
            {
                throw std::runtime_error("there was already a command executed with this executor");
            }
            if (!m_channel)
            {
                throw std::runtime_error("no connection available");
            }
            auto rc = ssh_channel_request_exec(m_channel.get(), command);
            if (rc != SSH_OK)
            {
                throw std::runtime_error("command execution failed");
            }
            m_executed = true;

            return std::async(std::launch::async,
                &ExecutionChannel::AsyncConsume<bufferSize>,
                std::make_shared<ExecutionChannel>(std::move(*this)),
                std::reference_wrapper(outStream),
                std::reference_wrapper(errorStream));
        }

    private:

        template<size_t bufferSize>
        static std::shared_ptr<AuthenticatedConnection> AsyncConsume(std::shared_ptr<ExecutionChannel> const& executionChannel, std::ostream& outStream, std::ostream& errorStream)
        {
            executionChannel->ConsumeStreams<bufferSize>(outStream, errorStream);
            return std::move(executionChannel->m_connection);
        }

        template<size_t N>
        static bool StreamPipe(ssh_channel channel, char(&buffer)[N], int isStdErr, std::ostream& stream, char const* errorMessage)
        {
            bool hasRead = false;

            int bytesRead = ssh_channel_read(channel, buffer, N, isStdErr);
            while (bytesRead > 0)
            {
                hasRead = true;
                stream.write(buffer, static_cast<size_t>(bytesRead));
                bytesRead = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
            }

            if (bytesRead != 0)
            {
                throw std::runtime_error(errorMessage);
            }
            return hasRead;
        }

        template<size_t bufferSize>
        void ConsumeStreams(std::ostream& outStream, std::ostream& errorStream) const
        {
            {
                char buffer[bufferSize];

                while (StreamPipe(m_channel.get(), buffer, 0, outStream, "error reading stdout")
                    || StreamPipe(m_channel.get(), buffer, 1, errorStream, "error reading stderr"))
                {
                }
            }

            if (ssh_channel_send_eof(m_channel.get()) != SSH_OK)
            {
                throw std::runtime_error("error sending eof to channel");
            }
        }

        std::shared_ptr<AuthenticatedConnection> m_connection;

        struct ChannelDeleter
        {
            void operator()(ssh_channel channel) const noexcept
            {
                ssh_channel_free(channel);
            }
        };

        std::unique_ptr<std::remove_pointer_t<ssh_channel>, ChannelDeleter> m_channel;
        bool m_executed{ false };

    };
}

#endif