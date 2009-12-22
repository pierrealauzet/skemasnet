
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
  memberList = new list<SKMNAgent*>;  
  watingToJoin = new list<SKMNAgent*>;
  merge_cnt=0;

  addMember(this);
}


int SKMNAgent::command(int argc, const char*const* argv)
{	
  if (argc == 3) {
    if (strcmp(argv[1], "join") == 0) {
		
		SKMNAgent *agent_ = (SKMNAgent*) TclObject::lookup(argv[2]);;
		//printf("%d \n", agent_->addr());					  
		requestJoin(agent_->addr());

		list<SKMNAgent*>::iterator it;
		for(it=agent_->memberList->begin(); it!=agent_->memberList->end(); it++)
		{
			addMember(*it);
		}
		addMember(agent_);
		
      return (TCL_OK);
    }	
	if (strcmp(argv[1], "merge") == 0) {

		SKMNAgent *agent_ = (SKMNAgent*) TclObject::lookup(argv[2]);;
		//printf("%d \n", agent_->addr());					  
		list<SKMNAgent*>::iterator it;
		for(it=agent_->memberList->begin(); it!=agent_->memberList->end(); it++)
		{
			addWaitingToJoin(*it);
		}
		agent_->watingToJoin->clear();
		for(it=memberList->begin(); it!=memberList->end(); it++)
		{
			agent_->addWaitingToJoin(*it);
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
		  merge_cnt++;
		  if(merge_cnt==(memberList->size()-1))
		  {
			  if(hdr->requester!=this)
			  {
				  list<SKMNAgent*>::iterator it;
				  for(it=watingToJoin->begin(); it!=watingToJoin->end(); it++)
				  {
					  addMember(*it);
				  }
				  //printMemberList();		  
				  broadcastSessionKey(this, SKMN_JOIN);
				  watingToJoin->clear();
				  //printWatingList();
			  }
			  //handleJoinReq(pkt);
			 // printf("I'm a leader!!\n");
			  merge_cnt=0;
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
		 addMember(hdr->requester);
		 printf("Recv Join request : at %d.%d from %d.%d member#(%d)", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport(), memberList->size());	
		 printMemberList();
		  
		 handleJoinReq(pkt);	
		 Packet::free(pkt);
	  }
	  else
	  {	
		  list<SKMNAgent*>::iterator it;
		  for(it=hdr->requester->memberList->begin(); it!=hdr->requester->memberList->end(); it++)
		  {
			  addMember(*it);
		  }
		  
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
		  printf("Recv Merge request : at %d.%d from %d.%d, member#(%d)", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport(), memberList->size());			 
		  printMemberList();		  
		  //hdr->requester = here_.addr_;
		  requestClaimToBeALeader(pkt, SKMN_CLAIM_MERGE);
		 
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
	hdr->requester = this;

	hdr_cmn* hdr_c = hdr_cmn::access(pkt);
	hdr_c->size() = SIZE_OF_KEY;

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
	hdr->requester = this;
	// Send the packet

	hdr_cmn* hdr_c = hdr_cmn::access(pkt);
	hdr_c->size() = SIZE_OF_KEY*memberList->size();

	send(pkt, 0);

	requestClaimToBeALeader(pkt, SKMN_CLAIM_MERGE);
}

void SKMNAgent::requestLeave()
{
	if(memberList->size()==1) return;
	list<SKMNAgent*>::iterator it;

	for(it = memberList->begin(); it!=memberList->end(); it++)
	{
		if((*it) !=this) break;
	}	

	SKMNAgent *agent = (*it);
	int destAddr = agent->addr();

	// Create a new packet
	Packet* pkt = allocpkt();

	// Access the Ping header for the new packet:
	hdr_skmn* hdr = hdr_skmn::access(pkt);
	hdr_ip * hdrip = hdr_ip::access(pkt);

	hdrip->dst_.addr_ = destAddr;
	hdrip->dst_.port_ = 0;
	hdr->requester = this;

	hdr->messageType = SKMN_LEAVE;				
	hdr->isAck = 0;		
	send(pkt, 0);
}

void SKMNAgent::requestClaimToBeALeader(Packet *pkt, int type)
{
	hdr_skmn* hdr = hdr_skmn::access(pkt);
	hdr_ip * hdrip = hdr_ip::access(pkt);

	list<SKMNAgent*>::iterator it;
	for(it=memberList->begin(); it !=memberList->end(); it++)
	{
		SKMNAgent* agent = *it;
		int destAddr = agent->addr();
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

void SKMNAgent::broadcastSessionKey(SKMNAgent* requester, int type)
{
	list<SKMNAgent *>::iterator it;
	for(it=memberList->begin(); it !=memberList->end(); it++)
	{
		SKMNAgent* agent = *it;
		int destAddr = agent->addr();

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

		hdr_cmn* hdr = hdr_cmn::access(pktret);

		if(type!=SKMN_MERGE)
		{
			if(agent!=requester)
				hdr->size() = SIZE_OF_KEY*2;
			else
				hdr->size() = SIZE_OF_KEY+SIZE_OF_KEY*(memberList->size()-1);
		}
		{
			hdr->size() = SIZE_OF_KEY+SIZE_OF_KEY*(memberList->size()/2);
		}

		// Send the packet
		send(pktret, 0);
	}
}

void SKMNAgent::addMember(SKMNAgent *node)
{
	list<SKMNAgent*>::iterator it;

	for(it= memberList->begin(); it!=memberList->end(); it++)
	{
		if((*it)==node) break;
	}

	if(it==memberList->end())
		memberList->push_back(node);
}

void SKMNAgent::addWaitingToJoin(SKMNAgent *node)
{
	list<SKMNAgent*>::iterator it;

	for(it= watingToJoin->begin(); it!=watingToJoin->end(); it++)
	{
		if((*it)==node) break;
	}

	if(it==watingToJoin->end())
		watingToJoin->push_back(node);
}

void SKMNAgent::printMemberList()
{
	list<SKMNAgent*>::iterator it;

	printf("(");
	for(it=memberList->begin(); it!=memberList->end(); it++)
	{
		SKMNAgent * agent = *it;
		printf("%d, ",agent->addr());
	}
	printf(")\n");
}

void SKMNAgent::printWatingList()
{
	list<SKMNAgent*>::iterator it;

	printf("(");
	for(it=watingToJoin->begin(); it!=watingToJoin->end(); it++)
	{
		SKMNAgent *agent = *it;;
		printf("%d, ", agent->addr());
	}
	printf(")\n");
}