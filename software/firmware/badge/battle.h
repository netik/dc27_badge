#ifndef _BATTLE_H_
#define _BATTLE_H_

#include "ble_lld.h"
#include "ble_gap_lld.h"
#include "ble_peer.h"

#include "vector.h"
#include "gfx.h"

#include "ides_sprite.h"

/*
 * We define these as macros rather than using an enum, because
 * these type definitions will also be used as part of the 
 * network protocol packets, and using enums for over-the-wire
 * data structures is a bad idea. The internal storage format
 * for a C enum is Implemantation Defined (tm), which means
 * you can't count on it being a particular size. We need to
 * be unambiguous about the type and its size.
 */

typedef uint16_t entity_type;

#define T_PLAYER	0x01
#define T_ENEMY		0x02
#define T_BULLET	0x03
#define T_SPECIAL	0x04

typedef struct _entity {
  entity_type type;

  ISPID sprite_id;               /* sprite id */

  bool visible;
  bool blinking;
  int ttl;                /* if -1, always visible, else a number of frames */

  VECTOR vecVelocity;     /* current velocity */
  VECTOR vecVelocityGoal; /* goal velocity */
  VECTOR vecPosition;     /* position */
  VECTOR prevPos;         /* where we were on the last frame */
  VECTOR vecGravity;
  VECTOR vecPositionLast; /* if this doesn't match, we'll erase and repaint */

  uint8_t size_x;
  uint8_t size_y;

} ENTITY;

typedef struct _enemy {
  ble_gap_addr_t  ble_peer_addr;
  char     name[16];
  uint8_t  level;
  uint16_t xp;
  uint8_t  ship_type;
  uint8_t  ttl;
  ENTITY   e;
} ENEMY;

typedef struct _bullet {
  uint8_t kind;
  ENTITY b;
} BULLET;

/*
 * Game network protocol definitions
 *
 * We need to exchange data between players over the radio,
 * and for that we need to define some packet types. Our structure
 * definition includes an opcode, entity type, and an ID field,
 * plus some context-specific info depending on the opcode and
 * entity type.
 * Note: The packet header structure is chosen to be 8 bytes
 * in size, and packet structures are padded to be a multiple
 * of 4 bytes.
 */

/* Entity opcodes */

#define BATTLE_OP_ENTITY_CREATE		0x01 /* New entity in play */
#define BATTLE_OP_ENTITY_DESTROY	0x02 /* Discard existing entity */
#define BATTLE_OP_ENTITY_UPDATE		0x03 /* Entity state update */

/* VS opcodes */

#define BATTLE_OP_SHIP_SELECT		0x04 /* Current ship selection */
#define BATTLE_OP_SHIP_CONFIRM		0x05 /* Final ship choice */

/* State opcodes */

#define BATTLE_OP_IAMDEAD		0x06 /* I've been killed */
#define BATTLE_OP_YOUAREDEAD		0x07 /* You've been killed */

/*
 * Battle packet header
 * This is common to all packet types
 */

typedef struct _bp_header {
	uint16_t	bp_opcode;	/* Opcode value */
	entity_type	bp_type;	/* Entity type */
	uint32_t	bp_id;		/* 32-bit index/ID */
} bp_header_t;

/*
 * Entity packets. Can be used to specify that a new entity
 * (combatant, bullet, etc...) is in the simulation, has left the
 * simulation, or is being updated.
 */

typedef struct _bp_entity_pkt {
	bp_header_t	bp_header;
	uint16_t	bp_x;
	uint16_t	bp_y;
	uint16_t	bp_velocity_x;
	uint16_t	bp_velocity_y;
	uint16_t	bp_visibility;
	uint16_t	bp_pad;
} bp_entity_pkt_t;

/*
 * VS. packets. These are used in the VS. select screen,
 * when players are deciding which ship in their navy to use
 * when battling their opponent.
 */

typedef struct _bp_vs_pkt {
	bp_header_t	bp_header;
	uint16_t	bp_shiptype;
	uint16_t	bp_pad;
} bp_vs_pkt_t;

/*
 * State packets. These are used to keep the state machine
 * from possibly getting stuck.
 */

typedef struct _bp_state_pkt {
	bp_header_t	bp_header;
} bp_state_pkt_t;

#endif /* _BATTLE_H_ */
