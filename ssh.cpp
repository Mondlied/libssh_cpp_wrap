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

#include <stdexcept>
#include <iostream>

#include "libssh_cpp_wrap/command_execution_channel.hpp"
#include "libssh_cpp_wrap/connection.hpp"
#include "libssh_cpp_wrap/ip.hpp"
#include "libssh_cpp_wrap/session.hpp"
#include "libssh_cpp_wrap/session_options.hpp"
#include "libssh_cpp_wrap/sftp_channel.hpp"

int main()
{
    using namespace libssh_wrap;

    constexpr IpV4 ip("127.0.0.1");

    try
    {
        Session session = Session::Create();
        session.SetOption(ip);
        session.SetOption(Port());
        session.SetOption(UserName("root"));

#if 1
        auto connection = Connection(std::move(session)).Authenticate("root");

        ExecutionChannel(connection).Execute("ls -al", std::cout, std::cerr);
        auto future = ExecutionChannel(std::move(connection)).ExecuteAsync("ls -al", std::cout, std::cerr);
        connection = std::move(future.get());

        SftpChannel fileChannel(std::move(connection));
        fileChannel.MakeDirectory("foo", 0777);
        fileChannel.MakeDirectory("bar", 0777, &IgnoreAlreadyExists);

        auto stream = fileChannel.OpenFile("foo.txt", 0666, FileAccessMode::ReadWrite);
        stream = fileChannel.OpenFile("bar.txt", 0666, FileAccessMode::WriteOnly, FileTruncation::Truncate, FileExistenceRequirement::MustExist);
        stream.Read(std::cout);
        stream.Write(std::cin);
        stream = std::move(*stream.ReadAsync(std::cout).get());
        stream = std::move(*stream.WriteAsync(std::cin).get());

        connection.reset(); // connection is the last owner 
#endif
    }
    catch (std::runtime_error const& ex)
    {
        std::cerr << ex.what() << '\n';
        return 1;
    }

    std::cout << "Done\n";
}
