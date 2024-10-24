{
  inputs,
  pkgs,
}: let
  inherit (inputs.hyprland.packages.${pkgs.system}) hyprland;
  inherit (hyprland) buildInputs nativeBuildInputs;
  inherit (pkgs) stdenvNoCC gcc13;

  name = "hypr-dynamic-cursors";
in
  stdenvNoCC.mkDerivation {
    inherit name buildInputs;
    src = ./..;
    nativeBuildInputs = nativeBuildInputs ++ [hyprland gcc13];

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
