/*
 *  File name: defs.h
 *
 *  Author: 2013 Fall Semester AED Team
 *
 *  Release date: 2013/10/04
 *
 *  Description: Header file for an abstract type.
 *
 *  Data type list:
 *    Item
 *
 *  Function list:
 *    None
 *
 *  Dependencies:
 *    None
 *
 *  Version: 1.0
 *
 *  Change log: N/A
 *
 */


/* Prevent multiple inclusions                                      */
#ifndef defsHeader
#define defsHeader



/*
 *  Data type: Item
 *
 *  Description: Pointer to void.
 */
typedef void * Item;

typedef enum {REG, UNR, QRY, SQRY, RPL, SRPL, NAME ,AUTH,message,MSG} RQST;
/*
 * 
 * 
 */
typedef struct user{
	char *name;
	char *ip;
	int port;
}User;

/*
 * 
 * 
 * 
 */
typedef struct LinkedListStruct
{
  Item this;
  struct LinkedList * next;
}LinkedList;

/* End of: Protect multiple inclusions                              */
#endif
