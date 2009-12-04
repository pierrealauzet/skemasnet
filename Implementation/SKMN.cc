
#include "SKMN.h"


int hdr_skmn::offset_;
static class SKMNHeaderClass : public PacketHeaderClass {
public:
  SKMNHeaderClass() : PacketHeaderClass("PacketHeader/SKMN", 
					sizeof(hdr_skmn)) {
		bind_offset(&hdr_skmn::offset_);
  }
} class_skmnhdr;


static class SKMNClass : public TclClass {
public:
  SKMNClass() : TclClass("Agent/SKMN") {}
  TclObject* create(int, const char*const*) {
    return (new SKMNAgent());
  }
} class_skmn;


SKMNAgent::SKMNAgent() : Agent(PT_SKMN)
{
  bind("packetSize_", &size_);  
  memberList = new list<int>;  
}


int SKMNAgent::command(int argc, const char*const* argv)
{	
  if (argc == 3) {
    if (strcmp(argv[1], "join") == 0) {
		
		SKMNAgent *agent_ = (SKMNAgent*) TclObject::lookup(argv[2]);;
		//printf("%d \n", agent_->addr());					  
		requestJoin(agent_->addr());

		list<int>::iterator it;
		for(it=agent_->memberList->begin(); it!=agent_->memberList->end(); it++)
		{
			addMember(*it);
		}
		addMember(agent_->addr());
		addMember(here_.addr_);
		
      return (TCL_OK);
    }	
	if (strcmp(argv[1], "merge") == 0) {

		SKMNAgent *agent_ = (SKMNAgent*) TclObject::lookup(argv[2]);;
		//printf("%d \n", agent_->addr());					  
		list<int>::iterator it;
		for(it=agent_->memberList->begin(); it!=agent_->memberList->end(); it++)
		{
			addMember(*it);
		}
		agent_->memberList->clear();
		for(it=memberList->begin(); it!=memberList->end(); it++)
		{
			agent_->addMember(*it);
		}		

		requestMerge(agent_->addr());		
		
		return (TCL_OK);
	}	
  }
  if(argc==2)
  {
	  if (strcmp(argv[1], "leave") == 0) {		  
		  
		  requestLeave();		  
		  return (TCL_OK);
	  }
  }
  // If the command hasn't been processed by SKMNAgent()::command,
  // call the command() function for the base class
  return (Agent::command(argc, argv));
}

void SKMNAgent::recv(Packet* pkt, Handler*)
{
  // Access the IP header for the received packet:
  hdr_ip* hdrip = hdr_ip::access(pkt);
  // Access the Ping header for the received packet:
  hdr_skmn* hdr = hdr_skmn::access(pkt);
  
  switch(hdr->messageType)
  {  
  case SKMN_CLAIM_MERGE:
	  if(hdr->isAck==0)
	  {
		  printf("Recv Claim_Merge request : at %d.%d from %d.%d member#(%d)", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport(), memberList->size());	
		  printMemberList();
		  handleClaim(pkt, SKMN_CLAIM_MERGE);
	  }
	  else
	  {
		  printf("Recv Claim_Merge Ack : at %d.%d from %d.%d member#(%d)", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport(), memberList->size());	
		  printMemberList();
		  static int cnt=1;
		  cnt++;
		  if(cnt==memberList->size())
		  {
			  handleJoinReq(pkt);
			  cnt=1;
		  }
	  }
	  break;
  case SKMN_CLAIM_LEAVE:
	  if(hdr->isAck==0)
	  {
		  printf("Recv Claim_Leave request : at %d.%d from %d.%d member#(%d)", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport(), memberList->size());			 
		  printMemberList();
		  handleClaim(pkt, SKMN_CLAIM_LEAVE);
	  }
	  else
	  {
		  printf("Recv Claim_Leave Ack : at %d.%d from %d.%d member#(%d)", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport(), memberList->size());	
		  printMemberList();
		  static int cnt=1;
		  cnt++;
		  if(cnt==memberList->size())
		  {
			  handleLeaveReq(pkt);
			  cnt=1;
		  }
	  }
	  break;
  case SKMN_JOIN:
	  if(hdr->isAck==0)
	  {
		 addMember(this->addr());
		 addMember(hdr->requester);
		 printf("Recv Join request : at %d.%d from %d.%d member#(%d)", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport(), memberList->size());	
		 printMemberList();
		  
		 handleJoinReq(pkt);	
		 Packet::free(pkt);
	  }
	  else
	  {		  
		  addMember(hdr->requester);
		  printf("Recv new Session key: at %d.%d from %d.%d, member#(%d)", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport(), memberList->size());		  
		  printMemberList();
		  Packet::free(pkt);
	  }
	  break;
  case SKMN_LEAVE:
	  if(hdr->isAck==0)
	  {
		  memberList->remove(hdr->requester);
		  printf("Recv Leave request : at %d.%d from %d.%d", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport());	
		  printMemberList();
		  //handleLeaveReq(pkt);
		  requestClaimToBeALeader(pkt, SKMN_CLAIM_LEAVE);
		  Packet::free(pkt);
	  }
	  else
	  {
		  memberList->remove(hdr->requester);
		  printf("Recv new Session Key : at %d.%d from %d.%d", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport());		  
		  printMemberList();
		  Packet::free(pkt);
	  }
	  break;
  case SKMN_MERGE:
	  if(hdr->isAck==0)
	  {
		  printf("Recv Merge request : at %d.%d from %d.%d", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport());			 
		  requestClaimToBeALeader(pkt, SKMN_CLAIM_MERGE);
		  
		  addMember(this->addr());		 
		  Packet::free(pkt);
	  }
	  else
	  {		  
		  printf("Recv Merge Ack: at %d.%d from %d.%d", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport());
		  Packet::free(pkt);
	  }
	  break;    
  }
}


void SKMNAgent::requestJoin(int destAddr)
{
	// Create a new packet
	Packet* pkt = allocpkt();

	// Access the Ping header for the new packet:
	hdr_skmn* hdr = hdr_skmn::access(pkt);
	hdr_ip * hdrip = hdr_ip::access(pkt);

	hdrip->dst_.addr_ = destAddr;
	hdrip->dst_.port_ = 0;

	hdr->messageType = SKMN_JOIN;
	hdr->publicKey = createPubKey();
	// Store the current time in the 'send_time' field
	hdr->isAck = 0;
	hdr->requester =here_.addr_;
	// Send the packet
	send(pkt, 0);
}

void SKMNAgent::requestMerge(int destAddr)
{
	// Create a new packet
	Packet* pkt = allocpkt();

	// Access the Ping header for the new packet:
	hdr_skmn* hdr = hdr_skmn::access(pkt);
	hdr_ip * hdrip = hdr_ip::access(pkt);

	hdrip->dst_.addr_ = destAddr;
	hdrip->dst_.port_ = 0;

	hdr->messageType = SKMN_MERGE;
	hdr->publicKey = createPubKey();
	// Store the current time in the 'send_time' field
	hdr->isAck = 0;
	hdr->requester = here_.addr_;
	// Send the packet
	send(pkt, 0);
}

void SKMNAgent::requestLeave()
{
	list<int>::iterator it;
	for(it = memberList->begin(); it!=memberList->end(); it++)
	{
		if((*it) !=here_.addr_) break;
	}	

	if(it!=memberList->end())
	{
		int destAddr = *it;

		// Create a new packet
		Packet* pkt = allocpkt();

		// Access the Ping header for the new packet:
		hdr_skmn* hdr = hdr_skmn::access(pkt);
		hdr_ip * hdrip = hdr_ip::access(pkt);

		hdrip->dst_.addr_ = destAddr;
		hdrip->dst_.port_ = 0;
		hdr->requester = here_.addr_;

		hdr->messageType = SKMN_LEAVE;				
		hdr->isAck = 0;		
		send(pkt, 0);
	}
}

void SKMNAgent::requestClaimToBeALeader(Packet *pkt, int type)
{
	hdr_skmn* hdr = hdr_skmn::access(pkt);
	hdr_ip * hdrip = hdr_ip::access(pkt);

	list<int>::iterator it;
	for(it=memberList->begin(); it !=memberList->end(); it++)
	{
		int destAddr = *it;
		if(destAddr==here_.addr_) continue;
	
		Packet* pktret = allocpkt();

		hdr_ip* hdripret = hdr_ip::access(pktret);
		hdr_skmn* hdrret = hdr_skmn::access(pktret);

		hdripret->dst_.addr_ = destAddr;
		hdripret->dst_.port_ = 0;

		hdrret->requester = hdr->requester;

		hdrret->messageType = type;			
		hdrret->isAck=0;		
		// Send the packet
		send(pktret, 0);
	}	
}

void SKMNAgent::handleClaim(Packet * pkt, int type)
{
	// Access the IP header for the received packet:
	hdr_ip* hdrip = hdr_ip::access(pkt);	
	hdr_skmn *hdr = hdr_skmn::access(pkt);

	// Create a new packet
	Packet* pktret = allocpkt();

	// Access the Ping header for the new packet:
	hdr_skmn* hdrret = hdr_skmn::access(pktret);
	hdr_ip* hdripret = hdr_ip::access(pktret);

	hdripret->dst_.addr_ = hdrip->saddr();
	hdripret->dst_.port_ = 0;

	hdrret->messageType = type;
	hdrret->publicKey = createPubKey();
	// Store the current time in the 'send_time' field
	hdrret->isAck = 1;
	hdrret->requester = hdr->requester;
	// Send the packet
	send(pktret, 0);
}

void SKMNAgent::handleInitReq(Packet *pkt, int type)
{
	// Access the IP header for the received packet:
	hdr_ip* hdrip = hdr_ip::access(pkt);	

	// Create a new packet
	Packet* pktret = allocpkt();

	// Access the Ping header for the new packet:
	hdr_skmn* hdrret = hdr_skmn::access(pktret);
	hdr_ip* hdripret = hdr_ip::access(pktret);

	hdripret->dst_.addr_ = hdrip->saddr();
	hdripret->dst_.port_ = 0;

	hdrret->messageType = type;
	hdrret->publicKey = createPubKey();
	// Store the current time in the 'send_time' field
	hdrret->isAck = 1;
	// Send the packet
	send(pktret, 0);
}

void SKMNAgent::handleJoinReq(Packet *pkt)
{
	// Access the IP header for the received packet:
	hdr_ip* hdrip = hdr_ip::access(pkt);
	// Access the Ping header for the received packet:
	hdr_skmn* hdr = hdr_skmn::access(pkt);	
	
	broadcastSessionKey(hdr->requester, SKMN_JOIN);	
}

void SKMNAgent::handleLeaveReq(Packet* pkt)
{
	hdr_ip* hdrip = hdr_ip::access(pkt);	
	hdr_skmn* hdr = hdr_skmn::access(pkt);	

	broadcastSessionKey(hdr->requester, SKMN_LEAVE);
}

char SKMNAgent::createPubKey()
{
	return 'p';
}

char SKMNAgent::createSessionKey()
{
	return 'c';
}

void SKMNAgent::broadcastSessionKey(int requester, int type)
{
	list<int>::iterator it;
	for(it=memberList->begin(); it !=memberList->end(); it++)
	{
		int destAddr = *it;

		if(destAddr == here_.addr_) continue;
		// Create a new packet
		Packet* pktret = allocpkt();

		hdr_ip *hdrip = hdr_ip::access(pktret);
		hdrip->dst_.addr_ = destAddr;
		hdrip->dst_.port_ = 0;

		// Access the Ping header for the new packet:
		hdr_skmn* hdrret = hdr_skmn::access(pktret);
		// Set the 'ret' field to 1, so the receiver won't send another echo
		hdrret->messageType = type;			
		hdrret->isAck=1;
		hdrret->sessionKey = createSessionKey();
		hdrret->requester = requester;
		// Send the packet
		send(pktret, 0);
	}
}

void SKMNAgent::addMember(int memberAddr)
{
	list<int>::iterator it;

	for(it= memberList->begin(); it!=memberList->end(); it++)
	{
		if((*it)==memberAddr) break;
	}

	if(it==memberList->end())
		memberList->push_back(memberAddr);
}


void SKMNAgent::printMemberList()
{
	list<int>::iterator it;

	printf("(");
	for(it=memberList->begin(); it!=memberList->end(); it++)
	{
		printf("%d, ", *it);
	}
	printf(")\n");
}