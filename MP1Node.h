/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Header file of MP1Node class.
 **********************************/

#ifndef _MP1NODE_H_
#define _MP1NODE_H_

#include "stdincludes.h"
#include "Log.h"
#include "Params.h"
#include "Member.h"
#include "EmulNet.h"
#include "Queue.h"

/**
 * Macros
 */
#define TREMOVE 20
#define TFAIL 5

/**
 * Events
 */

enum EventTypes
{
  ACTIVE, SUSPECT, FAIL, JOINED //Events to disseminate
};

typedef struct Event
{
  EventTypes type;
  Address target;
  Address claimer;
  Address claimed;
  int incarnation;
} Event;

/**
 * Messages
 */
enum MsgTypes
{
  JOINREQ, JOINREP, PING, ACK, PINGREQ
};

typedef struct MessageHdr
{
  enum MsgTypes msgType;
}MessageHdr;

enum Status
{
  ALIVE, DOUBT, CONFIRM
};

typedef struct Message
{
  Address sender;
  long heartbeat;
  Event piggyBack0;
  Event piggyBack1;
  Event piggyBack2;
  Event piggyBack3;
  Event piggyBack4;
  int sizePiggyBack = 0;
}Message;


/**
 * NodeList
 */
class MemberSwimListEntry
{
private:
  Address* address;
  long heartbeat;
  long timestamp;
  Status status;
  int incarnation;
public:
  MemberSwimListEntry (Address* address, int heartbeat, long timestamp,
		       Status status, int incarnation);
  ~MemberSwimListEntry ();
  long
  getHeartbeat () const;
  void
  setHeartbeat (long heartbeat);
  int
  getIncarnation () const;
  void
  setIncarnation (int incarnation);
  Status
  getStatus () const;
  void
  setStatus (Status status);
  long
  getTimestamp () const;
  void
  setTimestamp (long timestamp);
  Address*
  getAddress () const;
  void
  setAddress (Address* address);
};

class MembershipList
{
public:
  MembershipList ();
  ~MembershipList ();
  std::vector<MemberSwimListEntry> list;
  std::vector<MemberSwimListEntry>::iterator myPos;
};

class EventListEntry
{
public:
  const MemberSwimListEntry* member;
  const Event* event;
  int goshiped;
  EventListEntry (MemberSwimListEntry* entry, Event* event);
  ~EventListEntry ();
  const Event*
  getEvent () const;
  void
  setEvent (const Event* event);
  int
  getGoshiped () const;
  void
  setGoshiped (int goshiped);
  const MemberSwimListEntry*
  getMember () const;
  void
  setMember (const MemberSwimListEntry* member);
};

class EventList
{
public:
  EventList ();
  ~EventList ();
  std::vector<EventListEntry> list;
  std::vector<EventListEntry>::iterator myPos;
};

/**
 * CLASS NAME: MP1Node
 *
 * DESCRIPTION: Class implementing Membership protocol functionalities for failure detection
 */
class MP1Node
{

private:
  EmulNet *emulNet;
  Log *log;
  Params *par;
  Member *memberNode;
  char NULLADDR[6];

  int kRamdomProcesses;
  long timestamp;
  EventList* failuresNodes;
  EventList* updatesNodes;
  MembershipList* memberShipList;

  MemberSwimListEntry*
  addToMemberShipList (Address* address, long hbeat, long timestamp,
		       Status status);

  EventListEntry*
  addToUpdatesList (MemberSwimListEntry* member, Event* event);

  void
  handleJoinResponse (Message* msg);
  void
  handleJoinRequest (Message* msg);

public:
  MP1Node (Member *, Params *, EmulNet *, Log *, Address *);
  Member *
  getMemberNode ()
  {
    return memberNode;
  }
  int
  recvLoop ();
  static int
  enqueueWrapper (void *env, char *buff, int size);
  void
  nodeStart (char *servaddrstr, short serverport);
  int
  initThisNode (Address *joinaddr);
  int
  introduceSelfToGroup (Address *joinAddress);
  int
  finishUpThisNode ();
  void
  nodeLoop ();
  void
  checkMessages ();
  bool
  recvCallBack (void *env, char *data, int size);
  void
  nodeLoopOps ();
  int
  isNullAddress (Address *addr);
  Address
  getJoinAddress ();
  void
  initMemberListTable (Member *memberNode, int id, short port);
  void
  printAddress (Address *addr);
  virtual
  ~MP1Node ();

};

#endif /* _MP1NODE_H_ */
