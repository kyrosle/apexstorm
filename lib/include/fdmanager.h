#ifndef __FD_MANAGER_H__
#define __FD_MANAGER_H__

#include "iomanager.h"
#include "mutex.h"
#include "singleton.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace apexstorm {

// identify whether is socket file descriptor, and contains relevant settings
class FdCtx : public std::enable_shared_from_this<FdCtx> {
public:
  typedef std::shared_ptr<FdCtx> ptr;

  // Create by file descriptor.
  FdCtx(int fd);

  // Destructor.
  ~FdCtx();

  // Whether initialization is complete.
  bool isInit() const { return m_isInit; }

  // Whether is as socket file descriptor.
  bool isSocket() const { return m_isSocket; }

  // Whether this file descriptor is closed.
  bool isClosed() const { return m_isClosed; }

  // Set user initiative to set non-blocking.
  void setUserNonblock(bool v) { m_userNonblock = v; }
  // Get whether user intervention to set non-blocking.
  bool getUserNonblock() const { return m_userNonblock; }

  // Set System non-blocking.
  void setSysNonblock(bool v) { m_sysNonblock = v; }
  // Get whether System non-blocking.
  bool getSysNonblock() const { return m_sysNonblock; }

  // Set file descriptor timeout.
  // @param: type - SO_RCVTIMEO(read timeout) / SO_SNDTIMEO(write timeout)
  void setTimeout(int type, uint64_t v);
  // Get file descriptor timeout.
  uint64_t getTimeout(int type);

private:
  // Initialize
  bool init();

private:
  // whether is initialized.
  bool m_isInit : 1;
  // whether is socket.
  bool m_isSocket : 1;
  // whether set hook non-blocking
  bool m_sysNonblock : 1;
  // whether user set non-blocking.
  bool m_userNonblock : 1;
  // whether is closed
  bool m_isClosed : 1;
  // file descriptor
  int m_fd;

  // read timeout
  uint64_t m_recvTimeout;
  // write timeout
  uint64_t m_sendTimeout;
};

class FdManager {
public:
  typedef RWMutex RWMutexType;

  // Constructor
  FdManager();

  // Acquire/create a file descriptor context.
  // @params: auto_create - if the file descriptor is not existed in m_datas,
  // create one and add it to the collection.
  FdCtx::ptr get(int fd, bool auto_create = false);

  // Remove a file descriptor.
  void del(int fd);

private:
  // RW Mutex
  RWMutexType m_mutex;
  // file descriptor collection
  std::vector<FdCtx::ptr> m_datas;
};

// Singleton of FdManager
typedef Singleton<FdManager> FdMgr;
} // namespace apexstorm

#endif