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

#ifndef LIBSSH_CPP_WRAP_SFTP_CHANNEL
#define LIBSSH_CPP_WRAP_SFTP_CHANNEL

#include <cassert>
#include <chrono>
#include <future>
#include <memory>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <thread>

#include <sys/stat.h>
#include <fcntl.h>

#include "libssh/sftp.h"

#include "connection.hpp"

namespace libssh_wrap
{

    enum class FilePermission : mode_t
    {
        Read = 4,
        Write = 2,
        Execute = 1
    };

    struct FilePermissions
    {
        constexpr FilePermissions(mode_t mode)
            : m_ownerPermission((mode >> 6) & 7),
            m_groupPermission((mode >> 3) & 7),
            m_worldPermission(mode & 7)
        {
        }

        constexpr FilePermissions(mode_t ownerRights, mode_t groupRights, mode_t worldRights)
            : m_ownerPermission(ownerRights & 7),
            m_groupPermission(groupRights & 7),
            m_worldPermission(worldRights & 7)
        {
        }

        mode_t m_ownerPermission;
        mode_t m_groupPermission;
        mode_t m_worldPermission;

        constexpr operator mode_t() const noexcept
        {
            return ((m_ownerPermission & 7) << 6) |
                ((m_groupPermission & 7) << 3) |
                ((m_worldPermission & 7));
        }
    };

    enum class FileAccessMode : int
    {
        ReadOnly = O_RDONLY,
        WriteOnly = O_WRONLY,
        ReadWrite = O_RDWR
    };

    enum class FileExistenceRequirement : int
    {
        MayExist = O_CREAT,
        MustExist = O_CREAT | O_EXCL
    };

    enum class FileTruncation : int
    {
        Truncate = O_TRUNC,
        Append = 0,
    };

    template<class T>
    concept ErrorPredicate = requires(T && t, int error)
    {
        std::is_nothrow_convertible_v<decltype(t(error)), bool>;
    };

    constexpr bool DoNotIgnoreError(int error)
    {
        return error != SSH_OK;
    }

    constexpr bool IgnoreAlreadyExists(int error)
    {
        return error != SSH_OK && error != SSH_FX_FILE_ALREADY_EXISTS;
    }

    class FileStream;

    /**
     * \brief a channel for executing a ssh command
     */
    class SftpChannel
    {
    public:
        SftpChannel() noexcept = default;

        SftpChannel(std::shared_ptr<AuthenticatedConnection>&& connection)
        {
            if (!connection)
            {
                throw std::runtime_error("no valid connection passed");
            }

            auto session = sftp_new(connection->GetSession());
            if (session == nullptr)
            {
                throw std::runtime_error("error generating sftp session");
            }
            m_session.reset(session);
            if (sftp_init(session) != SSH_OK)
            {
                throw std::runtime_error("error initializing the sftp session");
            }
            m_connection = std::move(connection);
        }

        SftpChannel(std::shared_ptr<AuthenticatedConnection> const& connection)
            : SftpChannel(std::shared_ptr(connection))
        {
        }

        SftpChannel(SftpChannel&&) noexcept = default;
        SftpChannel& operator=(SftpChannel&&) noexcept = default;

        /**
         * \note named MakeDirectory instead of CreateDirectory to avoid the macro from the windows headers
         *       interfering with the nameing
         */
        template<ErrorPredicate Predicate = decltype(&DoNotIgnoreError)>
        void MakeDirectory(char const* dirName, FilePermissions permissions, Predicate&& predicate = {})
        {
            if (!m_session)
            {
                throw std::runtime_error("no active sftp session");
            }
            
            auto rc = sftp_mkdir(m_session.get(), dirName, permissions);
            if (std::forward<Predicate>(predicate)(rc))
            {
                throw std::runtime_error("error creating directory");
            }
        }

        template<ErrorPredicate Predicate = decltype(&DoNotIgnoreError)>
        void MakeDirectory(std::nullptr_t, FilePermissions, Predicate&&) = delete;

        template<ErrorPredicate Predicate = decltype(&DoNotIgnoreError)>
        void RemoveDirectory(char const* dirName, Predicate&& predicate = {})
        {
            if (!m_session)
            {
                throw std::runtime_error("no active sftp session");
            }
            
            auto rc = sftp_rmdir(m_session.get(), dirName);
            if (std::forward<Predicate>(predicate)(rc))
            {
                throw std::runtime_error("error creating directory");
            }
        }

        template<ErrorPredicate Predicate>
        void RemoveDirectory(std::nullptr_t, Predicate&& = {}) = delete;

        template<ErrorPredicate Predicate = decltype(&DoNotIgnoreError)>
        void DeleteFile(char const* fileName, Predicate&& predicate = {})
        {
            if (!m_session)
            {
                throw std::runtime_error("no active sftp session");
            }
            
            auto rc = sftp_unlink(m_session.get(), fileName);
            if (std::forward<Predicate>(predicate)(rc))
            {
                throw std::runtime_error("error creating directory");
            }
        }

        template<ErrorPredicate Predicate>
        void DeleteFile(std::nullptr_t, Predicate&& = {}) = delete;

        template<ErrorPredicate Predicate = decltype(&DoNotIgnoreError)>
        void Chmod(char const* fileName, FilePermissions targetPermissions, Predicate&& predicate = {})
        {
            if (!m_session)
            {
                throw std::runtime_error("no active sftp session");
            }
            
            auto rc = sftp_chmod(m_session.get(), fileName, targetPermissions);
            if (std::forward<Predicate>(predicate)(rc))
            {
                throw std::runtime_error("error creating directory");
            }
        }

        template<ErrorPredicate Predicate>
        void Chmod(std::nullptr_t, FilePermissions, Predicate&& = {}) = delete;

        /**
         * \note truncate is removed, if the file is opened readonly
         */
        template<class FirstModifierType = int, class...ModifierTypes>
        FileStream OpenFile(char const* fileName,
            FilePermissions permissions,
            FileAccessMode accessMode,
            FirstModifierType&& accessSpecifiers = FirstModifierType(static_cast<int>(FileExistenceRequirement::MayExist) | static_cast<int>(FileTruncation::Truncate)),
            ModifierTypes&&... types);
    private:

        std::shared_ptr<AuthenticatedConnection> m_connection;

        struct SessionDeleter
        {
            void operator()(sftp_session channel) const noexcept
            {
                sftp_free(channel);
            }
        };

        std::unique_ptr<std::remove_pointer_t<sftp_session>, SessionDeleter> m_session;
    };

    class FileStream
    {
    public:
        FileStream(sftp_file file = nullptr)
            : m_file(file)
        {}

        FileStream(FileStream&&) noexcept = default;
        FileStream& operator=(FileStream&&) noexcept = default;

        template<size_t bufferSize = 1024>
        void Write(std::istream& in)
        {
            if (!m_file)
            {
                throw std::runtime_error("file not opened");
            }

            char buffer[bufferSize];

            do
            {
                in.read(buffer, bufferSize);
                if (in.bad())
                {
                    throw std::runtime_error("error reading input stream");
                }
                auto read = in.gcount();
                if (read > 0)
                {
                    auto written = sftp_write(m_file.get(), buffer, read);
                    if (written != read)
                    {
                        throw std::runtime_error("error writing file");
                    }
                }
            } while (in);
        }

        template<size_t bufferSize = 1024>
        std::future<std::shared_ptr<FileStream>> WriteAsync(std::istream& in)
        {
            if (!m_file)
            {
                throw std::runtime_error("file not opened");
            }

            auto ptr = std::make_shared<FileStream>(std::move(*this));
            return std::async([](std::shared_ptr<FileStream> const& ptr, std::istream& in)
                {
                    ptr->Write<bufferSize>(in);
                    return ptr;
                },
                ptr,
                std::reference_wrapper(in));
        }

        template<size_t bufferSize = 1024>
        void Read(std::ostream& out)
        {
            if (!m_file)
            {
                throw std::runtime_error("file not opened");
            }

            char buffer[bufferSize];

            ssize_t readCount;
            do
            {
                readCount = sftp_read(m_file.get(), buffer, bufferSize);

                if (readCount < 0)
                {
                    throw std::runtime_error("error reading file");
                }

                out.write(buffer, readCount);
                if (!out)
                {
                    throw std::runtime_error("error writing the contents read via ssh to output stream");
                }
            } while (readCount > 0);
        }

        template<size_t bufferSize = 1024>
        std::future<std::shared_ptr<FileStream>> ReadAsync(std::ostream& out)
        {
            if (!m_file)
            {
                throw std::runtime_error("file not opened");
            }

            auto ptr = std::make_shared<FileStream>(std::move(*this));
            return std::async([](std::shared_ptr<FileStream> const& ptr, std::ostream& out)
                {
                    ptr->Read<bufferSize>(out);
                    return ptr;
                },
                ptr,
                std::reference_wrapper(out));
        }
    private:

        struct ChannelDeleter
        {
            void operator()(sftp_file file) const noexcept
            {
                [[maybe_unused]] int result = sftp_close(file);
                assert(result == SSH_OK);
            }
        };

        std::unique_ptr<std::remove_pointer_t<sftp_file>, ChannelDeleter> m_file;
    };

    template<class FirstModifierType, class ...ModifierTypes>
    inline FileStream SftpChannel::OpenFile(char const* fileName,
        FilePermissions permissions,
        FileAccessMode accessMode,
        FirstModifierType&& accessSpecifiers,
        ModifierTypes && ...modifiers)
    {
        int effectiveAccessMode = ((static_cast<int>(accessMode) | static_cast<int>(std::forward<FirstModifierType>(accessSpecifiers)))
            | ... | static_cast<int>(std::forward<ModifierTypes>(modifiers)));

        if (accessMode == FileAccessMode::ReadOnly)
        {
            // remove truncate, if we're using readonly mode
            static_assert(static_cast<int>(FileTruncation::Append) == 0, "relying on file trunction being 0 here");
            effectiveAccessMode &= ~static_cast<int>(FileTruncation::Truncate);
        }
        auto file = sftp_open(m_session.get(), fileName, effectiveAccessMode, permissions);
        if (file == nullptr)
        {
            ReportError("error opening file", m_session->session);
        }
        return FileStream(file);
    }
}

#endif