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

#include <iostream>
#include <stdexcept>
#include <string>

#include "libssh_cpp_wrap/command_execution_channel.hpp"
#include "libssh_cpp_wrap/connection.hpp"
#include "libssh_cpp_wrap/ip.hpp"
#include "libssh_cpp_wrap/session.hpp"
#include "libssh_cpp_wrap/session_options.hpp"
#include "libssh_cpp_wrap/sftp_channel.hpp"
#include "libssh_cpp_wrap/scp.hpp"

namespace
{

void PrintUsage(std::ostream& out)
{
    out <<
        (
            "Incorrect usage, should be:\n"
            "Example <ip> <user name> <password>\n"
        );
}

constexpr char const SshRemoteDirectory[] = "/home/root";

constexpr char const SshRemoteTestFileName[] = "foo.txt";

}


int main(int argc, char* argv[])
{
    using namespace libssh_wrap;

    if (argc != 3)
    {
        PrintUsage(std::cerr);
        return 1;
    }

    IpV4 ip(argv[1]);

    if (ip == IpV4("0.0.0.0"))
    {
        std::cerr << "Unexpected value passed as ip: " << argv[1] << '\n';
        return 1;
    }

    UserName username = argv[2];

    try
    {
        Session session = Session::Create();
        session.SetOption(ip);
        session.SetOption(Port());
        session.SetOption(username);

        std::string password;
        std::cout << "enter the password\n";
        std::cin >> password;

        auto connection = Connection(std::move(session)).Authenticate(password.c_str());

        //ExecutionChannel(connection).Execute("ls -al", std::cout, std::cerr);

        {
            std::stringstream stream;

            for (int c = 500; c != 0; --c)
            {
                stream << "Hello world!\n";
            }

            ScpSession scp(connection, SshRemoteDirectory, ScpAccessMode::Write);
            scp.WriteFile(SshRemoteTestFileName, stream, stream.tellp(), 0644);
        }

        {
            std::stringstream stream;

            for (int c = 500; c != 0; --c)
            {
                stream << "Hello world!\n";
            }

            ScpSession scp(connection, (std::string(SshRemoteDirectory) + SshRemoteTestFileName).c_str(), ScpAccessMode::Read);
            scp.ReadFile(std::cout);
        }
#if 0
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
