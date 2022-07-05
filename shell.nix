{ pkgs ? import <nixpkgs> {} }:


let

in pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    pkgsCross.avr.buildPackages.gcc
    gnumake
    avrdude
  ];
}