# SocketProgramming


```bash
# server.key / server.crt / ca.crt
openssl genrsa -out server.key 2048
openssl req -new -x509 -key server.key -out server.crt -days 365
cp server.crt ca.crt
```