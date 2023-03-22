# cliph
Dead-simple CLI SIP client (for experimental purposes mainly)

Presumably could be built on *NIX systems. At the moment only outgoing calling supported, only OPUS codec. Incoming audio quality may be low as no playout buffer is implemented atm.

Build using usual CMake routine. Find built binary in build folder in `out/bin` subfolder.

Simple usage:
```shell
./out/bin/cliph -u=sip:caller@domain.com -d=sip:callee@domain.com -o=sip:proxy.domain.com -a=<auth_user_name> -p=<auth_password>
```
