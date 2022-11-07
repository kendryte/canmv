#!/bin/bash
version=1.0.0
podman build . -t ghcr.io/kendryte/k210_build:$version
podman build . -t ghcr.io/kendryte/k210_build:latest