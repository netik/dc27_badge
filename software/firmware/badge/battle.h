#ifndef _BATTLE_H_
#define _BATTLE_H_

#include "ble_lld.h"
#include "ble_gap_lld.h"
#include "ble_peer.h"
#include "userconfig.h"

#include "vector.h"
#include "gfx.h"

#include "ides_sprite.h"
#include "entity.h"

#define FRAME_DELAY           0.033f     // timer will be set to this * 1,000,000 (33mS)
#define FPS                   30         // ... which represents about 30 FPS.
#define NETWORK_TIMEOUT       (FPS * 10) // number of ticks to timeout (10 seconds)
#define PEER_ADDR_LEN         12 + 5
#define MAX_PEERMEM           (PEER_ADDR_LEN + BLE_GAP_ADV_SET_DATA_SIZE_EXTENDED_MAX_SUPPORTED + 1)
#define COLOR_ENEMY           Red                   // TBD: Replace this with mod-con color
#define COLOR_PLAYER          HTML2COLOR(0xeeeseee) // light grey
#define SHIP_SIZE_WORLDMAP    10
#define SHIP_SIZE_ZOOMED      40
#define BULLET_SIZE           10

/*
 * We define these as macros rather than using an enum, because
 * these type definitions will also be used as part of the
 * network protocol packets, and using enums for over-the-wire
 * data structures is a bad idea. The internal storage format
 * for a C enum is Implemantation Defined (tm), which means
 * you can't count on it being a particular size. We need to
 * be unambiguous about the type and its size.
 */


typedef struct _enemy
{
  ble_gap_addr_t ble_peer_addr;
  char           name[CONFIG_NAME_MAXLEN];
  uint8_t        level;

  // these represent the current state of the user.
  int16_t        hp;
  int16_t        xp;
  int16_t        energy;
  uint8_t        ship_type;

  bool           ship_locked_in; // if the enemy has locked in their ship type
  uint8_t        ttl;
  ENTITY         e;
} ENEMY;

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
#define BATTLE_OP_ENTITY_CREATE     0x01     /* New entity in play */
#define BATTLE_OP_ENTITY_DESTROY    0x02     /* Discard existing entity */
#define BATTLE_OP_ENTITY_UPDATE     0x03     /* Entity state update */

/* VS opcodes */
#define BATTLE_OP_SHIP_SELECT       0x04 /* Current ship selection */
#define BATTLE_OP_SHIP_CONFIRM      0x05 /* Final ship choice */

/* State opcodes */
#define BATTLE_OP_IAMDEAD           0x06 /* I've been killed */
#define BATTLE_OP_YOUAREDEAD        0x07 /* You've been killed */

/*
 * Battle packet header
 * This is common to all packet types
 */

typedef struct _bp_header
{
  uint16_t    bp_opcode;                /* Opcode value */
  entity_type bp_type;                  /* Entity type */
  uint32_t    bp_id;                    /* 32-bit index/ID */
} bp_header_t;

/*
 * Entity packets. Can be used to specify that a new entity
 * (combatant, bullet, etc...) is in the simulation, has left the
 * simulation, or is being updated.
 */

typedef struct _bp_entity_pkt
{
  bp_header_t bp_header;
  uint16_t    bp_x;
  uint16_t    bp_y;
  uint16_t    bp_velocity_x;
  uint16_t    bp_velocity_y;
  uint16_t    bp_velogoal_x;
  uint16_t    bp_velogoal_y;
  uint16_t    bp_visibility;
  uint16_t    bp_pad;
} bp_entity_pkt_t;

/*
 * VS. packets. These are used in the VS. select screen,
 * when players are deciding which ship in their navy to use
 * when battling their opponent.
 */

typedef struct _bp_vs_pkt
{
  bp_header_t bp_header;
  uint16_t    bp_shiptype;
  uint16_t    bp_pad;
} bp_vs_pkt_t;

/*
 * State packets. These are used to keep the state machine
 * from possibly getting stuck.
 */

typedef struct _bp_state_pkt
{
  bp_header_t bp_header;
} bp_state_pkt_t;

#endif /* _BATTLE_H_ */
