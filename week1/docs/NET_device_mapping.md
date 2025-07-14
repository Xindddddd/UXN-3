# NET Device Port Mapping (Week 1 Draft)

**Device Base Address:** `0xD0`
**Port Count:** 15 (indices 0–14)

| Index | Name      | Size (bytes) | Description                                                                                              |
| ----- | --------- | ------------ | -------------------------------------------------------------------------------------------------------- |
| 2–3   | `&socket` | 2            | Socket identifier returned by `socket()`, high and low bytes                                             |
| 4–5   | `&addr`   | 2            | Pointer to `@addr` struct in Uxn memory (4-byte IPv4 address + 2-byte port number)                       |
| 6–7   | `&opt`    | 2            | Pointer to options struct for `setsockopt()`                                                             |
| 8–9   | `&buf`    | 2            | Pointer to data buffer (max `$400` = 1 kB)                                                               |
| 10–11 | `&len`    | 2            | Length of the data buffer                                                                                |
| 12    | `&flags`  | 1            | Flags for `send()`/`recv()`                                                                              |
| 13    | `&status` | 1            | Return status of last API call (`>=0` success, `<0` mapped to `0xFF` in Uxn)                             |
| 14    | `&call`   | 1            | Operation code: `0=socket`, `1=bind`, `2=listen`, `3=accept`, `4=connect`, `5=send`, `6=recv`, `7=close` |

```uxntal
@addr
&ipv4_addr $4  ; 4-byte IPv4 address
&port      $2  ; 2-byte network-order port number

@buf
&data      $400  ; Data buffer (up to 1 kB)
```

---

## Uxntal Examples (week1/examples)

### simple\_socket.uxn

```uxntal
; simple_socket.uxn
; Create a TCP socket and print its file descriptor via console.

@addr
&ipv4_addr $4
&port      $2

@buf
&data      $400

; 1) socket(AF_INET, SOCK_STREAM, 0)
2 1 0        ; domain=2, type=1, protocol=0
NET.call     ; &call=0 triggers socket(), result in &status

; 2) print &status via console device
&status CON.call
```

### net\_echo.uxn

```uxntal
; net_echo.uxn
; A simple loopback echo server using the NET device.

@addr
&ipv4_addr $4
&port      $2

@buf
&data      $400

; 1) socket()
2 1 0
NET.call    ; &call=0

; 2) bind() (assumes &addr is pre-initialized in Uxntal code)
1
NET.call    ; &call=1

; 3) listen(backlog=1)
1           ; set &flags = 1
2
NET.call    ; &call=2

:loop
  ; 4) accept()
 3
 NET.call   ; &call=3

  ; 5) recv(length=32)
 32
 6
 NET.call   ; &call=6

  ; 6) send(length=32)
 32
 5
 NET.call   ; &call=5

  ; 7) close()
 7
 NET.call   ; &call=7

  jump loop
```
