# Tiny hello-world (x86_64)

[![Build Status](https://travis-ci.org/Recmo/docker-hello-tiny.svg)](https://travis-ci.org/Recmo/docker-hello-tiny)   [![](https://images.microbadger.com/badges/image/recmo/docker-hello-tiny.svg)](https://microbadger.com/images/recmo/docker-hello-tiny)  [![](https://images.microbadger.com/badges/version/recmo/docker-hello-tiny.svg)](https://microbadger.com/images/recmo/docker-hello-tiny)


Creates a small docker container that prints “Hello, world!” and exits.

The entire assembly code executed by the container is:

```
4000b0:       b8 01 00 00 00          mov    $0x1,%eax
4000b5:       bf 01 00 00 00          mov    $0x1,%edi
4000ba:       be d0 00 40 00          mov    $0x4000d0,%esi
4000bf:       ba 0e 00 00 00          mov    $0xe,%edx
4000c4:       0f 05                   syscall
4000c6:       b8 3c 00 00 00          mov    $0x3c,%eax
4000cb:       31 ff                   xor    %edi,%edi
4000cd:       0f 05                   syscall
```

Exactly eight instructions doing two well-defined syscalls. This container is as
fast and as safe as it can possibly get.

In the future, I would like to implement a static web server along the same principles.


https://filippo.io/linux-syscall-table/
