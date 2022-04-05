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

#ifndef LIBSSH_CPP_WRAP_FILE_PERMISSIONS
#define LIBSSH_CPP_WRAP_FILE_PERMISSIONS

#include "libssh/libssh.h"

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

}

#endif