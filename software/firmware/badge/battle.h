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
#define COLOR_PLAYER          HTML2COLOR(0xeeeseee) // light grey
#define SHIP_SIZE_WORLDMAP    10
#define SHIP_SIZE_ZOOMED      40

/* Entity opcodes */
#define BATTLE_OP_ENTITY_CREATE     0x01 /* New entity in play */
#define BATTLE_OP_ENTITY_DESTROY    0x02 /* Discard existing entity */
#define BATTLE_OP_ENTITY_UPDATE     0x03 /* Entity state update */

/* VS opcodes */
#define BATTLE_OP_SHIP_SELECT       0x10 /* Current ship selection */
#define BATTLE_OP_SHIP_CONFIRM      0x11 /* Final ship choice */

/* State opcodes */
#define BATTLE_OP_TAKE_DMG          0x20 /* I hit you for X dmg... */
#define BATTLE_OP_ENG_UPDATE        0x21 /* my eng is X. */
#define BATTLE_OP_HP_UPDATE         0x22 /* my eng is X. */

#define BATTLE_OP_SET_SHIELD        0x24 /* My shield is ... */
#define BATTLE_OP_SET_HEAL          0x28 /* I am healing */
#define BATTLE_OP_SET_TELEPORT      0x2A /* I am teleporting */
#define BATTLE_OP_SET_CLOAK         0x2B /* I am cloaked */

#define BATTLE_OP_IAMDEAD           0x30 /* I've been killed */
#define BATTLE_OP_YOUAREDEAD        0x31 /* You've been killed */
#define BATTLE_OP_CLOCKUPDATE       0x32 /* this is the current game time. */

#define BATTLE_OP_ENDGAME           0xF0 /* this ends the game */

/* private memory */
typedef struct _BattleHandles
{
  GListener gl;
  GListener gl2;
  font_t    fontLG;
  font_t    fontXS;
  font_t    fontSM;
  font_t    fontSTENCIL;
  GHandle   ghTitleL;
  GHandle   ghTitleR;
  GHandle   ghACCEPT;
  GHandle   ghDECLINE;

  // these are the top row, player stats frame buffers.
  pixel_t * l_pxbuf;
  pixel_t * c_pxbuf;
  pixel_t * r_pxbuf;

  // misc variables
  uint16_t  cid;       // l2capchannelid for combat
  char      rxbuf[BLE_IDES_L2CAP_MTU];

  // left and right variants of the players; used in COMBAT only.
  ISPHOLDER *pl_left, *pl_right, *ce_left, *ce_right;
  // for the cruiser, we also need the shielded versions
  ISPHOLDER *pl_s_left, *pl_s_right, *ce_s_left, *ce_s_right;
  // for the frigate we need the healing versions
  ISPHOLDER *pl_g_left, *pl_g_right, *ce_g_left, *ce_g_right;
  // for the sub we need only the player submerged
  ISPHOLDER *pl_u_left, *pl_u_right;
} BattleHandles;

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

/*
 * Battle packet header
 * This is common to all packet types (8 bytes)
 */

typedef struct _bp_header
{
  uint16_t    bp_opcode;                /* 16-bit Opcode value */
  entity_type bp_type;                  /* 16-bit Entity type */
  uint32_t    bp_id;                    /* 32-bit index/ID */
} bp_header_t;

/*
 * Entity packets. Can be used to specify that a new entity
 * (combatant, bullet, etc...) is in the simulation, has left the
 * simulation, or is being updated. (24 bytes)
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
  uint16_t    bp_faces_right;
} bp_entity_pkt_t;

typedef struct _bp_bullet_pkt
{
  bp_header_t bp_header;
  uint16_t   bp_origin_x;
  uint16_t   bp_origin_y;
  int16_t    bp_dir_x;
  int16_t    bp_dir_y;
  uint8_t    bp_is_free;
  uint8_t    bp_pad1;
  uint16_t   bp_pad2;
} bp_bullet_pkt_t; /* 20 bytes */

/*
 * VS. packets. These are used in the VS. select screen,
 * when players are deciding which ship in their navy to use
 * when battling their opponent. (12 bytes)
 */

typedef struct _bp_vs_pkt
{
  bp_header_t bp_header;
  uint16_t    bp_shiptype;
  uint16_t    bp_unlocks;
} bp_vs_pkt_t;

/*
 * State packets. These are used to keep the state machine
 * from possibly getting stuck and to update game timers.
 * (12 bytes)
 */

typedef struct _bp_state_pkt
{
  bp_header_t bp_header;
  uint16_t    bp_operand;
  uint16_t    bp_pad;
} bp_state_pkt_t;

#endif /* _BATTLE_H_ */
