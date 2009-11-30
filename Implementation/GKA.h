#pragma once

#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "address.h"
#include "ip.h"
#include <list>
#include <string>

#define GKA_JOIN	0
#define GKA_LEAVE	1
#define GKA_MERGE	2
#define GKA_CLAIM	3
#define GKA_INIT_JOIN	4
#define GKA_INIT_LEAVE 5


struct hdr_gka {
	//0 join
	//1 leave
	//2 merge
	//3 claim to a leader	
	int messageType;
	int isAck;
	int requester;
	char publicKey;
	char sessionKey;
	// Header access methods
	static int offset_; // required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_gka* access(const Packet* p) {
		return (hdr_gka*) p->access(offset_);
	}
};

struct struct_pubkey{
	int node_addr;
	string key;
};

class GKAAgent : public Agent {
 public:
  GKAAgent();
  
  void boradcastSessionKey();
  void requestJoin(int destAddr);
  void requestLeave();
  void requestMerge(int destAddr);

  void handleJoinReq(Packet*);
  void handleInitReq(Packet*, int type);
  void handleLeaveReq(Packet*);
  void handleMerging(Packet*);

  int command(int argc, const char*const* argv);
  void recv(Packet*, Handler*); 

  char createPubKey();
  char createSessionKey();

  list <int> *memberList;
};
