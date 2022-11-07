#!/bin/bash
which podman-compose
if [ "$?" -eq 0 ]; then
    podman-compose down
else
    docker-compose rm -sf
fi