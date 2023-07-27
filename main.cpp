#include <iostream>
#include <string>
#include <asio.hpp>
#include <asio/io_context.hpp>

using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::ip::tcp;
using asio::use_awaitable;
using asio::io_context;
using namespace std;

awaitable<void> session(tcp::socket socket)
{
    string data;
    cout << "Client connected: " << socket.remote_endpoint() << '\n';
    while (data != "quit")
    {
        data.clear();
        co_await asio::async_read_until(socket, asio::dynamic_buffer(data), '\n', use_awaitable); // line-by-line reading
        if (data != "")
            co_await asio::async_write(socket, asio::buffer(data), use_awaitable);
    }
    cout << "Client disconnected: " << socket.remote_endpoint() << '\n';
}

awaitable<void> listener(io_context& ctx, unsigned short port)
{
    tcp::acceptor acceptor(ctx, { tcp::v4(), port });
    cout << "Server listening on port " << port << "..." << endl;
    while (true)
    {
        tcp::socket socket = co_await acceptor.async_accept(use_awaitable);
        co_spawn(ctx, session(move(socket)), detached);
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        cerr << "Usage: " << argv[0] << " <port>" << endl;
        return 1;
    }

    io_context ctx;
    string arg = argv[1];
    size_t pos;
    unsigned short port = stoi(arg, &pos);

    asio::signal_set signals(ctx, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto) { ctx.stop(); });
    auto listen = listener(ctx, port);
    co_spawn(ctx, move(listen), asio::detached);
    ctx.run();
}
