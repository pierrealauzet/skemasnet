#pragma once

#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "address.h"
#include "ip.h"
#include <list>
#include <string>

#define SKMN_JOIN				0
#define SKMN_LEAVE				1
#define SKMN_MERGE				2
#define SKMN_CLAIM_LEAVE		31
#define SKMN_CLAIM_MERGE		32
#define SKMN_INIT_LEAVE			5

#define SIZE_OF_KEY 64 //size of pubkey in bytes.


class SKMNAgent : public Agent {
 public:
  SKMNAgent();
  
  void broadcastSessionKey(SKMNAgent* requester, int type);
  void requestJoin(int destAddr);
  void requestLeave();
  void requestMerge(int destAddr);
  void requestClaimToBeALeader(Packet* , int type);

  void handleJoinReq(Packet*);
  void handleInitReq(Packet*, int type);
  void handleLeaveReq(Packet*);
  void handleMerge(Packet*);
  void handleClaim(Packet *, int type);

  
  void printMemberList();
  void printWatingList();
  

  int command(int argc, const char*const* argv);
  void recv(Packet*, Handler*); 

  char createPubKey();
  char createSessionKey();

  void addMember(SKMNAgent *node);
  void addWaitingToJoin(SKMNAgent *node);

  list <SKMNAgent*> *memberList;
  list <SKMNAgent*> *watingToJoin;
  int merge_cnt;
};

struct hdr_skmn {
	//0 join
	//1 leave
	//2 merge
	//3 claim to a leader	
	int messageType;
	int isAck;
	SKMNAgent* requester;
	char publicKey;
	char sessionKey;
	// Header access methods
	static int offset_; // required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_skmn* access(const Packet* p) {
		return (hdr_skmn*) p->access(offset_);
	}
};