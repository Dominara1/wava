#!/run/current-system/sw/bin/sh

nix-env -f. -i
nixos-rebuild switch
