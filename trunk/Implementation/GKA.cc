
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
  memberList = new list<int>;  
}


int GKAAgent::command(int argc, const char*const* argv)
{	
  if (argc == 3) {
    if (strcmp(argv[1], "join") == 0) {
		
		GKAAgent *agent_ = (GKAAgent*) TclObject::lookup(argv[2]);;
		//printf("%d \n", agent_->addr());					  
		requestJoin(agent_->addr());

		list<int>::iterator it;
		for(it=agent_->memberList->begin(); it!=agent_->memberList->end(); it++)
		{
			addMember(*it);//memberList->push_back(*it);
		}
		addMember(agent_->addr());
		addMember(here_.addr_);
		//memberList->push_back(agent_->addr());
		//memberList->push_back(here_.addr_);
      // return TCL_OK, so the calling function knows that the
      // command has been processed
      return (TCL_OK);
    }	
	if (strcmp(argv[1], "merge") == 0) {

		GKAAgent *agent_ = (GKAAgent*) TclObject::lookup(argv[2]);;
		//printf("%d \n", agent_->addr());					  
		list<int>::iterator it;
		for(it=agent_->memberList->begin(); it!=agent_->memberList->end(); it++)
		{
			addMember(*it);//memberList->push_back(*it);
		}
		agent_->memberList->clear();
		for(it=memberList->begin(); it!=memberList->end(); it++)
		{
			agent_->addMember(*it);//push_back(*it);
		}		

		requestMerge(agent_->addr());
		
		//memberList->push_back(agent_->addr());
		// return TCL_OK, so the calling function knows that the
		// command has been processed
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
  case GKA_JOIN:
	  if(hdr->isAck==0)
	  {
		 printf("Recv Join request : at %d.%d from %d.%d\n", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport());			 
		 handleJoinReq(pkt);

		 addMember(this->addr());
		 /*list<int>::iterator it;
		 
		 for(it=memberList->begin(); it!=memberList->end(); it++)
		 {
			 if(*it == this->addr()) break;
		 }
		 if(it==memberList->end())
			 memberList->push_back(this->addr());*/
		 Packet::free(pkt);
	  }
	  else
	  {		  
		  printf("Recv Join Ack: at %d.%d from %d.%d, member#(%d)\n", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport(), memberList->size());

		  Packet::free(pkt);
	  }
	  break;
  case GKA_LEAVE:
	  if(hdr->isAck==0)
	  {
		  printf("Recv Leave request : at %d.%d from %d.%d\n", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport());			 
		  handleLeaveReq(pkt);
		  Packet::free(pkt);
	  }
	  else
	  {
		  printf("Recv Leave Ack : at %d.%d from %d.%d\n", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport());
		  memberList->clear();
		  Packet::free(pkt);
	  }
	  break;
  case GKA_MERGE:
	  if(hdr->isAck==0)
	  {
		  printf("Recv Merge request : at %d.%d from %d.%d\n", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport());			 
		  handleJoinReq(pkt);
		  
		  addMember(this->addr());		 
		  Packet::free(pkt);
	  }
	  else
	  {		  
		  printf("Recv Merge Ack: at %d.%d from %d.%d\n", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport());
		  Packet::free(pkt);
	  }
	  break;  
  case GKA_INIT:
	  if(hdr->isAck==0)
	  {
		  printf("Recv Init request : at %d.%d from %d.%d\n", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport());		
		  addMember(hdr->requester);
		  
		  handleInitReq(pkt, GKA_INIT);

		  Packet::free(pkt);
	  }else
	  {
		  static int cnt=2;
		  cnt++;
		  printf("Recv Init Ack : at %d.%d from %d.%d member#(%d) \n", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport(), memberList->size());		  
		  if(cnt==memberList->size())
		  {			  
			  boradcastSessionKey();
			  cnt=2;
		  }
		  Packet::free(pkt);
	  }
	break;
  case GKA_INIT_LEAVE:
	  if(hdr->isAck==0)
	  {
		  printf("Recv Init leave request : at %d.%d from %d.%d\n", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport());
		  
		  memberList->remove(hdr->requester);
		  handleInitReq(pkt, GKA_INIT_LEAVE);
		  
		  Packet::free(pkt);
	  }else
	  {
		  static int cnt2=1;
		  cnt2++;
		  printf("Recv Init leave Ack : at %d.%d from %d.%d member#(%d) \n", here_.addr_, here_.port_, hdrip->saddr(), hdrip->sport(), memberList->size());		  
		  if(cnt2==memberList->size())
		  {			  
			  boradcastSessionKey();
			  cnt2=1;
		  }
		  Packet::free(pkt);
	  }
	  break;
  }
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
	// Send the packet
	send(pkt, 0);
}

void GKAAgent::requestMerge(int destAddr)
{
	// Create a new packet
	Packet* pkt = allocpkt();

	// Access the Ping header for the new packet:
	hdr_gka* hdr = hdr_gka::access(pkt);
	hdr_ip * hdrip = hdr_ip::access(pkt);

	hdrip->dst_.addr_ = destAddr;
	hdrip->dst_.port_ = 0;

	hdr->messageType = GKA_MERGE;
	hdr->publicKey = createPubKey();
	// Store the current time in the 'send_time' field
	hdr->isAck = 0;
	// Send the packet
	send(pkt, 0);
}

void GKAAgent::requestLeave()
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
		hdr_gka* hdr = hdr_gka::access(pkt);
		hdr_ip * hdrip = hdr_ip::access(pkt);

		hdrip->dst_.addr_ = destAddr;
		hdrip->dst_.port_ = 0;

		hdr->messageType = GKA_LEAVE;				
		hdr->isAck = 0;		
		send(pkt, 0);
	}
}

void GKAAgent::handleJoinReq(Packet *pkt)
{
	// Access the IP header for the received packet:
	hdr_ip* hdrip = hdr_ip::access(pkt);
	// Access the Ping header for the received packet:
	hdr_gka* hdr = hdr_gka::access(pkt);	

	if(memberList->size()==0)
	{
		Packet* pktret = allocpkt();
		hdr_gka* hdrret = hdr_gka::access(pktret);
		hdr_ip* hdripret = hdr_ip::access(pktret);

		hdripret->dst_.addr_ = hdrip->saddr();
		hdripret->dst_.port_ = 0;

		hdrret->sessionKey = createSessionKey();
		hdrret->messageType = GKA_JOIN;
		hdrret->isAck = 1;

		send(pktret, 0);
	}
	else
	{
		list<int>::iterator it;
		for(it=memberList->begin(); it !=memberList->end(); it++)
		{
			int destAddr = *it;
			if(destAddr==here_.addr_) continue;
			// Create a new packet
			Packet* pktret = allocpkt();

			hdr_ip* hdripret = hdr_ip::access(pktret);
			hdr_gka* hdrret = hdr_gka::access(pktret);

			hdripret->dst_.addr_ = destAddr;
			hdripret->dst_.port_ = 0;
			
			hdrret->messageType = GKA_INIT;			
			hdrret->isAck=0;
			hdrret->publicKey = hdr->publicKey;
			hdrret->requester = hdrip->saddr();
			// Send the packet
			send(pktret, 0);
		}		  		
	}
	addMember(hdrip->saddr());
	/*list<int>::iterator it;
	
	for(it = memberList->begin(); it!=memberList->end(); it++)
	{
		if(*it == hdrip->saddr())return;
	}

	memberList->push_back(hdrip->saddr());	*/
}

void GKAAgent::handleLeaveReq(Packet* pkt)
{
	hdr_ip* hdrip = hdr_ip::access(pkt);	
	hdr_gka* hdr = hdr_gka::access(pkt);	

	memberList->remove(hdrip->saddr());

	if(memberList->size()==0)
	{
		Packet* pktret = allocpkt();
		hdr_gka* hdrret = hdr_gka::access(pktret);
		hdr_ip* hdripret = hdr_ip::access(pktret);

		hdripret->dst_.addr_ = hdrip->saddr();
		hdripret->dst_.port_ = 0;

		hdrret->sessionKey = createSessionKey();
		hdrret->messageType = GKA_LEAVE;
		hdrret->isAck = 1;

		send(pktret, 0);
	}
	else
	{
		list<int>::iterator it;
		for(it=memberList->begin(); it !=memberList->end(); it++)
		{
			int destAddr = *it;			
			if(destAddr==here_.addr_) continue;
			// Create a new packet
			Packet* pktret = allocpkt();

			hdr_ip* hdripret = hdr_ip::access(pktret);
			hdr_gka* hdrret = hdr_gka::access(pktret);

			hdripret->dst_.addr_ = destAddr;
			hdripret->dst_.port_ = 0;

			hdrret->messageType = GKA_INIT_LEAVE;
			hdrret->isAck=0;
			hdrret->publicKey = hdr->publicKey;
			hdrret->requester = hdrip->saddr();
			send(pktret, 0);
		}		  		
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

void GKAAgent::boradcastSessionKey()
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
		hdr_gka* hdrret = hdr_gka::access(pktret);
		// Set the 'ret' field to 1, so the receiver won't send another echo
		hdrret->messageType = GKA_JOIN;			
		hdrret->isAck=1;
		hdrret->sessionKey = createSessionKey();
		// Send the packet
		send(pktret, 0);
	}
}

void GKAAgent::addMember(int memberAddr)
{
	list<int>::iterator it;

	for(it= memberList->begin(); it!=memberList->end(); it++)
	{
		if((*it)==memberAddr) break;
	}

	if(it==memberList->end())
		memberList->push_back(memberAddr);
}