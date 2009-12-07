#pragma once

#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "address.h"
#include "ip.h"
#include <list>
#include <string>

#define GKA_JOIN		0
#define GKA_LEAVE		1
#define GKA_MERGE		2
#define GKA_CLAIM_JOIN	30
#define GKA_CLAIM_LEAVE	31
#define GKA_CLAIM_MERGE	32
#define GKA_INIT		4
#define GKA_INIT_LEAVE	5


#define SIZE_OF_KEY 64 //size of pubkey in bytes.

class GKAAgent : public Agent {
 public:
  GKAAgent();
  
  void broadcastSessionKey(GKAAgent *requester, int type);
  void requestJoin(int destAddr);
  void requestLeave();
  void requestMerge(int destAddr);
  void requestClaimToBeALeader(Packet* , int type);

  void handleJoinReq(Packet*);
  void handleInitReq(Packet*, int type);
  void handleLeaveReq(Packet*);
  void handleMerging(Packet*);
  void handleClaim(Packet *, int type);

  void printMemberList();  

  int command(int argc, const char*const* argv);
  void recv(Packet*, Handler*); 

  char createPubKey();
  char createSessionKey();

  void addMember(GKAAgent* memberAddr);

  list <GKAAgent*> *memberList;

  int cnt;
};

struct hdr_gka {
	//0 join
	//1 leave
	//2 merge
	//3 claim to a leader	
	int messageType;
	int isAck;
	GKAAgent* requester;
	char publicKey;
	char sessionKey;
	// Header access methods
	static int offset_; // required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_gka* access(const Packet* p) {
		return (hdr_gka*) p->access(offset_);
	}
};