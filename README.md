# Uxn Network Device

A TCP networking library for the [Uxn virtual machine](https://wiki.xxiivv.com/site/uxn.html), enabling network communication capabilities in Uxntal programs.

## Overview

This project adds a network device to Uxn, allowing Uxntal programs to create TCP servers and clients. The implementation follows the Varvara device specification and uses POSIX (BSD) sockets for cross-platform compatibility.

## Features

- **TCP Server Support**: Create listening servers that accept multiple client connections
- **TCP Client Support**: Connect to remote servers and exchange data
- **Non-blocking I/O**: Network operations don't block the Uxn main execution loop
- **Event-driven Architecture**: Network events trigger Uxntal callback vectors
- **Cross-platform**: Works on macOS, Linux, and other POSIX-compliant systems
- **Debug Support**: Comprehensive logging for network operations

## Device Specification

The network device occupies ports `0xa0-0xaf` in the Varvara device space:

```
|a0 @Network &vector $2    ; Network event callback vector
    &state $1             ; Connection state
    &type $1              ; Socket type (0x00=client, 0x01=server)
    &host $2              ; Host address pointer
    &port $2              ; Port number
    &length $2            ; Data length for read/write operations
    &read $2              ; Read buffer address
    &write $2             ; Write buffer address
```

### Device States

- `0x00` - **CLOSED**: Socket is closed
- `0x01` - **CONNECTING**: Client connection in progress
- `0x02` - **CONNECTED**: Connection established
- `0x03` - **LISTENING**: Server listening for connections
- `0x04` - **ERROR**: Network error occurred

### Commands

Write to `Network/state` to execute commands:

- `0x01` - **CONNECT**: Initiate client connection
- `0x02` - **LISTEN**: Start server listening
- `0x03` - **CLOSE**: Close connection
- `0x04` - **ACCEPT**: Accept pending server connection

## Installation

### Prerequisites

- Uxn development environment
- SDL2 (for uxnemu)
- C compiler with POSIX socket support
- netcat (for testing)

### Build Steps

1. **Clone or download the Uxn source code**:
   ```bash
   git clone https://git.sr.ht/~rabbits/uxn
   cd uxn
   ```

2. **Add network device files**:
   - Copy `network.c` to `src/devices/`
   - Copy `network.h` to `src/devices/`

3. **Modify build configuration**:
   - Add `src/devices/network.c` to the build script
   - Update `uxnemu.c` to register the network device

4. **Compile the emulator**:
   ```bash
   ./build.sh
   ```

## Usage Examples

### TCP Echo Server

```uxntal
( Echo Server Example )
|00 @System &vector $2 &wst $1 &rst $1 &eaddr $2 &ecode $2
|10 @Console &vector $2 &read $1 &pad $5 &write $1 &error $1
|a0 @Network &vector $2 &state $1 &type $1 &host $2 &port $2 &length $2 &read $2 &write $2

|0100 ( -> )
    ( Set network callback )
    ;on-network .Network/vector DEO2
    
    ( Configure as TCP server )
    #01 .Network/type DEO
    
    ( Listen on port 9999 )
    #270f .Network/port DEO2
    
    ( Start listening )
    #02 .Network/state DEO
    
BRK

@on-network ( -> )
    .Network/state DEI
    
    DUP #03 EQU ?&accept-client
    DUP #02 EQU ?&handle-client
    
    POP BRK
    
    &accept-client
        #04 .Network/state DEO  ( Accept connection )
        POP BRK
        
    &handle-client
        ( Read data )
        #0080 .Network/length DEO2
        ;buffer .Network/read DEO2
        
        ( Echo data back )
        .Network/length DEI2 .Network/length DEO2
        ;buffer .Network/write DEO2
        
        POP BRK

@buffer $80
```

### HTTP Client

```uxntal
( HTTP Client Example )
|00 @System &vector $2 &wst $1 &rst $1 &eaddr $2 &ecode $2
|10 @Console &vector $2 &read $1 &pad $5 &write $1 &error $1
|a0 @Network &vector $2 &state $1 &type $1 &host $2 &port $2 &length $2 &read $2 &write $2

|0100 ( -> )
    ;on-network .Network/vector DEO2
    
    ( Configure as TCP client )
    #00 .Network/type DEO
    
    ( Set target host and port )
    ;host .Network/host DEO2
    #0050 .Network/port DEO2
    
    ( Connect )
    #01 .Network/state DEO
    
BRK

@on-network ( -> )
    .Network/state DEI
    
    DUP #02 EQU ?&connected
    
    POP BRK
    
    &connected
        ( Send HTTP request )
        ;request string-length .Network/length DEO2
        ;request .Network/write DEO2
        
        ( Read response )
        #0200 .Network/length DEO2
        ;response .Network/read DEO2
        
        POP BRK

@host "httpbin.org $1
@request "GET 20 "/ip 20 "HTTP/1.1 0d 0a "Host: 20 "httpbin.org 0d 0a 0d 0a $1
@response $200
```

## Testing

### Compile Examples

```bash
# Compile server
./bin/uxnasm echo-server.tal echo-server.rom

# Compile client
./bin/uxnasm http-client.tal http-client.rom
```

### Test Echo Server

```bash
# Terminal 1: Start server
./bin/uxnemu echo-server.rom

# Terminal 2: Test with netcat
echo "Hello Uxn Network!" | nc localhost 9999
```

### Test HTTP Client

```bash
./bin/uxnemu http-client.rom
```

## Implementation Details

### Architecture

The network device is implemented as a standard Varvara device with:

- **C Backend** (`network.c`): Handles POSIX socket operations
- **Device Interface**: Standard DEI/DEO operations for Uxn integration
- **Event Loop**: Non-blocking I/O with event notifications
- **Memory Management**: Safe buffer handling within Uxn's 64KB address space

### Key Features

- **Non-blocking Operations**: All socket operations are non-blocking to prevent freezing the Uxn VM
- **Event-driven**: Network events automatically trigger Uxntal callback vectors
- **Error Handling**: Comprehensive error detection and reporting
- **Memory Safety**: Bounds checking for all buffer operations

### Limitations

- **TCP Only**: Currently supports only TCP connections (UDP could be added)
- **Single Connection**: Servers handle one client at a time
- **IPv4 Only**: No IPv6 support currently
- **No SSL/TLS**: Plain text connections only

## Development

### Debug Mode

The network device includes extensive debug logging. Compile with debug flags to see detailed network operations:

```bash
./build.sh --debug
```

### Adding Features

The modular design makes it easy to extend:

- **UDP Support**: Add datagram socket handling
- **Multiple Connections**: Implement connection pooling
- **DNS Resolution**: Add hostname resolution support
- **Security**: Add TLS/SSL support

