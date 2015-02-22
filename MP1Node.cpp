/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node (Member *member, Params *params, EmulNet *emul, Log *log, Address *address)
{
  for (int i = 0; i < 6; i++)
    {
      NULLADDR[i] = 0;
    }
  this->memberNode = member;
  this->emulNet = emul;
  this->log = log;
  this->par = params;
  this->memberNode->addr = *address;
  k = 4;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node ()
{
}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int
MP1Node::recvLoop ()
{
  if (memberNode->bFailed)
    {
      return false;
    }
  else
    {
      return emulNet->ENrecv (&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int
MP1Node::enqueueWrapper (void *env, char *buff, int size)
{
  Queue q;
  return q.enqueue ((queue<q_elt> *) env, (void *) buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void
MP1Node::nodeStart (char *servaddrstr, short servport)
{
  Address joinaddr;
  joinaddr = getJoinAddress ();

  // Self booting routines
  if (initThisNode (&joinaddr) == -1)
    {
#ifdef DEBUGLOG
      log->LOG (&memberNode->addr, "init_thisnode failed. Exit.");
#endif
      exit (1);
    }

  if (!introduceSelfToGroup (&joinaddr))
    {
      finishUpThisNode ();
#ifdef DEBUGLOG
      log->LOG (&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
      exit (1);
    }

  return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int
MP1Node::initThisNode (Address *joinaddr)
{
  /*
   * This function is partially implemented and may require changes
   */
  int id = *(int*) (&memberNode->addr.addr);
  int port = *(short*) (&memberNode->addr.addr[4]);

  memberNode->bFailed = false;
  memberNode->inited = true;
  memberNode->inGroup = false;
  // node is up!
  memberNode->nnb = 0;
  memberNode->heartbeat = 0;
  memberNode->pingCounter = TFAIL;
  memberNode->timeOutCounter = -1;
  initMemberListTable (memberNode, id, port);

  return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int
MP1Node::introduceSelfToGroup (Address *joinaddr)
{
  MessageHdr *msg;
#ifdef DEBUGLOG
  static char s[1024];
#endif

  if (0 == strcmp ((char *) &(memberNode->addr.addr), (char *) &(joinaddr->addr)))
    {
      // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
      log->LOG (&memberNode->addr, "Starting up group...");
#endif
      memberNode->inGroup = true;
    }
  else
    {
      size_t msgsize = sizeof(MessageHdr);
      msg = (MessageHdr *) malloc (msgsize * sizeof(char));
      // create JOINREQ message
      msg->msgType = JOINREQ;
      msg->heartbeat = memberNode->heartbeat;
      msg->sender = memberNode->addr;
      msg->memberSize = 0;

#ifdef DEBUGLOG
      sprintf (s, "Trying to join...");
      log->LOG (&memberNode->addr, s);
#endif

      // send JOINREQ message to introducer member
      emulNet->ENsend (&memberNode->addr, joinaddr, (char *) msg, msgsize);

      free (msg);
    }

  return 1;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int
MP1Node::finishUpThisNode ()
{
  /*
   * Your code goes here
   */
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void
MP1Node::nodeLoop ()
{
  if (memberNode->bFailed)
    {
      return;
    }

  // Check my messages
  checkMessages ();

  // Wait until you're in the group...
  if (!memberNode->inGroup)
    {
      return;
    }

  // ...then jump in and share your responsibilites!
  nodeLoopOps ();

  return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void
MP1Node::checkMessages ()
{
  void *ptr;
  int size;

  // Pop waiting messages from memberNode's mp1q
  while (!memberNode->mp1q.empty ())
    {
      ptr = memberNode->mp1q.front ().elt;
      size = memberNode->mp1q.front ().size;
      memberNode->mp1q.pop ();
      recvCallBack ((void *) memberNode, (char *) ptr, size);
    }
  return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool
MP1Node::recvCallBack (void *env, char *data, int size)
{

  MessageHdr* header = reinterpret_cast<MessageHdr*> (data);
  cout << "request " << header->msgType << " from " << header->sender.getAddress () << " to " << memberNode->addr.getAddress ()
      << " with heartbeat " << header->heartbeat << endl;
  //get id,port
  int id = 0;
  short port;
  memcpy (&id, &header->sender.addr, sizeof(int));
  memcpy (&port, &header->sender.addr[4], sizeof(short));
  //get goshipedlist
  std::vector<MemberListEntry> goshiped;
  for (int i = 0; i < header->memberSize; i++)
    {
      MemberListEntry* entry = reinterpret_cast<MemberListEntry*> (data + sizeof(MessageHdr) + (i * sizeof(MemberListEntry)));
      goshiped.insert (goshiped.begin (), *entry);
    }

  if (header->msgType == JOINREQ)
    {
      //add to membershiplist
      MemberListEntry* entry = new MemberListEntry (id, port, header->heartbeat, par->getcurrtime ());
      memberNode->memberList.insert (memberNode->memberList.end (), *entry);

      // create JOINREQP message
      size_t msgsize = sizeof(MessageHdr) + (memberNode->memberList.size () * sizeof(MemberListEntry));
      MessageHdr* msg = (MessageHdr *) malloc (msgsize * sizeof(char));
      this->createMessage (JOINREP, msg);
      this->fillMemberList (msg);

      emulNet->ENsend (&memberNode->addr, &header->sender, (char *) msg, msgsize);

      free (msg);
    }
  else if (header->msgType == JOINREP)
    {

      for (std::vector<MemberListEntry>::iterator it = goshiped.begin (); it != goshiped.end (); ++it)
	{
	  memberNode->memberList.insert (memberNode->memberList.end (), *it);
	}

      memberNode->inGroup = true;

    }
  else if (header->msgType == GOSSIP)
    {
      cout << "updating menberlist of node" << memberNode->addr.getAddress () << endl;
      for (std::vector<MemberListEntry>::iterator it = memberNode->memberList.begin (); it != memberNode->memberList.end (); ++it)
	{
	  Address address = parseAddress ((*it).id, (*it).port);
	  if (!(address == memberNode->addr))
	    {
	      for (std::vector<MemberListEntry>::iterator goss = goshiped.begin (); goss != goshiped.end (); ++goss)
		{
		  if ((*it).id == (*goss).id)
		    {
		      MemberListEntry* entry;
		      if ((*it).heartbeat < (*goss).heartbeat)
			{
			  entry = new MemberListEntry ((*it).id, (*it).port, (*goss).heartbeat, par->getcurrtime ());
			}
		    }
		}

	    }

	}
      cout << "searching failed nodes in menberlist of node" << memberNode->addr.getAddress () << endl;
      for (std::vector<MemberListEntry>::iterator it = memberNode->memberList.begin (); it != memberNode->memberList.end (); ++it)
	{
	  int timeout = par->getcurrtime () - (*it).timestamp;
	  if (timeout >= 40)
	    {
	      //send failed messages
	    }
	  else if (timeout >= 80)
	    {
	      //delete from memberlist
	    }
	}

    }

  return true;
}

void
MP1Node::createMessage (MsgTypes type, MessageHdr* msg)
{
  msg->msgType = type;
  msg->heartbeat = memberNode->heartbeat;
  msg->sender = memberNode->addr;
  msg->memberSize = memberNode->memberList.size ();
}

void
MP1Node::fillMemberList (MessageHdr* msg)
{
  int i = 0;
  for (std::vector<MemberListEntry>::iterator it = memberNode->memberList.begin (); it != memberNode->memberList.end (); ++it)
    {
      MemberListEntry entry = *it;
      memcpy (((char*) msg + sizeof(MessageHdr) + (i * sizeof(MemberListEntry))), &entry, sizeof(MemberListEntry));
      i++;
    }
}

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void
MP1Node::gossip ()
{
  //Select random menbers to send gosip
  if (k >= memberNode->memberList.size ())
    {

      for (std::vector<MemberListEntry>::iterator it = memberNode->memberList.begin (); it != memberNode->memberList.end (); ++it)
	{

	  Address address = parseAddress ((*it).id, (*it).port);
	  if (!(address == memberNode->addr))
	    {
	      size_t msgsize = sizeof(MessageHdr) + (memberNode->memberList.size () * sizeof(MemberListEntry));
	      MessageHdr* msg = (MessageHdr *) malloc (msgsize * sizeof(char));
	      this->createMessage (GOSSIP, msg);
	      this->fillMemberList (msg);
	      emulNet->ENsend (&memberNode->addr, &address, (char *) msg, msgsize);
	      cout << "sending " << msg->msgType << " from " << memberNode->addr.getAddress () << " to " << address.getAddress ()
		  << " with heartbeat " << msg->heartbeat << endl;
	      free (msg);

	    }
	}

    }
  else
    {
      int selected = 0;
      int random;
      int selectedTable[memberNode->memberList.size ()];
      for (int i = 0; i < memberNode->memberList.size (); i++)
	{
	  selectedTable[i] = 0;
	}

      std::vector<MemberListEntry>::iterator it = memberNode->memberList.begin ();
      while (selected < k)
	{
	  random = rand () % memberNode->memberList.size ();
	  if (selectedTable[random] != 1)
	    {
	      it = it + random;
	      Address address = parseAddress ((*it).id, (*it).port);
	      if (!(address == memberNode->addr))
		{
		  size_t msgsize = sizeof(MessageHdr) + (memberNode->memberList.size () * sizeof(MemberListEntry));
		  MessageHdr* msg = (MessageHdr *) malloc (msgsize * sizeof(char));
		  this->createMessage (GOSSIP, msg);
		  this->fillMemberList (msg);
		  emulNet->ENsend (&memberNode->addr, &address, (char *) msg, msgsize);
		  cout << "sending random" << msg->msgType << " from " << memberNode->addr.getAddress () << " to " << address.getAddress ()
		      << " with heartbeat " << msg->heartbeat << endl;
		  free (msg);
		}
	      selected = selected + 1;
	      it = memberNode->memberList.begin ();
	      selectedTable[random] = 1;
	    }

	}

    }
}

void
MP1Node::nodeLoopOps ()
{
  gossip ();
  memberNode->heartbeat++;
  return;
}

Address
MP1Node::parseAddress (int id, int port)
{
  Address address;
  memset (&address, 0, sizeof(Address));
  *(int *) (&address.addr) = id;
  *(short *) (&address.addr[4]) = port;
  return address;
}
/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int
MP1Node::isNullAddress (Address *addr)
{
  return (memcmp (addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address
MP1Node::getJoinAddress ()
{
  Address joinaddr;

  memset (&joinaddr, 0, sizeof(Address));
  *(int *) (&joinaddr.addr) = 1;
  *(short *) (&joinaddr.addr[4]) = 0;

  return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void
MP1Node::initMemberListTable (Member *memberNode, int id, int port)
{
  memberNode->memberList.clear ();
  MemberListEntry* myself = new MemberListEntry (id, port, memberNode->heartbeat, par->getcurrtime ());
  memberNode->memberList.insert (memberNode->memberList.end (), *myself);
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void
MP1Node::printAddress (Address *addr)
{
  printf ("%d.%d.%d.%d:%d \n", addr->addr[0], addr->addr[1], addr->addr[2], addr->addr[3], *(short*) &addr->addr[4]);
}
