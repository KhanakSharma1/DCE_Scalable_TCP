# DCE_Scalable_TCP

## Validation of ns-3 implementation of scalable TCP

### DCE-05: Validation of ns-3 implementation of scalable TCP against its Linux implementaion

Brief: Scalable TCP is a congestion control algorithm which does not reduce the congestion
window (cwnd) by ½ like the traditional TCP. Instead, it reduces the cwnd by ⅞ with an aim to
allow the sender to reach its previous cwnd value sooner. This algorithm ensures that the
bandwidth is sufficiently utilized in the presence of transient congestion. In this project, the
aim is to validate ns-3 Scalable TCP implementation by comparing the results obtained from it
to those obtained by simulating Linux Scalable TCP.

### Useful Links:
● Scalable Tcp Paper (Link: https://ieeexplore.ieee.org/abstract/document/832487/)

● Linux kernel code (Link:
https://elixir.bootlin.com/linux/v4.4/source/net/ipv4/tcp_scalable.c)

● ns-3 code for Scalable TCP (Path: ns-3.xx/src/internet/model/tcp-scalable.{h, cc})

