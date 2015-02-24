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
      goshiped.push_back (*entry);
    }

  if (header->msgType == JOINREQ)
    {
      //add to membershiplist
      MemberListEntry entry (id, port, header->heartbeat, par->getcurrtime ());
      memberNode->memberList.push_back (entry);

      Address address = parseAddress (id, port);
      log->logNodeAdd (&memberNode->addr, &address);

      // create JOINREQP message
      size_t msgsize = sizeof(MessageHdr) + (memberNode->memberList.size () * sizeof(MemberListEntry));
      MessageHdr* JOINRPMSG = (MessageHdr *) malloc (msgsize * sizeof(char));
      this->createMessage (JOINREP, JOINRPMSG, memberNode->memberList.size ());
      this->fillMemberList (JOINRPMSG);

      emulNet->ENsend (&memberNode->addr, &header->sender, (char *) JOINRPMSG, msgsize);

      free (JOINRPMSG);
    }
  else if (header->msgType == JOINREP)
    {

      for (std::vector<MemberListEntry>::iterator it = goshiped.begin (); it != goshiped.end (); ++it)
	{
	  memberNode->memberList.push_back (*it);
	  Address address = parseAddress (it->id, it->port);
	  log->logNodeAdd (&memberNode->addr, &address);
	}

      memberNode->inGroup = true;

    }
  else if (header->msgType == GOSSIP)
    {

      cout << "updating menberlist of node " << memberNode->addr.getAddress () << endl;
      bool found = false;
      for (std::vector<MemberListEntry>::iterator goss = goshiped.begin (); goss != goshiped.end (); ++goss)
	{
	  for (std::vector<MemberListEntry>::iterator it = memberNode->memberList.begin (); it != memberNode->memberList.end (); ++it)
	    {
	      Address address = parseAddress (it->id, it->port);
	      if (!(address == memberNode->addr) && it->id == goss->id)
		{

		  found = true;
		  if (it->heartbeat < goss->heartbeat)
		    {
		      cout << "updating node entry " << goss->id << " with " << goss->heartbeat << " heartbeat of node "
			  << memberNode->addr.getAddress () << endl;
		      it->timestamp = par->getcurrtime ();
		      it->heartbeat = goss->heartbeat;
		    }

		}

	    }

	  if (!found)
	    {
	      Address address = parseAddress (goss->id, goss->port);
	      if (!(address == memberNode->addr))
		{
		  MemberListEntry entry (*goss);
		  entry.timestamp = par->getcurrtime ();
		  memberNode->memberList.push_back (entry);
		  Address address = parseAddress (id, port);
		  log->logNodeAdd (&memberNode->addr, &address);
		}
	    }

	  found = false;

	}
      cout << "searching failed nodes in menberlist of node " << memberNode->addr.getAddress () << endl;
      for (std::vector<MemberListEntry>::iterator fail = memberNode->memberList.begin (); fail != memberNode->memberList.end (); ++fail)
	{
	  int timeout = par->getcurrtime () - fail->timestamp;

	  for (std::vector<int>::iterator failedNode = failedNodes.begin (); failedNode != failedNodes.end (); ++failedNode)
	    {
	      if (*failedNode == fail->id)
		{
		  if (timeout >= 100)
		    {

		      //add to failed nodes
		      cout << "detected failed node  " << fail->id << " at " << memberNode->addr.getAddress () << endl;
		      failedNodes.push_back (fail->id);
		      //broadcast failed messages
		      for (std::vector<MemberListEntry>::iterator msgit = memberNode->memberList.begin ();
			  msgit != memberNode->memberList.end (); ++msgit)
			{
			  if (msgit->id != fail->id)
			    {
			      size_t msgsize = sizeof(MessageHdr) + sizeof(MemberListEntry);
			      MessageHdr* FAILMSG = (MessageHdr *) malloc (msgsize * sizeof(char));
			      this->createMessage (FAIL, FAILMSG, 1);

			      Address address = parseAddress (msgit->id, msgit->port);
			      if (!(address == memberNode->addr))
				{
				  memcpy (((char*) FAILMSG + sizeof(MessageHdr)), &(msgit), sizeof(MemberListEntry));
				  emulNet->ENsend (&memberNode->addr, &address, (char *) FAILMSG, msgsize);
				  /*cout << "sending " << FAILMSG->msgType << " from " << memberNode->addr.getAddress () << " to "
				   << address.getAddress () << " with heartbeat " << FAILMSG->heartbeat << endl;*/
				  free (FAILMSG);
				}
			    }
			}

		    }
		  else if (timeout >= 200)
		    {
		      //delete from memberlist
		      failedNodes.erase (failedNode);
		      memberNode->memberList.erase (fail);
		      Address address = parseAddress (fail->id, fail->port);
		      log->logNodeRemove (&memberNode->addr, &address);

		    }
		}

	    }

	}

    }
  else if (header->msgType == FAIL)
    {
      bool present = false;
      for (std::vector<int>::iterator failedNode = failedNodes.begin (); failedNode != failedNodes.end () && !present; ++failedNode)
	{
	  if (*failedNode == goshiped.begin ()->id)
	    {
	      present = true;
	    }

	}
      if (!present)
	{
	  failedNodes.push_back (goshiped.begin ()->id);
	}

    }

  return true;
}

void
MP1Node::createMessage (MsgTypes type, MessageHdr* msg, int messages)
{
  msg->msgType = type;
  msg->heartbeat = memberNode->heartbeat;
  msg->sender = memberNode->addr;
  msg->memberSize = messages;
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

      for (std::vector<MemberListEntry>::iterator it = memberNode->memberList.begin () + 1; it != memberNode->memberList.end (); ++it)
	{

	  Address address = parseAddress (it->id, it->port);

	  size_t msgsize = sizeof(MessageHdr) + (memberNode->memberList.size () * sizeof(MemberListEntry));
	  MessageHdr* SGOSSIP = (MessageHdr *) malloc (msgsize * sizeof(char));
	  this->createMessage (GOSSIP, SGOSSIP, memberNode->memberList.size ());
	  this->fillMemberList (SGOSSIP);
	  emulNet->ENsend (&memberNode->addr, &address, (char *) SGOSSIP, msgsize);
	  cout << "sending " << SGOSSIP->msgType << " from " << memberNode->addr.getAddress () << " to " << address.getAddress ()
	      << " with heartbeat " << SGOSSIP->heartbeat << endl;
	  free (SGOSSIP);

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
	      Address address = parseAddress (it->id, it->port);
	      if (!(address == memberNode->addr))
		{
		  size_t msgsize = sizeof(MessageHdr) + (memberNode->memberList.size () * sizeof(MemberListEntry));
		  MessageHdr* GOSSIPMSG = (MessageHdr *) malloc (msgsize * sizeof(char));
		  this->createMessage (GOSSIP, GOSSIPMSG, memberNode->memberList.size ());
		  this->fillMemberList (GOSSIPMSG);
		  emulNet->ENsend (&memberNode->addr, &address, (char *) GOSSIPMSG, msgsize);
		  cout << "sending random" << GOSSIPMSG->msgType << " from " << memberNode->addr.getAddress () << " to "
		      << address.getAddress () << " with heartbeat " << GOSSIPMSG->heartbeat << endl;
		  free (GOSSIPMSG);

		  selected = selected + 1;
		  it = memberNode->memberList.begin ();
		  selectedTable[random] = 1;
		}
	    }

	}

    }
}

void
MP1Node::nodeLoopOps ()
{
  //update heartbeat
  memberNode->heartbeat++;

  //make sure
  //update my menber entry
  cout << "nodeLoopOps at node " << memberNode->addr.getAddress () << endl;
  memberNode->myPos = memberNode->memberList.begin ();
  memberNode->myPos->timestamp = par->getcurrtime ();
  memberNode->myPos->heartbeat = memberNode->heartbeat;

  gossip ();

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
  MemberListEntry myself (id, port, memberNode->heartbeat, par->getcurrtime ());
  memberNode->memberList.push_back (myself);
  log->logNodeAdd (&memberNode->addr, &memberNode->addr);
  memberNode->myPos = memberNode->memberList.begin ();
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
