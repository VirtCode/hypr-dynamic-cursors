{
  inputs,
  pkgs,
}:
pkgs.stdenvNoCC.mkDerivation rec {
  name = "hypr-dynamic-cursors";
  pname = name;
  src = ./..;
  nativeBuildInputs = inputs.hyprland.packages.${pkgs.system}.hyprland.nativeBuildInputs ++ [inputs.hyprland.packages.${pkgs.system}.hyprland pkgs.gcc13];
  buildInputs = inputs.hyprland.packages.${pkgs.system}.hyprland.buildInputs;

  dontUseCmakeConfigure = true;
  dontUseMesonConfigure = true;
  dontUseNinjaBuild = true;
  dontUseNinjaInstall = true;

  installPhase = ''
    runHook preInstall

    mkdir -p "$out/lib"
    cp -r out/* "$out/lib/lib${name}.so"

    runHook postInstall
  '';
}
