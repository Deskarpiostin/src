# flake.nix
{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils, ... }@inputs:
  flake-utils.lib.eachDefaultSystem (system:
    let
      pkgs = import nixpkgs { inherit system; };
    in
    {
      # Install the game on your own computer
      packages = {
        hl2sbpp = pkgs.callPackage ./build.nix { inherit hl2sbpp-unwrapped; };
        default = self.packages.${system}.hl2sbpp;
      };

      # Build the game for others
      devShells.default = pkgs.mkShell rec {
        nativeBuildInputs = with pkgs; [
          makeWrapper
          SDL2
          freetype
          fontconfig
          zlib
          bzip2
          libjpeg
          libpng
          curl
          openal
          libopus
          pkg-config
          gcc
          python3
        ];
      };
    }
  );
}
