<!--
# See LICENSE file for details
-->

# My Newt Keyboard

My own Keyboard on a Adafruit nRF52 Pro Feather with MyNewt

## Overview

I am making an effort on my part to create my own BT keyboard because I
am unhappy with the state of the keyboards I have:
- New keyboard on every device
- Stupid key layout on my favourite portable keyboard.

This repo contains the software flailings I do.

## Building

The below procedure describes how to build this application for the
Adafruit nRF52 Pro Feather. I assume Ubuntu 17.10 as build environment.

1. Download and install Apache Newt.

You will need to download the Apache Newt tool, as documented in the [Getting Started Guide](http://mynewt.apache.org/latest/os/introduction/).

2. Clone this repo somewhere.

```no-highlight
    $ git clone 'https://github.com/samveen/mnk.git'
```

3. Download the Apache Mynewt Core package (in the project root dir).

```no-highlight
    $ newt install
```

4. Build the app

```no-highlight
    $ newt build mnk
```

5. Profit

```no-highlight
    $ echo "$$$$$"
```

# More background
- [Getting Started Guide](http://mynewt.apache.org/latest/os/introduction/).
- [Adafruit nRF52 Pro Feather](https://learn.adafruit.com/adafruit-nrf52-pro-feather/overview)
