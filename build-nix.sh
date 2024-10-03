#!/run/current-system/sw/bin/sh

sudo nix-env -f. -i
sudo nixos-rebuild switch
