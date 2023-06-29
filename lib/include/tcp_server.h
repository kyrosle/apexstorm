#ifndef __APEXSTORM_TCP_SERVER_H__
#define __APEXSTORM_TCP_SERVER_H__

#include "address.h"
#include "iomanager.h"
#include "noncopyable.h"
#include "socket.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace apexstorm {

// Tcp Server
class TcpServer : public std::enable_shared_from_this<TcpServer>, Noncopyable {
public:
  typedef std::shared_ptr<TcpServer> ptr;

  // Constructor
  // @param: worker        - socket client worker scheduler
  // @param: accept_worker - server socket execute receiving operation scheduler
  TcpServer(
      apexstorm::IOManager *worker = apexstorm::IOManager::GetThis(),
      apexstorm::IOManager *accept_worker = apexstorm::IOManager::GetThis());

  // Destructor
  virtual ~TcpServer();

  // Bind address
  virtual bool bind(apexstorm::Address::ptr addr);
  // Bind address array, return whether is successes, and store the fails
  // address in `fails` array.
  virtual bool bind(const std::vector<apexstorm::Address::ptr> &addrs,
                    std::vector<apexstorm::Address::ptr> &fails);

  // Startup service
  virtual bool start();

  // Stop service
  virtual void stop();

  // Get server name
  std::string getName() const { return m_name; }

  // Set server name
  void setName(const std::string &v) { m_name = v; }

  // Get Read receive timeout
  uint64_t getRecvTimeout() const { return m_recvTimeout; }

  // Set Read receive timeout
  void setRecvTimeout(uint64_t v) { m_recvTimeout = v; }

  // Whether server is stopped
  bool isStop() const { return m_isStop; }

protected:
  // Handle socket client connection.
  virtual void handleClient(Socket::ptr client);

  // Start to accept connection.
  virtual void startAccept(Socket::ptr sock);

private:
  // listen socket array
  std::vector<Socket::ptr> m_socks;
  // scheduler for newly connected socket jobs
  IOManager *m_worker;
  // scheduler for server sockets to receive connections
  IOManager *m_accept_worker;
  // receive timeout
  uint64_t m_recvTimeout;
  // server name
  std::string m_name;
  // whether server is stopped
  bool m_isStop;
};

} // namespace apexstorm

#endif