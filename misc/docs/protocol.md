# Voxelius protocol
**NOTE:** up-to-date packet specification is availabe as a read-only Google Spreadsheets document: [voxelius protocol](https://docs.google.com/spreadsheets/d/1rcui4Fh1t7LsoWwYqY5ICiZtVHuL3cHBzrcfqZfXPIk/edit?usp=sharing)  

# Server info ping
So far the idea is this:  

1. Client sends `Handshake` with `PING` intention  
2. Server responds with `ServerInfo` packet  
3. Server kindly asks the client to piss off (`enet_peer_disconnect`)  
4. Client, upon receiving the desired packet, promptly closes the connection without awaiting for a `DISCONNECT` event  

# Login sequence
So far the idea is this:  

1. Client sends `Handshake` with `LOGIN` intention (loading screen is up with the "Connecting" title)  
2. Client sends `LoginStart` (loading screen is up with the "Logging In" title)  
3. If the server doesn't like the client's attitude, it will send `Disconnect` with a reason, then politely ask the client to piss off (`enet_peer_disconnect`)
4. If everything is fine, the server, after possibly resolving a username conflict, sends `LoginSuccess` to the client
5. Upon receiving `LoginSuccess`, client awaits for entity packets (loading screen is up with the "Loading world" title)  
6. The server sends a bunch of `AddEntity`, `ChunkVoxels`, and `EntityTransform` (a combination of `EntityPosition` and `EntityAngles` shoved in a single packet) to the client  
7. When the server is done with sending world data, it sends `SpawnPlayer`, containing an EnTT entity identifier the client must use as `globals::player`; if there is no such identifier clientside or it doesn't have a `PlayerComponent`, the client promptly disconnects with an error reason (something in likes of "Player entity not present")  


# Entity updates
* Despite having a shared-bound nature, `EntityPosition`, `EntityVelocity` and other such components are to be client-bound most of the time. The only time these are sent as server-bound packets is player movement updates.  
* The server validates entity movement positions and if they're out of tolerance, the server brutally fixes player transform by sending `EntityTransform`. Clientside should interpret this as a prediction failure (if prediction is implemented);  

# Voxel updates
* The current concept idea of voxel destruction is something like `func_breakable` in GoldSource games - each voxel can be hit with a tool a fixed amount of times after which the voxel breaks; might define a packet for that  
* Voxel placing will be done with `SetVoxel` shared-bound packet; the server also validates and brutally corrects clientside actions here  

# Server features
The `ServerInfo` packet contains a 32-bit bit field that would contain various feature flags the server has implemented/enabled, these include:  
1. Password protection (`ServerInfo::PASSWORD`, bit 0). The server-bound `LoginStart` packet contains a CRC64 checksum of the password (calculated via the pre-defined table in `shared/crc64.cc`), which is not ignored when password protection is enabled; password validation failure results in a `Disconnect` packet and a polite request to piss off  
2. Strict version matching (TODO, `ServerInfo::STRICT_VER`, bit 1). The `LoginStart` packet would also contain a precise commit hash; this hash, when `STRICT_VER` flag is set, tested against server's commit hash and if they differ, the usual `Disconnect` treatment is administered; the hash is ignored when bit is clear and only protocol versions are compared. This feature would be very useful for server testing to ensure everyone has the correct game revision  

# Username limitations
In V2 (first post-historic protocol revision, `VERSION=1`) usernames can be any length and any Unicode character. This is planned to be restricted in next protocol revisions to the following limitations:  
* Up to 64 standard ASCII (7-bit, single-byte UTF-8) characters  
* Characters must be printable (`std::isprint`)  
