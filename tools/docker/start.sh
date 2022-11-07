#!/bin/bash
which podman-compose
if [ "$?" -eq 0 ]; then
    podman-compose up -d
else
    docker-compose up -d
fi

