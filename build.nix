# build.nix
{ callPackage
, symlinkJoin
, fetchurl
, makeDesktopItem
, copyDesktopItems
, hl2sbpp-unwrapped ? callPackage ./hl2sbpp-unwrapped.nix { }
, hl2sbpp-wrapper ? callPackage ./hl2sbpp-wrapper.nix { inherit hl2sbpp-unwrapped; }
}:

let
  name = "hl2sbpp";

  icon = fetchurl {
    url = "https://upload.wikimedia.org/wikipedia/commons/1/14/Half-Life_2_Logo.svg";
    hash = "sha256-/4GlYVAUSZiK7eLjM/HymcGphk5s2uCPOQuB1XOstuI=";
  };
in
symlinkJoin rec{
  inherit name;

  paths = [ 
    hl2sbpp-unwrapped
    hl2sbpp-wrapper
  ] ++ desktopItems;

  nativeBuildInputs = [
    copyDesktopItems
  ];

  postBuild = ''
    ln -s $out/bin/hl2sbpp-wrapper $out/bin/${name}

    install -Dm444 ${icon} $out/share/icons/hicolor/scalable/apps/${name}.svg
  '';

  desktopItems = [
    (makeDesktopItem {
      name = "${name}";
      exec = "${name}";
      icon = "${name}";
      desktopName = "Half-Life 2: Sandbox++";
      comment = "HL2:SB++ is a spiritual successor to HALF-LIFE 2: Sandbox, which aims to add more features than normal HL2:SB and port the game over to Android, PC and more!";
      categories = [ "Game" ];
    })
  ];
}
