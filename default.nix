let pkgs = import <nixpkgs> {};
in with pkgs; let
    
    devDependencies = [
        stdenv
        gcc
        gnumake
        llvmPackages.clang-unwrapped # for clang-format
        cppcheck
    ];
    
    dependencies = [
        
    ];

in {
    devEnv = stdenv.mkDerivation {
        name = "binscripter-dev";
        buildInputs = devDependencies ++ dependencies;
    };
}

