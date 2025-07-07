# NET Device Port Mapping (Week 1 Draft)

**Device Base Address:** `0xD0`  
**Port Count:** 15 (indices 0–14)

| Index   | Name     | Size (bytes) | Description                                                           |
|---------|----------|--------------|-----------------------------------------------------------------------|
| 2–3     | `&socket`| 2            | Socket identifier returned by `socket()`, high and low bytes          |
| 4–5     | `&addr`  | 2            | Pointer to `@addr` struct in Uxn memory (4-byte IPv4 + 2-byte port)  |
| 6–7     | `&opt`   | 2            | Pointer to options struct for `setsockopt()`                          |
| 8–9     | `&buf`   | 2            | Pointer to data buffer (max 1 kB)                                     |
| 10–11   | `&len`   | 2            | Length of the data buffer                                             |
| 12      | `&flags` | 1            | Flags for `send()`/`recv()`                                           |
| 13      | `&status`| 1            | Return status of last API call (>=0 success, <0 error)                |
| 14      | `&call`  | 1            | Operation code: 0=socket,1=bind,2=listen,3=accept,4=connect,5=send,6=recv,7=close |

```uxntal
@addr
&ipv4_addr $4  ; 4-byte IPv4 address
&port      $2  ; 2-byte network-order port number
@buf
&data      $400 ; Data buffer (up to 1 kB)

---

### 4. `week1/examples/net_echo.uxn`

```uxntal
; Loopback echo server example in Uxntal

; 1. socket()
&call $1
&call $0  ; operation code 0 = socket

; 2. bind()
&call $1  ; operation code 1 = bind

; 3. listen()
&call $2  ; operation code 2 = listen

:loop
  ; 4. accept()
  &call $3

  ; 5. recv()
  &call $6

  ; 6. send()
  &call $5

  ; 7. close()
  &call $7

  jump loop

