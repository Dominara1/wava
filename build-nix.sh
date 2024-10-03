#!/run/current-system/sw/bin/sh

nix-build
nixos-rebuild switch
