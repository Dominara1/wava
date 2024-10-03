{
    inputs = {
    nixpkgs = {
      url = "github:nixos/nixpkgs/nixos-unstable";
    };
    wava = {
      url = "github:/Dominara1/wava";
    };
  };
  outputs = { nixpkgs, wava, ... }: wava.lib.eachDefaultSystem (system:
    let
      pkgs = import nixpkgs {
        inherit system;
      };
      wava = (with pkgs; stdenv.mkDerivation {
          pname = "wava";
          version = "0.2";
          src = fetchgit {
            url = "https://github.com/Dominara1/wava";
            rev = "v0.2";
            sha256 = lib.fakeSha256;
            fetchSubmodules = true;
          };
          nativeBuildInputs = [
            gcc
            cmake
          ];
#buildPhase = "make -j $NIX_BUILD_CORES";
          buildlPhase = ''
            mkdir -p build-nix
            cd build-nix
            cmake ..
            make -j $(npric)
          '';
        }
      );
    in rec {
      defaultApp = wava.lib.mkApp {
        drv = defaultPackage;
      };
      defaultPackage = wava;
      devShell = pkgs.mkShell {
        buildInputs = [
          wava
        ];
      };
    }
  );
}
