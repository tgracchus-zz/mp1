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

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Message Types
 */
enum MsgTypes
{
  JOINREQ, JOINREP, GOSSIP, FAIL, DUMMYLASTMSGTYPE
};

/**
 * STRUCT NAME: MessageHdr
 *
 * DESCRIPTION: Header and content of a message
 */
typedef struct MessageHdr
{
  enum MsgTypes msgType;
  Address sender;
  long heartbeat;
  int memberSize;
} MessageHdr;

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
  int k;
  char NULLADDR[6];
  vector<int> failedNodes;
  vector<MemberListEntry>::iterator lastGossip;

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
  initMemberListTable (Member *memberNode, int id, int port);
  void
  printAddress (Address *addr);
  virtual
  ~MP1Node ();
  void
  createMessage (MsgTypes type, MessageHdr* msg, int messages);
  void
  updateMemberList (std::vector<MemberListEntry>* goshiped);
  void
  updateMember (MemberListEntry* goss);
  bool
  isAlreadyFailedNode (int id);
  void
  fillMemberList (MessageHdr* msg);
  Address
  parseAddress (int id, int port);
  void
  gossip ();
};

#endif /* _MP1NODE_H_ */
