# Networking

The client and the server talk over ENet, a thin reliability layer on top of UDP. Each side reads and writes [buffers](arch-core.md#buffers). Every packet starts with a 16-bit unsigned integer that names its ID.

A client goes through one of two exchanges after it connects: a status ping, or a full login. Both exchanges start the same way. Both end with the server closing the connection or keeping it open.

## Status pings

A client can connect just to ask a server about itself: protocol version, MOTD, player count, and slot limit. The server does not keep this connection open.

### Flow

|Step|Sender|Receiver|Description|
|----|----|----|----|
|1|Client|Server|The client connects|
|2|Client|Server|The client sends a status request packet|
|3|Server|Client|The server responds with a status response packet|
|4|Server|Client|The server closes the connection|

## Authentication

If the client sends a login request instead of a status request, the server starts an authentication exchange.

### Flow

|Step|Sender|Receiver|Description|
|----|----|----|----|
|1|Client|Server|The client connects|
|2|Client|Server|The client sends a login request packet: username, public Ed25519 key, invite code, protocol version, and registry hashes|
|3|Server|Client|If the server accepts the request, it responds with a challenge packet holding a random nonce|
|4|Client|Server|The client signs the nonce plus the current UTC time (minute resolution), and sends the signature back in a challenge response packet|
|5|Server|Client|If the signature matches, the server admits the client with a packet holding its assigned username|
|6|N/A|N/A|Entity exchange and game packets start here|

##### Identity

The client proves its identity with an Ed25519 keypair. The server challenges the client with a random nonce, and the client signs it.

##### Invite codes

The invite code is a one-time code. On a successful login, the server consumes the invite code. The server then adds the client's public key to its whitelist. The same client can then reconnect later without a new invite. An operator issues codes with `invite`.

##### Registry hashes

The client sends its block, biome, fluid, tint, and entity class registry hashes. The server checks each hash against its own. A mismatch means the two sides disagree on what a given ID means. When that happens, the server disconnects the client with a checksum mismatch reason. Neither side acts on the disagreement.

This check protects both sides:

- It stops a mismatched client from receiving chunk data it would render as corrupted or nonsensical.
- It stops that same client from placing a block ID the server does not recognize.

## Operator access

A server can mark an Ed25519 public key as an operator. The server keeps these keys in `operators.txt` next to `whitelist.txt` in the user directory. The file holds one exported public key per line, the same format as `whitelist.txt`.

The status is tracked per session. An operator bypasses the whitelist, so the same key does not have to appear in both files.

### Enrolling an operator

The player runs the `whoami` command in the console. The client prints its public key and its identity. The identity is the `crc64` of the key and is the value that shows up in server logs; the public key is the value the files expect.

The first operator enrolls by hand:

|Step|Who|Action|
|----|----|----|
|1|Player|Connect and run `whoami`|
|2|Server host|Copy the `public key:` line into `operators.txt`|
|3|Server host|Restart the server|

After that, an operator runs `op <public key>` or `deop <public key>`. These update `operators.txt` right away and promote or demote a matching live session without a reconnect. A hand edit still needs a restart, because the server loads the file during startup and writes it back on shutdown.

### Operator commands

An operator connects like everyone else. The server sends the command list of an operator with the operator-only commands included, and every other client gets a list without them. A client that does not know a command reports it as unknown.

|Command|Description|
|----|----|
|`who`|List connected sessions with their client IDs, identities, public keys, and access levels|
|`op`|Add a public key to the operator list and promote that player|
|`deop`|Remove a public key from the operator list and demote that player|
|`kick`|Disconnect a player by client ID or username|
|`invite`|Issue a one-time invite code|

The commands carry the `admin` and `recall` flags. `recall` lets the client forward the command to the server. `admin` makes `cvar::exec` refuse it unless the context holds a client that is an operator. A context without a client, such as `server.conf` or the client's own console, is trusted.

A demoted operator keeps the commands it already learned, but the server rejects them.

### Kicking

`kick` sends a disconnect packet with the kicked reason and then closes the connection. The key is not banned; a kicked client can reconnect unless something else, such as the whitelist or a full server, stops it.

## Cvar replication

The server owns every variable flagged with `FCVAR_REPLICATED`. After admission it sends a full snapshot of them. On every fixed tick it sends the variables that changed since the previous tick.

The client keeps the server value in a shadow. A local variable with the same name must also be flagged `FCVAR_REPLICATED`; otherwise the server value is ignored and a warning is logged. A full snapshot drops stale remote variables and clears the old shadows.

The server does not accept cvar values from clients. Gamerules use the `gr_` prefix; match rules will use `mp_`. The full list is in [Variables](../../manual/console/variables.md) and [Commands](../../manual/console/commands.md).

The command snapshot follows the same exchange. The server sends one full list to each admitted client, and the list leaves out the commands flagged `FCVAR_ADMIN` unless that client is an operator. When a live session is promoted or demoted, the server sends the list again. A dirty variable update never carries admin-flagged variables, since none of those replicate.

The snapshot packet carries a `uint8 full` flag and a `uint32 count`, then for each variable a `string name` and a `string value`. A full snapshot adds a `uint16 flags` and a `string help` for each entry.

## World

Once the server admits a client, the client asks for chunks and reports block and entity actions. Only the server can make a world change final.

##### Chunk transfer

The client sends `RequestChunk` for any chunk position it needs. The server answers with `ChunkBlocks`, holding that chunk's full block data. The server also sends `ChunkBlocks` on its own initiative, as a full resync.

##### Block and entity actions

The client sends one of four packets depending on what it targets and what it does: `PlayerAttackE`/`PlayerInteractE` for an entity target, `PlayerAttackB`/`PlayerInteractB` for a block target. There is no single unified packet with a nullable target field.

The entity variants carry only the targeted entity. The block variants carry the block's position and the ID the client expects to find there, plus the hit face, normal, and point. These three fields match the `physics::BlockHit` fields the block callbacks expect.

## Packet reference

Each packet starts with a 16-bit unsigned integer that names its ID. The packets are named and structured as follows:

### `0x0001` `StatusRequestPacket`

Request a status from server

|Type|Name|Description|
|----|----|----|
|`uint32`|`major`|Major game version|
|`uint32`|`minor`|Minor game version|
|`uint32`|`patch`|Patch game version|

### `0x0002` `StatusResponsePacket`

Server's response to a `StatusRequestPacket`

|Type|Name|Description|
|----|----|----|
|`uint32`|`major`|Major game version|
|`uint32`|`minor`|Minor game version|
|`uint32`|`patch`|Patch game version|
|`uint32`|`tags`|Server tags|
|`string`|`motd`|One-line MOTD no longer than 32 characters|
|`uint16`|`players`|Current player count|
|`uint16`|`slots`|Maximum player slots|

##### Server flags

|Value|Description|
|----|----|
|`0x00000001`|Server has whitelist enabled|
|`0x00000002`|Server enforces strict version matching|

### `0x0003` `AuthRequestPacket`

Request an authentication on a server

|Type|Name|Description|
|----|----|----|
|`uint32`|`major`|Major game version|
|`uint32`|`minor`|Minor game version|
|`uint32`|`patch`|Patch game version|
|`uint8[32]`|`pkey`|Public Ed25519 key|
|`uint64`|`invite`|Invite-code|
|`uint64`|`biomes_hash`|Biome registry hash|
|`uint64`|`blocks_hash`|Block registry hash|
|`uint64`|`fluids_hash`|Fluid registry hash|
|`uint64`|`tints_hash`|Tint registry hash|
|`uint64`|`ents_hash`|Entity registry hash|
|`string`|`username`|Desired username|

### `0x0004` `AuthChallengePacket`

Server's response to `AuthRequestPacket`

|Type|Name|Description|
|----|----|----|
|`uint8[64]`|`nonce`|Nonce for the client to sign|

### `0x0005` `AuthResponsePacket`

Client's repsonse to `AuthChallengePacket`

|Type|Name|Description|
|----|----|----|
|`uint8[64]`|`signature`|Signed nonce + UNIX minutes|

### `0x0006` `AuthAdmissionPacket`

On successful auth, server's response to `AuthResponsePacket`

|Type|Name|Description|
|----|----|----|
|`uint16`|`client_id`|Client ID|
|`uint64`|`identity`|Client identity|
|`string`|`username`|Assigned username|
|`uint64`|`entity`|Player entity|

### `0x0007` `Disconnect`

Disconnection notification. Each network side should but may not terminate the connection immediately afterwards this packet is sent

|Type|Name|Description|
|----|----|----|
|`uint32`|`reason`|Disconnect reason|

##### Reason values

|Value|Description|When|
|----|----|----|
|`0x00000000`|Unspecified reason|Anything not specified below|
|`0x00000001`|Client disconnect|Client manually disconnects|
|`0x00000002`|Client shutdown|Client closes the game|
|`0x00000003`|Entity ID desync|EnTT registry goes nuts|
|`0x00000004`|Checksum mismatch|Client isn't synced up with server mods|
|`0x00000005`|Not whitelisted|Client's public key is not in the whitelist|
|`0x00000006`|Invalid signature|Client messed up nonce signing|
|`0x00000007`|Outdated client|Client is too old for the server|
|`0x00000008`|Outdated server|Client is too new for the server|
|`0x00000009`|Server is full|No more free slots available|
|`0x0000000A`|Server shutdown|Server is terminating|
|`0x0000000C`|Kicked|An operator removed the client|

### `0x0008` `RequestChunk`

Sent by client to request a chunk to be generated or loaded and sent to the client over the network. Server performs view distance checks and may not respond at all

|Type|Name|Description|
|----|----|----|
|`vector3<int32>`|`cpos`|Requested chunk position|

### `0x0009` `ChunkBlocks`

Sent by server as a response to `RequestChunk` or at arbitrary moments to synchronize chunk contents with clients

|Type|Name|Description|
|----|----|----|
|`vector3<int32>`|`cpos`|Chunk position|
|`data`|`blocks`|Serialized and compressed block storage contents|

### `0x000A` `ChunkBiomes`

Sent by server as a response to `RequestChunk` once per realm to synchronize biome caches used for client rendering

|Type|Name|Description|
|----|----|----|
|`uint32`|`realm`|Biome realm|
|`vector2<int32>`|`cpos`|XZ-position|
|`uint32<256>`|`biomes`|A list of biome IDs|

### `0x000B` `SetBlock`

Sent by server as a sub-chunk block update

|Type|Name|Description|
|----|----|----|
|`vector3<int64>`|`bpos`|World-scale block position|
|`uint32`|`block`|Resulting numeric block ID|

### `0x000C` `PlayerAttackE`

Sent by client to specify an attack action against an entity

|Type|Name|Description|
|----|----|----|
|`uint64`|`target`|Targeted entity|

### `0x000D` `PlayerAttackB`

Sent by client to specify an attach action against a block

|Type|Name|Description|
|----|----|----|
|`vector3<int64>`|`bpos`|Targeted block position|
|`uint32`|`expected`|Numeric block ID the client expects at `bpos`|

### `0x000E` `PlayerInteractE`

Sent by client to specify an interact action against an entity

> **TODO:** send over specific part of an entity's hitbox as well

|Type|Name|Description|
|----|----|----|
|`uint64`|`target`|Targeted entity|

### `0x000F` `PlayerInteractB`

Sent by client to specify an interact action against a block

|Type|Name|Description|
|----|----|----|
|`vector3<int64>`|`bpos`|Targeted block position|
|`uint32`|`expected`|Numeric block ID the client expects at `bpos`|
|`uint8`|`face`|Hit face used for interactions|
|`vector3<float>`|`normal`|Hit normal|
|`vector3<float>`|`point`|Hit point local to the block|

### `0x0010` `EntitySpawn`

Sent by server when an entity is spawned

|Type|Name|Description|
|----|----|----|
|`uint64`|`id`|Entity ID|
|`class_id_type`|`class_id`|Numeric class ID|

### `0x0011` `EntityPatch`

Sent by server when an entity's component is updated

|Type|Name|Description|
|----|----|----|
|`uint64`|`id`|Entity ID|
|`uint32`|`ncomp`|Component count|
|`component[ncomp]`|`components`|Serialized components|

### `0x0012` `EntityRemove`

Sent by server when an entity is removed

|Type|Name|Description|
|----|----|----|
|`uint64`|`id`|Entity ID|
