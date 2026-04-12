#ifndef __DIRECTORYCONNECTION_H__
#define __DIRECTORYCONNECTION_H__

#include "Common.h"
#include "RawMessageConnection.h"

namespace SMART {

struct RemoteConnection;


// directory thread
struct DirectoryConnection {
  uint16_t dirID;
#ifndef CXL
  RdmaContext ctx;
  ibv_cq *cq;

  RawMessageConnection *message;

  ibv_qp **data2app[MAX_APP_THREAD];

  ibv_mr *dsmMR;
  uint32_t dsmLKey;
#endif
  void *dsmPool;
  uint64_t dsmSize;
  

#ifndef CXL
  ibv_mr *lockMR;
  uint32_t lockLKey;
#endif
  void *lockPool; // address on-chip
  uint64_t lockSize;

  RemoteConnection *remoteInfo;

  DirectoryConnection(uint16_t dirID, void *dsmPool, uint64_t dsmSize,
                      uint32_t machineNR, RemoteConnection *remoteInfo);
#ifndef CXL
  void sendMessage2App(RawMessage *m, uint16_t node_id, uint16_t th_id);
#endif
};

} // namespace SMART

#endif /* __DIRECTORYCONNECTION_H__ */
