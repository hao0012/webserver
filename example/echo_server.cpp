#include "tcp_server.h"
#include "logger.h"

#include <string>

class echo_server {
public:
  echo_server(const webserver::inet_address &addr, const std::string &name)
  : server_(addr, name) {
    server_.set_connection_callback([this](const webserver::tcp_connection_ptr& conn) {
      // webserver::logger::info("connection state %d", conn->get_state());
      // if (conn->is_connected()) {
      //   webserver::logger::info("connection from %s, is connected", conn->peer_addr().to_ip_port().c_str());
      // } else {
      //   webserver::logger::info("connection from %s, is closed", conn->peer_addr().to_ip_port().c_str());
      // }
    });

    server_.set_message_callback([this](const webserver::tcp_connection_ptr& conn, webserver::buffer *data, webserver::timestamp receive_time) {
      // webserver::logger::info("run user message_callback");
      conn->send(data->retrieve_all_as_string());
    });

    server_.set_thread_num(1);
  }

  void start() { server_.start(); }
private:
  webserver::tcp_server server_;
};

int main() {
  webserver::inet_address addr(8000, "127.0.0.1");
  echo_server server(addr, "echo_server");
  server.start();
  return 0;
}