
#include "GKA.h"


int hdr_gka::offset_;
static class GKAHeaderClass : public PacketHeaderClass {
public:
  GKAHeaderClass() : PacketHeaderClass("PacketHeader/GKA", 
					sizeof(hdr_gka)) {
		bind_offset(&hdr_gka::offset_);
  }
} class_gkahdr;


static class GKAClass : public TclClass {
public:
  GKAClass() : TclClass("Agent/GKA") {}
  TclObject* create(int, const char*const*) {
    return (new GKAAgent());
  }
} class_gka;


GKAAgent::GKAAgent() : Agent(PT_GKA)
{
  bind("packetSize_", &size_);  
  memberList = new list<GKAAgent*>;  
  addMember(this);
  cnt=0;
}

int GKAAgent::command(int argc, const char*const* argv)
{	
  if (argc == 3) {
    if (strcmp(argv[1], "join") == 0) {
		
		GKAAgent *agent_ = (GKAAgent*) TclObject::lookup(argv[2]);;
		//printf("%d \n", agent_->addr());					  
		list<GKAAgent*>::iterator it;
		for(it=agent_->memberList->begin(); it!=agent_->memberList->end(); it++)
		{
			addMember(*it);
		}
		requestJoin(agent_->addr());

		
      return (TCL_OK);
    }	
	if (strcmp(argv[1], "merge") == 0) {

		GKAAgent *agent_ = (GKAAgent*) TclObject::lookup(argv[2]);;
		//printf("%d \n", agent_->addr());					  
		
		requestMerge(agent_->addr());

		list<GKAAgent*>::iterator it;
		for(it=agent_->memberList->begin(); it!=agent_->memberList->end(); it++)
		{
			addMember(*it);
		}
		/*for(it=memberList->begin(); it!=memberList->end(); it++)
		{
			agent_->addMember(*it);
		}*/

				
		
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
  // If the command hasn't been processed by GKAAgent()::command,
  // call the command() function for the base class
  return (Agent::command(argc, argv));
}

void GKAAgent::recv(Packet* pkt, Handler*)
{
  // Access the IP header for the received packet:
  hdr_ip* hdrip = hdr_ip::access(pkt);
  // Access the Ping header for the received packet:
  hdr_gka* hdr = hdr_gka::access(pkt);
  
  switch(hdr->messageType)
  {
  case GKA_CLAIM_JOIN:
	  if(hdr->isAck==0)
	  {
		  printf("Recv Claim_Join request : at %d.%d from %d.%d member#(%d)", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport(), memberList->size());
		  printMemberList();
		  handleClaim(pkt, GKA_CLAIM_JOIN);
	  }
	  else
	  {
		  printf("Recv Claim_Join Ack : at %d.%d from %d.%d member#(%d)", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport(), memberList->size());	
		  printMemberList();		  
		  cnt++;
		  if(cnt==(memberList->size()-2))
		  {
			handleJoinReq(pkt);
			cnt=0;
		  }
	  }
	  break;
  case GKA_CLAIM_MERGE:
	  if(hdr->isAck==0)
	  {
		  printf("Recv Claim_Merge request : at %d.%d from %d.%d member#(%d)", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport(), memberList->size());			 
		  printMemberList();
		  handleClaim(pkt, GKA_CLAIM_MERGE);
	  }
	  else
	  {
		  printf("Recv Claim_Merge Ack : at %d.%d from %d.%d member#(%d)", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport(), memberList->size());	
		  printMemberList();		  
		  cnt++;
		  if(cnt==(memberList->size()-1))
		  {
			  list<GKAAgent*>::iterator it;
			  for(it=hdr->requester->memberList->begin(); it!=hdr->requester->memberList->end(); it++)
			  {
				  addMember(*it);
			  }
			  hdr->requester =this;
			  handleJoinReq(pkt);
			  cnt=0;
		  }
	  }
	  break;
  case GKA_CLAIM_LEAVE:
	  if(hdr->isAck==0)
	  {
		  printf("Recv Claim_Leave request : at %d.%d from %d.%d member#(%d)", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport(), memberList->size());	
		  printMemberList();
		  handleClaim(pkt, GKA_CLAIM_LEAVE);
	  }
	  else
	  {
		  printf("Recv Claim_Leave Ack : at %d.%d from %d.%d member#(%d)", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport(), memberList->size());	
		  printMemberList();		  
		  cnt++;
		  if(cnt==(memberList->size()-1))
		  {
			  handleLeaveReq(pkt);
			  cnt=0;
		  }
	  }
	  break;
  case GKA_JOIN:
	  if(hdr->isAck==0)
	  {		  
		  printf("Recv Join request : at %d.%d from %d.%d member#(%d)", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport(), memberList->size());	
		  printMemberList();
		  
		  if(memberList->size()>2)
		  {
			requestClaimToBeALeader(pkt, GKA_CLAIM_JOIN);
			addMember(hdr->requester);
		  }
		  else{
			  addMember(hdr->requester);
			  handleJoinReq(pkt);
		  }	   

		Packet::free(pkt);
	  }
	  else
	  {	  
		  if(hdr->requester!=this && hdr->requester)
		  {
			  list<GKAAgent*>::iterator it;
			  for(it=hdr->requester->memberList->begin(); it!=hdr->requester->memberList->end(); it++)
			  {
				  addMember(*it);
			  }
		  }
		  printf("Recv Join Ack: at %d.%d from %d.%d, member#(%d)", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport(), memberList->size());
		  printMemberList();

		  Packet::free(pkt);
	  }
	  break;
  case GKA_LEAVE:
	  if(hdr->isAck==0)
	  {
		  memberList->remove(hdr->requester);
		  printf("Recv Leave request : at %d.%d from %d.%d member#(%d) ", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport(), memberList->size());		
		  printMemberList();
		  //handleLeaveReq(pkt);
		  requestClaimToBeALeader(pkt, GKA_CLAIM_LEAVE);
		  Packet::free(pkt);
	  }
	  else
	  {
		  memberList->remove(hdr->requester);
		  printf("Recv Leave Ack : at %d.%d from %d.%d member#(%d)", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport(), memberList->size());		
		  printMemberList();

		  Packet::free(pkt);
	  }
	  break;
  case GKA_MERGE:
	  if(hdr->isAck==0)
	  {
		  printf("Recv Merge request : at %d.%d from %d.%d member#(%d), ", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport(), memberList->size());			 
		  printMemberList();
		  cnt++;
		  if(cnt == (hdr->requester->memberList->size()))
		  {
			  requestClaimToBeALeader(pkt, GKA_CLAIM_MERGE);
			  cnt=0;
		  }
		  
		 
		  Packet::free(pkt);
	  }
	  else
	  {		  
		  printf("Recv Merge Ack: at %d.%d from %d.%d member#(%d)", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport(), memberList->size());
		  printMemberList();

		  Packet::free(pkt);
	  }
	  break;   
  }
}


void GKAAgent::requestJoin(int destAddr)
{
	// Create a new packet
	Packet* pkt = allocpkt();

	// Access the Ping header for the new packet:
	hdr_gka* hdr = hdr_gka::access(pkt);
	hdr_ip * hdrip = hdr_ip::access(pkt);

	hdrip->dst_.addr_ = destAddr;
	hdrip->dst_.port_ = 0;

	hdr->messageType = GKA_JOIN;
	hdr->publicKey = createPubKey();
	// Store the current time in the 'send_time' field
	hdr->isAck = 0;
	hdr->requester = this;
	// Send the packet	

	send(pkt, 0);
}

void GKAAgent::requestMerge(int destAddr)
{
	list<GKAAgent*>::iterator it;

	for(it = memberList->begin(); it!= memberList->end(); it++)
	{
		GKAAgent *agent = *it;
		// Create a new packet
		Packet* pkt = allocpkt();

		// Access the Ping header for the new packet:
		hdr_gka* hdr = hdr_gka::access(pkt);
		hdr_ip * hdrip = hdr_ip::access(pkt);

		hdrip->dst_.addr_ = destAddr;
		hdrip->dst_.port_ = 0;
		hdrip->src_.addr_ = agent->addr();
		hdrip->src_.port_ = 0;

		hdr->messageType = GKA_MERGE;
		hdr->publicKey = createPubKey();
		// Store the current time in the 'send_time' field
		hdr->isAck = 0;
		hdr->requester = agent;
		// Send the packet
		send(pkt, 0);
	}
}

void GKAAgent::requestLeave()
{
	list<GKAAgent*>::iterator it;
	for(it = memberList->begin(); it!=memberList->end(); it++)
	{
		if((*it) !=this) break;
	}	

	if(it!=memberList->end())
	{
		GKAAgent * agent = *it;
		int destAddr = agent->addr();

		// Create a new packet
		Packet* pkt = allocpkt();

		// Access the Ping header for the new packet:
		hdr_gka* hdr = hdr_gka::access(pkt);
		hdr_ip * hdrip = hdr_ip::access(pkt);

		hdrip->dst_.addr_ = destAddr;
		hdrip->dst_.port_ = 0;
		hdr->requester = this;

		hdr->messageType = GKA_LEAVE;				
		hdr->isAck = 0;			

		send(pkt, 0);
	}
}

void GKAAgent::requestClaimToBeALeader(Packet *pkt, int type)
{
	hdr_gka* hdr = hdr_gka::access(pkt);
	hdr_ip * hdrip = hdr_ip::access(pkt);

	list<GKAAgent*>::iterator it;
	for(it=memberList->begin(); it !=memberList->end(); it++)
	{
		GKAAgent* agent = *it;
		int destAddr = agent->addr();
		if(destAddr==here_.addr_) continue;
	
		Packet* pktret = allocpkt();

		hdr_ip* hdripret = hdr_ip::access(pktret);
		hdr_gka* hdrret = hdr_gka::access(pktret);

		hdripret->dst_.addr_ = destAddr;
		hdripret->dst_.port_ = 0;

		hdrret->requester = hdr->requester;

		hdrret->messageType = type;			
		hdrret->isAck=0;		
		// Send the packet
		send(pktret, 0);
	}	
}

void GKAAgent::handleClaim(Packet * pkt, int type)
{
	// Access the IP header for the received packet:
	hdr_ip* hdrip = hdr_ip::access(pkt);	
	hdr_gka *hdr = hdr_gka::access(pkt);

	// Create a new packet
	Packet* pktret = allocpkt();

	// Access the Ping header for the new packet:
	hdr_gka* hdrret = hdr_gka::access(pktret);
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

void GKAAgent::handleInitReq(Packet *pkt, int type)
{
	// Access the IP header for the received packet:
	hdr_ip* hdrip = hdr_ip::access(pkt);	

	// Create a new packet
	Packet* pktret = allocpkt();

	// Access the Ping header for the new packet:
	hdr_gka* hdrret = hdr_gka::access(pktret);
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

void GKAAgent::handleJoinReq(Packet *pkt)
{
	// Access the IP header for the received packet:
	hdr_ip* hdrip = hdr_ip::access(pkt);
	// Access the Ping header for the received packet:
	hdr_gka* hdr = hdr_gka::access(pkt);	

	if(memberList->size()==1)
	{
		Packet* pktret = allocpkt();
		hdr_gka* hdrret = hdr_gka::access(pktret);
		hdr_ip* hdripret = hdr_ip::access(pktret);
		
		hdripret->dst_.addr_ = hdr->requester->addr();
		hdripret->dst_.port_ = 0;

		hdrret->sessionKey = createSessionKey();
		hdrret->messageType = GKA_JOIN;
		hdrret->isAck = 1;
		hdrret->requester = hdr->requester;

		hdr_cmn* hdr = hdr_cmn::access(pktret);
		hdr->size() = SIZE_OF_KEY;

		send(pktret, 0);
	}
	else
	{
		broadcastSessionKey(hdr->requester, GKA_JOIN);		  		
	}	
}

void GKAAgent::handleLeaveReq(Packet* pkt)
{
	hdr_ip* hdrip = hdr_ip::access(pkt);	
	hdr_gka* hdr = hdr_gka::access(pkt);	

	memberList->remove(hdr->requester);

	if(memberList->size()==1)
	{
		Packet* pktret = allocpkt();
		hdr_gka* hdrret = hdr_gka::access(pktret);
		hdr_ip* hdripret = hdr_ip::access(pktret);

		hdripret->dst_.addr_ = hdr->requester->addr();
		hdripret->dst_.port_ = 0;

		hdrret->sessionKey = createSessionKey();
		hdrret->messageType = GKA_LEAVE;
		hdrret->isAck = 1;

		send(pktret, 0);
	}
	else
	{
		broadcastSessionKey(hdr->requester, GKA_LEAVE);
	}
}

char GKAAgent::createPubKey()
{
	return 'p';
}

char GKAAgent::createSessionKey()
{
	return 'c';
}

void GKAAgent::broadcastSessionKey(GKAAgent *requester, int type)
{
	list<GKAAgent*>::iterator it;
	for(it=memberList->begin(); it !=memberList->end(); it++)
	{
		GKAAgent* agent = *it;
		int destAddr = agent->addr();

		if(destAddr == here_.addr_) continue;
		// Create a new packet
		Packet* pktret = allocpkt();

		hdr_ip *hdrip = hdr_ip::access(pktret);
		hdrip->dst_.addr_ = destAddr;
		hdrip->dst_.port_ = 0;

		// Access the Ping header for the new packet:
		hdr_gka* hdrret = hdr_gka::access(pktret);
		// Set the 'ret' field to 1, so the receiver won't send another echo
		hdrret->messageType = type;			
		hdrret->isAck=1;
		hdrret->sessionKey = createSessionKey();
		hdrret->requester = requester;
		// Send the packet

		hdr_cmn* hdr = hdr_cmn::access(pktret);
		hdr->size() = SIZE_OF_KEY*memberList->size()*2;

		send(pktret, 0);
	}
}

void GKAAgent::addMember(GKAAgent* node)
{
	list<GKAAgent*>::iterator it;

	for(it= memberList->begin(); it!=memberList->end(); it++)
	{
		if((*it)==node) break;
	}

	if(it==memberList->end())
		memberList->push_back(node);
}


void GKAAgent::printMemberList()
{
	list<GKAAgent*>::iterator it;

	printf("(");
	for(it=memberList->begin(); it!=memberList->end(); it++)
	{
		GKAAgent * agent = *it;
		printf("%d, ",agent->addr());
	}
	printf(")\n");
}