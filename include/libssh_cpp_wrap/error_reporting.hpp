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

#ifndef LIBSSH_CPP_WRAP_SSH_ERROR_REPORTING
#define LIBSSH_CPP_WRAP_SSH_ERROR_REPORTING

#include <stdexcept>
#include <string>

#include "libssh/libssh.h"

namespace libssh_wrap
{
    inline void ReportError(char const* message, void* entity)
    {
        std::string errorMessage;
        if (message != nullptr)
        {
            errorMessage += message;
            errorMessage += ": ";
        }
        auto sshErrorMessage = ssh_get_error(entity);
        if (sshErrorMessage == nullptr)
        {
            errorMessage += "SSH ERROR UNAVAILABLE";
        }
        else
        {
            errorMessage += sshErrorMessage;
        }
        throw std::runtime_error(errorMessage);
    }
}

#endif