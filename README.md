# Wio Terminal, DMX monitor

## Schematic

```
    +---------+
    |         |+5V----------------------+
    |   Wio   |               LTC485    |
    |         |              +--__--+   |   +[120]+  *option
    |  (5)BCM3|RXD----[1k]---|1    8|---+   |     |
    |  (15) D1|XMIT------+---|2    7|-------+-----|---(2) DMX D-  XLR 5P
    |         |          +---|3    6|-------------+---(3) DMX D+  XLR 5P
    |  (3)BCM2|TXD-----------|4    5|---+
    |         |              +------+   |
    |         |                         |
    |         |GND----------------------+-------------(1) DMX GND XLR 5P
    +---------+
```


## Memo

- 5way Up: Page up
- 5way Down: Page down
