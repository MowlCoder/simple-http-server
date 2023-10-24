# 🥳 Simple HTTP Server

## Overview
I coded this project just for practice purpose. The server is written in C without any libraries. I'm not expert in C or Network programming, but I want to have better and deeper understanding about HTTP and network, so I wrote this tiny project.

## Features
- Serve static files
- Serve SPA App (tested with VueJS build)

## Config
Server supports a little configuration. You can see example in [`config.conf`](config.conf) file.

**WARNING**: You not allowed to use comments. I lefted them to describe every configuration option more.
```conf
PORT=3000 # port number
REQUEST_LOGGING=1 # 0 for not logging, 1 for logging
STATIC_PATH=./static # path to folder with static files
```

## Build and Run
For now it is working only on Linux systems or on Windows using WSL.

```shell
make server
./server
```

**WARNING**: The configuration file must be on the path `./config.conf` relative to the executable file

## Contacts
Feel free to contact me if you have any questions by emailing me at maikezseller@gmail.com.
