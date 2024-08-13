# Voxelius protocol
**NOTE:** up-to-date packet specification is available as a read-only Google Spreadsheets document: [voxelius protocol](https://docs.google.com/spreadsheets/d/1rcui4Fh1t7LsoWwYqY5ICiZtVHuL3cHBzrcfqZfXPIk/edit?usp=sharing)  

# Server status
## Status request
Upon connection the client (or a separate server bothering subsystem implemented client-side) sends a `StatusRequest` packet and awaits for a response.  

## Status response
The server responds with a `StatusResponse` packet containing useful information about itself such as player count and protocol version.  

## Aftermath
Upon receiving a response the client must promptly cease the connection. If the connection is not ceased in time, the server forcefully disconnects the peer.  

# Login sequence
## Login request
Upon connection the client sends a `LoginRequest` packet and awaits a response. In terms of GUI, this should be `GUI_PROGRESS` with an "establishing connection" subtitle.  

## Login response
If the request makes sense, the server responds with a `LoginResponse` packet, initiating the login sequence.  

## Error handling
Protocol version mismatches or lack of free player slots results in the server sending a `Disconnect` packet instead of a valid response; the client must then promptly cease the connection, otherwise it gets treated with a forced disconnect.  

## Entity data
Immediately after sending a login response the server floods the channel with entity packets such as `CreateEntity`, `ChunkVoxels`, `EntityTransform` and so on. Client-side this corresponds with a `GUI_PROGRESS` screen with a "loading world" subtitle.  

## Spawning the player
After all that the server sends a `SpawnPlayer` packet. This packet contains just an entity handle, which marks a specific entity client-side should treat as the local player.  
