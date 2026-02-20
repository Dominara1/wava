{ pkgs ? import <nixpkgs> {} }:

pkgs.stdenv.mkDerivation {
  name = "wava";
  src = ./.;

  buildInputs = with pkgs;
  [
    wayland
    wayland-protocols
    wayland-scanner
    cmake
    gcc
    egl-wayland
    glew
    pulseaudio
    pipewire
    fftw
    fftwFloat
    pkg-config
    imagemagick
    librsvg
  ];

  buildPhase = ''
    cmake .. -DCMAKE_SKIP_BUILD_RPATH=ON
    make -j $(nproc)
    '';

  installPhase = ''
    make install
    '';


}
